// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSkill.h"
#include "GameFramework/Character.h"
#include "HeroCharacter.h"
#include "SkillDataAsset.h"
#include "SkillComponent.h"
#include "GridMovementComponent.h"
#include "AttributesComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UBaseSkill::Initialize(ACharacter* InOwner, const USkillDataAsset* InSkillData)
{
	OwnerCharacter = InOwner;
	SkillData = InSkillData;
    if (OwnerCharacter)
    {
        // 在初始化时，一次性找到并缓存组件的引用
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

    // 获取移动组件，用于坐标转换
    UGridMovementComponent* MovementComp = OwnerCharacter->FindComponentByClass<UGridMovementComponent>();
    if (!MovementComp)
    {
        UE_LOG(LogTemp, Error, TEXT("Activate_Implementation failed: GridMovementComponent is missing."));
        return;
    }

    // 获取技能范围 (需要将 AHeroCharacter 转换为通用 ACharacter)
    // 注意：GetSkillRangeInWorld 是 AHeroCharacter 的函数，我们需要一种通用的方式来调用它。
    // 最好的方式是将这个函数也移到一个通用的基类或接口中。
    // 作为一个临时的、但有效的解决方案，我们可以尝试将 OwnerCharacter 转回 AHeroCharacter。
    // 但为了长期的健壮性，您应该考虑创建一个 AGridTacticsCharacter 基类。
    TArray<FIntPoint> WorldGrids;
    if (auto GridTacticsChar = Cast<AHeroCharacter>(OwnerCharacter))
    {
        WorldGrids = GridTacticsChar->GetSkillRangeInWorld(SkillData->RangePattern);
    }
    else
    {
        // 如果未来有其他类型的角色，这里需要添加相应的逻辑
        UE_LOG(LogTemp, Error, TEXT("Activate_Implementation failed: OwnerCharacter is not a AHeroCharacter, cannot get skill range."));
        return;
    }


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
                // 查找目标身上的AttributesComponent来施加伤害
                if (UAttributesComponent* TargetAttrs = OverlappedActor->FindComponentByClass<UAttributesComponent>())
                {
                    TargetAttrs->ApplyDamage(SkillData->Damage);
                    DamagedActors.Add(OverlappedActor);
                    UE_LOG(LogTemp, Log, TEXT("Damaged Actor: %s"), *OverlappedActor->GetName());
                }
            }
        } 
    }
    // 具体的视觉效果（如粒子、声音）在蓝图子类的Activate事件中实现。
}
