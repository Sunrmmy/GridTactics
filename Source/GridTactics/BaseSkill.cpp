// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSkill.h"
#include "GameFramework/Character.h"
#include "HeroCharacter.h"
#include "SkillDataAsset.h"
#include "SkillComponent.h"
#include "GridMovementComponent.h"
#include "GridManager.h"
#include "AttributesComponent.h"
#include "Skills/SkillEffect.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

void UBaseSkill::Initialize(ACharacter* InOwner, const USkillDataAsset* InSkillData)
{
    OwnerCharacter = InOwner;
    SkillData = InSkillData;

    if (OwnerCharacter)
    {
        OwningComponent = OwnerCharacter->FindComponentByClass<USkillComponent>();
        AttributesComp = OwnerCharacter->FindComponentByClass<UAttributesComponent>();
    }
}

bool UBaseSkill::CanActivate_Implementation()
{
	if (!OwnerCharacter || !SkillData || !OwningComponent || !AttributesComp) return false;

    // 检查冷却时间
    const int32 SkillIndex = OwningComponent->GetSkillIndex(this);
    if (OwningComponent->GetCooldownRemaining(SkillIndex) > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Skill '%s' is on cooldown."), *SkillData->SkillName.ToString());
        return false;
    }

    // 检查资源
    bool bHasEnoughStamina = AttributesComp->GetStamina() >= SkillData->StaminaCost;
    bool bHasEnoughMP = AttributesComp->GetMP() >= SkillData->MPCost;

    if (!bHasEnoughMP)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough MP to activate skill '%s'."), *SkillData->SkillName.ToString());
    }
    else if (!bHasEnoughStamina)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough Stamina to activate skill '%s'."), *SkillData->SkillName.ToString());
    }
	return bHasEnoughStamina && bHasEnoughMP;
}

void UBaseSkill::Activate_Implementation()
{
    if (!CanActivate_Implementation()) return;

    // 消耗属性资源
    if (AttributesComp)
    {
        AttributesComp->ConsumeStamina(SkillData->StaminaCost);
        AttributesComp->ConsumeMP(SkillData->MPCost);
    }

    UE_LOG(LogTemp, Warning, TEXT("Skill '%s' Activated!"), *SkillData->SkillName.ToString());

    // 直接执行 Effect 系统（不再检查向后兼容）
    ExecuteSkillEffects();
}

// ========================================
// Effect 系统实现
// ========================================

bool UBaseSkill::ExecuteSkillEffects()
{
    if (!SkillData || SkillData->SkillEffects.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteSkillEffects: No SkillEffects defined in '%s'!"), 
            *SkillData->SkillName.ToString());
        return false;
    }

    // 1. 获取目标格子
    FIntPoint TargetGrid = GetTargetGrid();

    if (TargetGrid == FIntPoint::ZeroValue)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExecuteSkillEffects: Invalid target grid"));
        return false;
    }

    // 2. 获取受影响的角色
    TArray<AActor*> AffectedActors = GetAffectedActors(TargetGrid);

    UE_LOG(LogTemp, Log, TEXT("  Executing %d effects on %d actors at grid %s"),
        SkillData->SkillEffects.Num(),
        AffectedActors.Num(),
        *TargetGrid.ToString());

    // 3. 按顺序执行所有 Effect
    bool bAnySucceeded = false;

    for (USkillEffect* Effect : SkillData->SkillEffects)
    {
        if (!Effect)
        {
            UE_LOG(LogTemp, Warning, TEXT("  Null Effect in SkillEffects array"));
            continue;
        }

        // 检查是否可以执行
        if (!Effect->CanExecute(OwnerCharacter, TargetGrid))
        {
            UE_LOG(LogTemp, Warning, TEXT("  Effect '%s' CanExecute failed, skipping"),
                *Effect->EffectName.ToString());
            continue;
        }

        // 执行 Effect
        bool bSuccess = Effect->Execute(OwnerCharacter, TargetGrid, AffectedActors);

        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("  Effect '%s' executed successfully"),
                *Effect->EffectName.ToString());
            bAnySucceeded = true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("  Effect '%s' execution failed"),
                *Effect->EffectName.ToString());
        }
    }

    return bAnySucceeded;
}

FIntPoint UBaseSkill::GetTargetGrid() const
{
    if (!OwnerCharacter || !SkillData || !OwningComponent)
    {
        return FIntPoint::ZeroValue;
    }

    AGridManager* GridMgr = Cast<AGridManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
    );

    if (!GridMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("GetTargetGrid: GridManager not found"));
        return FIntPoint::ZeroValue;
    }

    // 根据 TargetType 决定目标格子
    switch (SkillData->TargetType)
    {
    case ESkillTargetType::Self:
        // 目标是自己的当前格子
        return GridMgr->GetActorCurrentGrid(OwnerCharacter);

    case ESkillTargetType::Direction:
    case ESkillTargetType::TargetGrid:
        // 从 SkillComponent 获取瞄准的目标格子
        return OwningComponent->GetAimingTargetGrid();

    case ESkillTargetType::TargetEnemy:
        // TODO: 自动寻找最近的敌人
        return GridMgr->GetActorCurrentGrid(OwnerCharacter);

    default:
        return FIntPoint::ZeroValue;
    }
}

TArray<AActor*> UBaseSkill::GetAffectedActors(FIntPoint TargetGrid) const
{
    TArray<AActor*> AffectedActors;

    UGridMovementComponent* MovementComp = GetGridMovementComponent();
    if (!MovementComp || !SkillData)
    {
        return AffectedActors;
    }

    // 使用 EffectPattern（如果为空则使用 RangePattern）
    TArray<FIntPoint> PatternToUse = SkillData->EffectPattern.Num() > 0
        ? SkillData->EffectPattern
        : SkillData->RangePattern;

    // 将 Pattern 转换为世界坐标格子
    TArray<FIntPoint> WorldGrids;
    
    if (auto Hero = Cast<AHeroCharacter>(OwnerCharacter))
    {
        // 使用 HeroCharacter 的转换函数（考虑朝向）
        WorldGrids = Hero->GetSkillRangeInWorldFromCenter(PatternToUse, TargetGrid);
    }
    else
    {
        // 直接使用相对坐标
        for (const FIntPoint& RelativePos : PatternToUse)
        {
            WorldGrids.Add(TargetGrid + RelativePos);
        }
    }

    // 检测每个格子上的角色
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerCharacter);

    for (const FIntPoint& Grid : WorldGrids)
    {
        FVector WorldLocation = MovementComp->GridToWorld(Grid.X, Grid.Y);
        TArray<AActor*> OverlappedActors;

        UKismetSystemLibrary::SphereOverlapActors(
            GetWorld(),
            WorldLocation,
            50.0f, // 半径50cm，确保能覆盖格子中心
            ObjectTypes,
            nullptr, // 不按特定类过滤，也可以指定为敌人基类
            ActorsToIgnore,
            OverlappedActors
        );

        for (AActor* Actor : OverlappedActors)
        {
            AffectedActors.AddUnique(Actor);
        }
    }

    return AffectedActors;
}

UGridMovementComponent* UBaseSkill::GetGridMovementComponent() const
{
    if (!OwnerCharacter)
    {
        return nullptr;
    }

    return OwnerCharacter->FindComponentByClass<UGridMovementComponent>();
}
