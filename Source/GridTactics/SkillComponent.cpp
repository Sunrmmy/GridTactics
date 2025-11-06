// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillComponent.h"
#include "SkillDataAsset.h"
#include "BaseSkill.h"
#include "HeroCharacter.h"

// Sets default values for this component's properties
USkillComponent::USkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	AHeroCharacter* OwnerCharacter = Cast<AHeroCharacter>(GetOwner());
	if (!OwnerCharacter) return;

    // 基于数据资产，实例化技能逻辑对象
    for (const USkillDataAsset* DataAsset : EquippedSkillsData)
    {
        if (DataAsset && DataAsset->SkillLogicClass)
        {
            FSkillEntry NewEntry;
            NewEntry.SkillData = DataAsset;
            NewEntry.SkillInstance = NewObject<UBaseSkill>(this, DataAsset->SkillLogicClass);
            NewEntry.SkillInstance->Initialize(OwnerCharacter, DataAsset);
            SkillSlots.Add(NewEntry);
        }
    }
	
}

// Called every frame
void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 更新所有技能的冷却时间
    for (FSkillEntry& SkillEntry : SkillSlots)
    {
        if (SkillEntry.CooldownRemaining > 0.0f)
        {
            SkillEntry.CooldownRemaining = FMath::Max(0.0f, SkillEntry.CooldownRemaining - DeltaTime);
        }
    }
}

bool USkillComponent::TryActivateSkill(int32 SkillIndex)
{
    if (!SkillSlots.IsValidIndex(SkillIndex)) return false;

    FSkillEntry& SkillEntry = SkillSlots[SkillIndex];

    // 检查冷却和能否激活
    if (SkillEntry.SkillInstance && SkillEntry.SkillInstance->CanActivate())
    {
        SkillEntry.SkillInstance->Activate();
        // 设置冷却时间
        SkillEntry.CooldownRemaining = SkillEntry.SkillData->Cooldown;
        return true;    // 技能激活成功
    }
    return false;
}
float USkillComponent::GetCooldownRemaining(int32 SkillIndex) const     // 将技能cd剩余时间传给BaseSkill的CanActivate函数来判断技能是否可用
{
    if (SkillSlots.IsValidIndex(SkillIndex))
    {
        return SkillSlots[SkillIndex].CooldownRemaining;
    }
    return 0.0f;
}

const USkillDataAsset* USkillComponent::GetSkillData(int32 SkillIndex) const
{
    // 检查索引是否在有效范围内
    if (SkillSlots.IsValidIndex(SkillIndex))
    {
        // 返回对应技能插槽中的数据资产指针
        return SkillSlots[SkillIndex].SkillData;
    }

    // 如果索引无效，返回空指针
    return nullptr;
}

int32 USkillComponent::GetSkillIndex(const UBaseSkill* SkillInstance) const
{
    for (int32 i = 0; i < SkillSlots.Num(); ++i)
    {
        if (SkillSlots[i].SkillInstance == SkillInstance)
        {
            return i;
        }
    }
    return INDEX_NONE;  //找不到则返回-1
}