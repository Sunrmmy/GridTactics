// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillEffect.h"
#include "SkillEffect_Teleport.generated.h"

/**
 * 传送效果 - 将施法者传送到目标位置
 */
UCLASS(Blueprintable, EditInlineNew, meta = (DisplayName = "Teleport Effect"))
class GRIDTACTICS_API USkillEffect_Teleport : public USkillEffect
{
	GENERATED_BODY()

public:
    USkillEffect_Teleport();

    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors) override;
    virtual bool CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const override;
};
