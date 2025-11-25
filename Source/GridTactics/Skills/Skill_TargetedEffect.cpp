// Fill out your copyright notice in the Description page of Project Settings.

#include "Skill_TargetedEffect.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/SkillDataAsset.h"
#include "GridTactics/GridMovementComponent.h"
#include "GridTactics/AttributesComponent.h"
#include "GridTactics/HeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

USkill_TargetedEffect::USkill_TargetedEffect()
{
}

bool USkill_TargetedEffect::CanActivate_Implementation()
{
    if (!Super::CanActivate_Implementation())
    {
        return false;
    }

    if (!SkillData)
    {
        UE_LOG(LogTemp, Error, TEXT("Skill_TargetedEffect: SkillData is null!"));
        return false;
    }

    // 获取目标格子
    FIntPoint TargetGrid = GetMouseTargetGrid();
    if (TargetGrid == FIntPoint::ZeroValue)
    {
        UE_LOG(LogTemp, Warning, TEXT("Skill_TargetedEffect: Invalid target grid!"));
        return false;
    }

    // 验证目标在施法范围内
    if (!IsTargetInRange(TargetGrid))
    {
        UE_LOG(LogTemp, Warning, TEXT("Skill_TargetedEffect: Target out of range!"));
        return false;
    }

    return true;
}

void USkill_TargetedEffect::Activate_Implementation()
{
    if (!CanActivate_Implementation())
    {
        return;
    }

    Super::Activate_Implementation();

    FIntPoint TargetGrid = GetMouseTargetGrid();

    UE_LOG(LogTemp, Log, TEXT("  Skill_TargetedEffect: Targeting %s"), *TargetGrid.ToString());

    // 支持施法延迟
    if (SkillData->CastDelay > 0.0f)
    {
        FTimerHandle DelayHandle;
        OwnerCharacter->GetWorldTimerManager().SetTimer(
            DelayHandle,
            [this, TargetGrid]()
            {
                ExecuteEffect(TargetGrid);
            },
            SkillData->CastDelay,
            false
        );
    }
    else
    {
        ExecuteEffect(TargetGrid);
    }
}

void USkill_TargetedEffect::ExecuteEffect(FIntPoint TargetGrid)
{
    if (!SkillData)
    {
        return;
    }

    // 获取效果范围内的所有角色
    TArray<AActor*> AffectedActors = GetActorsInEffect(TargetGrid);

    UE_LOG(LogTemp, Log, TEXT("  Effect hits %d actors"), AffectedActors.Num());

    // 对每个角色应用效果
    for (AActor* Actor : AffectedActors)
    {
        if (Actor)
        {
            ApplyEffectToActor(Actor);
        }
    }

    // 绘制调试范围
    #if WITH_EDITOR
    AGridManager* GridMgr = GetGridManager();
    if (GridMgr)
    {
        TArray<FIntPoint> EffectGrids = GetEffectGrids(TargetGrid);
        for (const FIntPoint& Grid : EffectGrids)
        {
            FVector WorldPos = GridMgr->GridToWorld(Grid);
            DrawDebugSphere(
                OwnerCharacter->GetWorld(),
                WorldPos + FVector(0, 0, 50),
                30.0f,
                12,
                FColor::Red,
                false,
                2.0f
            );
        }
    }
    #endif
}

void USkill_TargetedEffect::ApplyEffectToActor(AActor* Target)
{
    if (!Target || !SkillData)
    {
        return;
    }

    UAttributesComponent* Attributes = Target->FindComponentByClass<UAttributesComponent>();
    if (!Attributes)
    {
        return;
    }

    // 根据效果类型应用
    switch (SkillData->EffectType)
    {
    case ESkillEffectType::Damage:
        Attributes->ApplyDamage(SkillData->Damage);
        UE_LOG(LogTemp, Log, TEXT("    Damaged %s for %.1f"), *Target->GetName(), SkillData->Damage);
        break;

    case ESkillEffectType::Heal:
        // TODO: 添加治疗接口
        UE_LOG(LogTemp, Log, TEXT("    Healed %s for %.1f"), *Target->GetName(), SkillData->Damage);
        break;

    case ESkillEffectType::Buff:
    case ESkillEffectType::Debuff:
        // 应用 Modifier
        for (const FAttributeModifier& Modifier : SkillData->TargetModifiers)
        {
            Attributes->AddAttributeModifier(Modifier);
        }
        break;

    default:
        break;
    }
}

FIntPoint USkill_TargetedEffect::GetMouseTargetGrid() const
{
    if (!OwnerCharacter)
    {
        return FIntPoint::ZeroValue;
    }

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC)
    {
        return FIntPoint::ZeroValue;
    }

    FVector WorldLocation, WorldDirection;
    if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        return FIntPoint::ZeroValue;
    }

    FVector PlaneOrigin = FVector::ZeroVector;
    FVector PlaneNormal = FVector::UpVector;
    
    FVector MouseGroundPos = FMath::LinePlaneIntersection(
        WorldLocation,
        WorldLocation + WorldDirection * 10000.f,
        PlaneOrigin,
        PlaneNormal
    );

    AGridManager* GridMgr = GetGridManager();
    if (GridMgr)
    {
        return GridMgr->WorldToGrid(MouseGroundPos);
    }

    return FIntPoint::ZeroValue;
}

bool USkill_TargetedEffect::IsTargetInRange(FIntPoint TargetGrid) const
{
    AGridManager* GridMgr = GetGridManager();
    if (!GridMgr || !SkillData)
    {
        return false;
    }

    FIntPoint CurrentGrid = GridMgr->GetActorCurrentGrid(OwnerCharacter);

    // 将 RangePattern 转换为世界坐标
    if (AHeroCharacter* Hero = Cast<AHeroCharacter>(OwnerCharacter))
    {
        TArray<FIntPoint> WorldRangeGrids = Hero->GetSkillRangeInWorld(SkillData->RangePattern);
        
        return WorldRangeGrids.Contains(TargetGrid);
    }

    return false;
}

TArray<FIntPoint> USkill_TargetedEffect::GetEffectGrids(FIntPoint TargetGrid) const
{
    TArray<FIntPoint> EffectGrids;

    if (!SkillData)
    {
        return EffectGrids;
    }

    // 使用 EffectPattern 计算效果范围
    FRotator CurrentRotation = OwnerCharacter->GetActorRotation();

    for (const FIntPoint& RelativePos : SkillData->EffectPattern)
    {
        // 转换为世界坐标（考虑角色朝向）
        FVector LocalPosVec(RelativePos.X, RelativePos.Y, 0);
        FVector RotatedVec = CurrentRotation.RotateVector(LocalPosVec);
        FIntPoint RotatedOffset(FMath::RoundToInt(RotatedVec.X), FMath::RoundToInt(RotatedVec.Y));

        EffectGrids.Add(TargetGrid + RotatedOffset);
    }

    return EffectGrids;
}

TArray<AActor*> USkill_TargetedEffect::GetActorsInEffect(FIntPoint TargetGrid) const
{
    TArray<AActor*> Actors;

    AGridManager* GridMgr = GetGridManager();
    if (!GridMgr)
    {
        return Actors;
    }

    TArray<FIntPoint> EffectGrids = GetEffectGrids(TargetGrid);

    for (const FIntPoint& Grid : EffectGrids)
    {
        AActor* ActorAtGrid = GridMgr->GetActorAtGrid(Grid);
        if (ActorAtGrid && ActorAtGrid != OwnerCharacter)
        {
            Actors.AddUnique(ActorAtGrid);
        }
    }

    return Actors;
}

AGridManager* USkill_TargetedEffect::GetGridManager() const
{
    if (!OwnerCharacter || !OwnerCharacter->GetWorld())
    {
        return nullptr;
    }

    return Cast<AGridManager>(
        UGameplayStatics::GetActorOfClass(OwnerCharacter->GetWorld(), AGridManager::StaticClass())
    );
}

