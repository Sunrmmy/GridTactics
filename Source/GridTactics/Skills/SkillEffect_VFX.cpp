// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillEffect_VFX.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/GridMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

USkillEffect_VFX::USkillEffect_VFX()
{
    EffectName = FText::FromString(TEXT("VFX/Sound"));
    bServerOnly = false;  // 特效需要在客户端执行
}

bool USkillEffect_VFX::CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const
{
    // VFX Effect 总是可以执行
    return Instigator != nullptr;
}

bool USkillEffect_VFX::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    if (!Instigator)
    {
        return false;
    }

    UWorld* World = Instigator->GetWorld();
    if (!World)
    {
        return false;
    }

    // 处理延迟播放
    if (PlayDelay > 0.0f)
    {
        FTimerHandle TimerHandle;
        World->GetTimerManager().SetTimer(
            TimerHandle,
            [this, World, Instigator, TargetGrid, AffectedActors]()
            {
                ExecuteVFX(World, Instigator, TargetGrid, AffectedActors);
            },
            PlayDelay,
            false
        );
    }
    else
    {
        ExecuteVFX(World, Instigator, TargetGrid, AffectedActors);
    }

    return true;
}

void USkillEffect_VFX::ExecuteVFX(UWorld* World, AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    // 播放粒子特效
    if (NiagaraEffect)
    {
        TArray<FVector> ParticleLocations = GetSpawnLocations(Instigator, TargetGrid, AffectedActors, ParticleSpawnLocation);
        
        for (const FVector& Location : ParticleLocations)
        {
            FRotator SpawnRotation = Instigator->GetActorRotation() + ParticleRotation;
            AActor* AttachTarget = bAttachToTarget ? Instigator : nullptr;
            SpawnParticleAtLocation(World, Location + ParticleOffset, SpawnRotation, AttachTarget);
        }
    }

    // 直接使用 Sound
    if (Sound)
    {
        TArray<FVector> SoundLocations = GetSpawnLocations(Instigator, TargetGrid, AffectedActors, SoundSpawnLocation);
        
        for (const FVector& Location : SoundLocations)
        {
            PlaySoundAtLocation(World, Location);
        }
    }
}

void USkillEffect_VFX::SpawnParticleAtLocation(UWorld* World, FVector Location, FRotator Rotation, AActor* AttachTarget)
{
    if (NiagaraEffect)
    {
        // Niagara 特效
        if (AttachTarget && AttachSocketName != NAME_None)
        {
            UNiagaraFunctionLibrary::SpawnSystemAttached(
                NiagaraEffect,
                AttachTarget->GetRootComponent(),
                AttachSocketName,
                FVector::ZeroVector,
                Rotation,
                EAttachLocation::SnapToTarget,
                true
            );
        }
        else
        {
            UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                World,
                NiagaraEffect,
                Location,
                Rotation,
                ParticleScale,
                true,
                true
            );
        }
    }
}

void USkillEffect_VFX::PlaySoundAtLocation(UWorld* World, FVector Location)
{
    if (Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            World,
            Sound,  // 直接使用
            Location,
            VolumeMultiplier,
            PitchMultiplier
        );
    }
}

TArray<FVector> USkillEffect_VFX::GetSpawnLocations(
    AActor* Instigator,
    FIntPoint TargetGrid,
    const TArray<AActor*>& AffectedActors,
    EVFXSpawnLocation SpawnType) const
{
    TArray<FVector> Locations;

    AGridManager* GridMgr = GetGridManager(Instigator);

    switch (SpawnType)
    {
    case EVFXSpawnLocation::Caster:
        Locations.Add(Instigator->GetActorLocation());
        break;

    case EVFXSpawnLocation::TargetGrid:
        if (GridMgr)
        {
            Locations.Add(GridMgr->GridToWorld(TargetGrid) + FVector(0, 0, 50.0f));
        }
        break;

    case EVFXSpawnLocation::AffectedActors:
        for (AActor* Actor : AffectedActors)
        {
            if (Actor)
            {
                Locations.Add(Actor->GetActorLocation());
            }
        }
        // 如果没有受影响目标，回退到目标格子
        if (Locations.Num() == 0 && GridMgr)
        {
            Locations.Add(GridMgr->GridToWorld(TargetGrid) + FVector(0, 0, 50.0f));
        }
        break;

    case EVFXSpawnLocation::CasterToTarget:
        // 返回施法者位置（可用于生成轨迹特效）
        Locations.Add(Instigator->GetActorLocation());
        break;
    }

    return Locations;
}

