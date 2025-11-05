// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSkill.h"
#include "HeroCharacter.h"
#include "SkillDataAsset.h"
#include "GridTacticsPlayerState.h"

void UBaseSkill::Initialize(AHeroCharacter* InOwner, const USkillDataAsset* InSkillData)
{
	OwnerCharacter = InOwner;
	SkillData = InSkillData;
}

bool UBaseSkill::CanActivate_Implementation()
{
	if (!OwnerCharacter || !SkillData) return false;

	// 获取角色的PlayerState
	AGridTacticsPlayerState* PlayerState = OwnerCharacter->GetGridTacticsPlayerState();
	if (!PlayerState) return false;

	//检查角色属性资源是否足够
	bool bHasEnoughStamina = PlayerState->GetStamina() >= SkillData->StaminaCost;
	bool bHasEnoughMP = PlayerState->GetMP() >= SkillData->MPCost;

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
    // 具体的技能效果将子类中实现
}
