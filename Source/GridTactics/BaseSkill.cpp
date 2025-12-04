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

    // 2. 获取受影响的角色（立即计算，供所有 Effect 使用）
    TArray<AActor*> AffectedActors = GetAffectedActors(TargetGrid);

    UE_LOG(LogTemp, Log, TEXT("ExecuteSkillEffects: %d effects on %d actors at grid %s"),
        SkillData->SkillEffects.Num(),
        AffectedActors.Num(),
        *TargetGrid.ToString());

    // 3. 先验证所有 Effect 是否可以执行（在任何延迟之前）
    for (USkillEffect* Effect : SkillData->SkillEffects)
    {
        if (!Effect) continue;

        if (!Effect->CanExecute(OwnerCharacter, TargetGrid))
        {
            UE_LOG(LogTemp, Warning, TEXT("  Effect '%s' CanExecute failed, ABORTING"),
                *Effect->EffectName.ToString());
            return false;
        }
    }

    // 4. 按各自的 ExecutionDelay 执行每个 Effect
    UWorld* World = GetWorld();
    if (!World) return false;

    for (USkillEffect* Effect : SkillData->SkillEffects)
    {
        if (!Effect) continue;

        // 在延迟执行时使用弱引用
        if (Effect->ExecutionDelay > 0.0f)
        {
            FTimerHandle TimerHandle;
            FTimerDelegate TimerDelegate;
            
            // 使用弱引用捕获 Actor 列表
            TArray<TWeakObjectPtr<AActor>> WeakAffectedActors;
            for (AActor* Actor : AffectedActors)
            {
                WeakAffectedActors.Add(Actor);
            }
            
            TWeakObjectPtr<AActor> WeakOwner(OwnerCharacter);
            
            TimerDelegate.BindLambda([Effect, WeakOwner, TargetGrid, WeakAffectedActors]()
            {
                // 检查施法者是否仍然有效
                AActor* Owner = WeakOwner.Get();
                if (!Effect || !Owner)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Effect execution cancelled - owner destroyed"));
                    return;
                }
                
                // 过滤出仍然有效的目标
                TArray<AActor*> ValidActors;
                for (const TWeakObjectPtr<AActor>& WeakActor : WeakAffectedActors)
                {
                    if (AActor* Actor = WeakActor.Get())
                    {
                        ValidActors.Add(Actor);
                    }
                }
                
                Effect->Execute(Owner, TargetGrid, ValidActors);
                
                UE_LOG(LogTemp, Log, TEXT("  Effect '%s' executed after delay with %d/%d valid targets"),
                    *Effect->EffectName.ToString(), 
                    ValidActors.Num(), 
                    WeakAffectedActors.Num());
            });

            World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Effect->ExecutionDelay, false);

            UE_LOG(LogTemp, Log, TEXT("  Effect '%s' scheduled with %.2fs delay"),
                *Effect->EffectName.ToString(), Effect->ExecutionDelay);
        }
        else
        {
            // 立即执行
            Effect->Execute(OwnerCharacter, TargetGrid, AffectedActors);
            UE_LOG(LogTemp, Log, TEXT("  Effect '%s' executed immediately"),
                *Effect->EffectName.ToString());
        }
    }

    return true;
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
        // 修复：方向性技能的目标格子是玩家自己的位置
        // （效果范围会根据角色朝向从玩家位置向外延伸）
        return GridMgr->GetActorCurrentGrid(OwnerCharacter);

    case ESkillTargetType::TargetGrid:
        // 精确目标技能（如传送、AOE）：目标是鼠标所在的格子
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
        // 修复：根据技能类型决定如何计算效果范围
        if (SkillData->TargetType == ESkillTargetType::Direction)
        {
            // 方向性技能：使用 GetSkillRangeInWorld（以玩家为中心，考虑朝向）
            WorldGrids = Hero->GetSkillRangeInWorld(PatternToUse);
        }
        else
        {
            // 其他技能（如 TargetGrid）：以目标格子为中心
            WorldGrids = Hero->GetSkillRangeInWorldFromCenter(PatternToUse, TargetGrid);
        }
    }
    else
    {
        // 直接使用相对坐标
        for (const FIntPoint& RelativePos : PatternToUse)
        {
            WorldGrids.Add(TargetGrid + RelativePos);
        }
    }

    //// 检测每个格子上的角色
    //TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    //ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    //TArray<AActor*> ActorsToIgnore;
    //// ActorsToIgnore.Add(OwnerCharacter);

    //for (const FIntPoint& Grid : WorldGrids)
    //{
    //    FVector WorldLocation = MovementComp->GridToWorld(Grid.X, Grid.Y);
    //    
    //    // 修复：将检测高度提高到角色可能存在的位置
    //    WorldLocation.Z = OwnerCharacter->GetActorLocation().Z;
    //    
    //    TArray<AActor*> OverlappedActors;

    //    UKismetSystemLibrary::SphereOverlapActors(
    //        GetWorld(),
    //        WorldLocation,
    //        50.0f,
    //        ObjectTypes,
    //        nullptr,
    //        ActorsToIgnore,
    //        OverlappedActors
    //    );

    //    for (AActor* Actor : OverlappedActors)
    //    {
    //        AffectedActors.AddUnique(Actor);
    //    }
    //}

    //UE_LOG(LogTemp, Log, TEXT("GetAffectedActors: Found %d actors in %d grids"), 
    //    AffectedActors.Num(), WorldGrids.Num());



    // 备选方案：遍历所有角色，检查其网格位置
    TArray<AActor*> AllCharacters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllCharacters);

    UE_LOG(LogTemp, Warning, TEXT("=== GetAffectedActors (Grid-Based) ==="));
    UE_LOG(LogTemp, Warning, TEXT("TargetType: %d, TargetGrid: %s"), 
        static_cast<int32>(SkillData->TargetType), *TargetGrid.ToString());
    UE_LOG(LogTemp, Warning, TEXT("WorldGrids: %d, AllCharacters: %d"), WorldGrids.Num(), AllCharacters.Num());

    for (AActor* Actor : AllCharacters)
    {
        if (!Actor) continue;

        // 获取角色所在的网格
        int32 ActorGridX, ActorGridY;
        MovementComp->WorldToGrid(Actor->GetActorLocation(), ActorGridX, ActorGridY);
        FIntPoint ActorGrid(ActorGridX, ActorGridY);

        UE_LOG(LogTemp, Log, TEXT("  Character %s at Grid %s"),
            *Actor->GetName(), *ActorGrid.ToString());

        // 检查是否在目标网格列表中
        if (WorldGrids.Contains(ActorGrid))
        {
            UE_LOG(LogTemp, Warning, TEXT("    -> MATCHED! Adding to AffectedActors"));
            AffectedActors.AddUnique(Actor);

            // 可视化：绘制命中的角色（红色）
            DrawDebugSphere(
                GetWorld(),
                Actor->GetActorLocation(),
                50.0f,
                12,
                FColor::Red,
                false,
                3.0f,
                0,
                3.0f
            );
        }
    }

    // 可视化：绘制所有检测格子（绿色）
    for (const FIntPoint& Grid : WorldGrids)
    {
        FVector WorldLocation = MovementComp->GridToWorld(Grid.X, Grid.Y);
        WorldLocation.Z = OwnerCharacter->GetActorLocation().Z;

        DrawDebugBox(
            GetWorld(),
            WorldLocation,
            FVector(50.0f, 50.0f, 100.0f),
            FColor::Green,
            false,
            3.0f,
            0,
            2.0f
        );
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Total Found: %d actors ==="), AffectedActors.Num());

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
