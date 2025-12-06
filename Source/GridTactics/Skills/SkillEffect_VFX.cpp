// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillEffect_VFX.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/GridMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"

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
            // 获取角色旋转
            FRotator InstigatorRotation = Instigator->GetActorRotation();

            // 将偏移向量从局部坐标转换到世界坐标
            FVector RotatedOffset = InstigatorRotation.RotateVector(ParticleOffset);

            // 应用旋转后的偏移
            FVector FinalLocation = Location + RotatedOffset;

            // 计算最终旋转
            FRotator SpawnRotation = InstigatorRotation + ParticleRotation;
            
            AActor* AttachTarget = bAttachToTarget ? Instigator : nullptr;

            // 如果启用弹道移动
            if (bEnableProjectileMovement)
            {
                ProjectileStartLocation = FinalLocation;
                
                // 使用新的计算方法
                ProjectileEndLocation = CalculateProjectileEndLocation(Instigator, TargetGrid);
                
                float Distance = FVector::Dist(ProjectileStartLocation, ProjectileEndLocation);
                ProjectileDuration = Distance / ProjectileSpeed;
                ProjectileElapsedTime = 0.0f;

                UE_LOG(LogTemp, Log, TEXT("SkillEffect_VFX: Projectile from %s to %s (Distance: %.1f cm, Duration: %.2f s)"),
                    *ProjectileStartLocation.ToString(),
                    *ProjectileEndLocation.ToString(),
                    Distance,
                    ProjectileDuration);

                // 生成 Niagara Component
                ActiveNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    World,
                    NiagaraEffect,
                    ProjectileStartLocation,
                    SpawnRotation,
                    ParticleScale,
                    true,
                    true
                );

                // 启动 Tick 更新（使用成员变量）
                World->GetTimerManager().SetTimer(
                    ProjectileTimerHandle,
                    [this]()
                    {
                        UpdateProjectileMovement(0.016f);
                    },
                    0.016f,
                    true
                );
            }
            else
            {
                SpawnParticleAtLocation(World, FinalLocation, SpawnRotation, AttachTarget);
            }
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

FVector USkillEffect_VFX::CalculateProjectileEndLocation(AActor* Instigator, FIntPoint TargetGrid) const
{
    AGridManager* GridMgr = GetGridManager(Instigator);
    if (!GridMgr)
    {
        return Instigator->GetActorLocation();
    }

    switch (ProjectileTargetMode)
    {
    case EProjectileTargetMode::ToTargetGrid:
        // 使用技能的目标格子
        return GridMgr->GridToWorld(TargetGrid);

    case EProjectileTargetMode::ToDirection:
        {
            // 沿角色朝向飞行指定网格距离
            
            // 1. 获取角色当前网格位置
            FIntPoint CurrentGrid = GridMgr->GetActorCurrentGrid(Instigator);
            
            // 2. 获取角色朝向（转换为网格方向）
            FRotator ActorRotation = Instigator->GetActorRotation();
            float Yaw = ActorRotation.Yaw;
            
            // 3. 将朝向转换为网格方向向量
            FIntPoint Direction;
            if (Yaw >= -22.5f && Yaw < 22.5f)
            {
                Direction = FIntPoint(1, 0);   // 东
            }
            else if (Yaw >= 22.5f && Yaw < 67.5f)
            {
                Direction = FIntPoint(1, 1);   // 东北
            }
            else if (Yaw >= 67.5f && Yaw < 112.5f)
            {
                Direction = FIntPoint(0, 1);   // 北
            }
            else if (Yaw >= 112.5f && Yaw < 157.5f)
            {
                Direction = FIntPoint(-1, 1);  // 西北
            }
            else if (Yaw >= 157.5f || Yaw < -157.5f)
            {
                Direction = FIntPoint(-1, 0);  // 西
            }
            else if (Yaw >= -157.5f && Yaw < -112.5f)
            {
                Direction = FIntPoint(-1, -1); // 西南
            }
            else if (Yaw >= -112.5f && Yaw < -67.5f)
            {
                Direction = FIntPoint(0, -1);  // 南
            }
            else // -67.5f ~ -22.5f
            {
                Direction = FIntPoint(1, -1);  // 东南
            }
            
            // 4. 计算目标网格位置 = 当前位置 + 方向 × 距离
            FIntPoint TargetGridPos = CurrentGrid + (Direction * ProjectileGridDistance);
            
            // 5. 转换为世界坐标
            return GridMgr->GridToWorld(TargetGridPos);
        }

    default:
        return GridMgr->GridToWorld(TargetGrid);
    }
}

void USkillEffect_VFX::UpdateProjectileMovement(float DeltaTime)
{
    if (!ActiveNiagaraComponent || !ActiveNiagaraComponent->IsValidLowLevel())
    {
        // 清理 Timer
        if (UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull))
        {
            World->GetTimerManager().ClearTimer(ProjectileTimerHandle);
        }
        return;
    }

    ProjectileElapsedTime += DeltaTime;

    if (ProjectileElapsedTime >= ProjectileDuration)
    {
        // 弹道结束
        ActiveNiagaraComponent->DestroyComponent();
        ActiveNiagaraComponent = nullptr;

        // 清理 Timer
        if (UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull))
        {
            World->GetTimerManager().ClearTimer(ProjectileTimerHandle);
        }
        return;
    }

    float Progress = FMath::Clamp(ProjectileElapsedTime / ProjectileDuration, 0.0f, 1.0f);

    // 线性插值位置
    FVector LinearPos = FMath::Lerp(ProjectileStartLocation, ProjectileEndLocation, Progress);
    
    // 抛物线高度（如果启用）
    float ArcHeight = 0.0f;
    if (ProjectileArc > 0.0f)
    {
        ArcHeight = ProjectileArc * 500.0f * FMath::Sin(Progress * PI);
    }
    
    FVector NewPos = LinearPos + FVector(0, 0, ArcHeight);

    ActiveNiagaraComponent->SetWorldLocation(NewPos);

    // 旋转朝向移动方向
    FVector Velocity = (NewPos - ActiveNiagaraComponent->GetComponentLocation());
    if (!Velocity.IsNearlyZero())
    {
        ActiveNiagaraComponent->SetWorldRotation(Velocity.Rotation());
    }
}

