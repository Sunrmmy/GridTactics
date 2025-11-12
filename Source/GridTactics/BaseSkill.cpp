// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSkill.h"
#include "HeroCharacter.h"
#include "SkillDataAsset.h"
#include "SkillComponent.h"
#include "GridMovementComponent.h"
#include "GridTacticsPlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UBaseSkill::Initialize(AHeroCharacter* InOwner, const USkillDataAsset* InSkillData)
{
	OwnerCharacter = InOwner;
	SkillData = InSkillData;
    if (OwnerCharacter)
    {
        OwningComponent = OwnerCharacter->SkillComponent;
    }
}

bool UBaseSkill::CanActivate_Implementation()
{
	if (!OwnerCharacter || !SkillData || !OwningComponent) return false;

    // 检查冷却时间
    const int32 SkillIndex = OwningComponent->GetSkillIndex(this);
    if (OwningComponent->GetCooldownRemaining(SkillIndex) > 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Skill '%s' is on cooldown."), *SkillData->SkillName.ToString());
        return false;
    }

	// 获取角色的PlayerState
	AGridTacticsPlayerState* PlayerState = OwnerCharacter->GetGridTacticsPlayerState();
	if (!PlayerState) return false;

	//检查角色属性资源是否足够
	bool bHasEnoughStamina = PlayerState->GetStamina() >= SkillData->StaminaCost;
	bool bHasEnoughMP = PlayerState->GetMP() >= SkillData->MPCost;
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
    AGridTacticsPlayerState* PlayerState = OwnerCharacter->GetGridTacticsPlayerState();
    if (PlayerState)
    {
        PlayerState->ConsumeStamina(SkillData->StaminaCost);
        PlayerState->ConsumeMP(SkillData->MPCost); 
    }
    UE_LOG(LogTemp, Warning, TEXT("Skill '%s' Activated!"), *SkillData->SkillName.ToString());

    // 获取移动组件，用于坐标转换
    UGridMovementComponent* MovementComp = OwnerCharacter->GetGridMovementComponent();
    if (!MovementComp)
    {
        UE_LOG(LogTemp, Error, TEXT("Activate_Implementation failed: GridMovementComponent is missing."));
        return;
    }

    // 获取技能范围
    const TArray<FIntPoint> WorldGrids = OwnerCharacter->GetSkillRangeInWorld(SkillData->RangePattern);

    // 准备重叠检测参数
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerCharacter); // 忽略施法者自己

    // 定义我们想要检测的对象类型，这里以Pawn为例
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    TSet<AActor*> DamagedActors; // 使用TSet防止对同一个Actor造成多次伤害

    // 遍历范围格子，在每个格子上检测敌人
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
        DrawDebugSphere(GetWorld(), WorldLocation, 50, 12, FColor::Blue, false, 2.0f);

        for (AActor* OverlappedActor : OverlappedActors)
        {
            // 确保我们没有重复处理同一个Actor
            if (!DamagedActors.Contains(OverlappedActor))
            {
                UGameplayStatics::ApplyDamage(
                    OverlappedActor,
                    SkillData->Damage,
                    OwnerCharacter->GetController(),
                    OwnerCharacter,
                    nullptr // 伤害类型
                );
                DamagedActors.Add(OverlappedActor);
                UE_LOG(LogTemp, Log, TEXT("Damaged Actor: %s"), *OverlappedActor->GetName());
            }
        }
    }
    // 具体的视觉效果（如粒子、声音）在蓝图子类的Activate事件中实现。
}
