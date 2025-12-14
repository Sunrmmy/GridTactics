// Fill out your copyright notice in the Description page of Project Settings.


#include "GridTacticsGameInstance.h"
#include "Skills/SkillDataAsset.h"

UGridTacticsGameInstance::UGridTacticsGameInstance()
{
}


void UGridTacticsGameInstance::AddPendingSkill(USkillDataAsset* Skill)
{
    if (!Skill)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameInstance: Attempted to add null skill"));
        return;
    }

    PendingSkills.Add(Skill);
    UE_LOG(LogTemp, Log, TEXT("GameInstance: Added skill: %s (Total: %d)"),
        *Skill->SkillName.ToString(),
        PendingSkills.Num());
}

