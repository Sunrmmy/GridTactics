// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridTactics/BaseSkill.h"
#include "Skill_Charge.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API USkill_Charge : public UBaseSkill
{
	GENERATED_BODY()
	
protected:
	virtual void Activate_Implementation() override;
};
