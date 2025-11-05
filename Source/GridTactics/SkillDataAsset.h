// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTacticsPlayerState.h"
#include "SkillDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GRIDTACTICS_API USkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Info")
	FText SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Info")
	TObjectPtr<UTexture2D> SkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic")
	TSubclassOf<class UBaseSkill> SkillLogicClass;

	// 技能的范围模式，基于角色面向正前方(Y+)	例如: {(0,1), (0,2), (1,1), (-1,1)} 代表角色朝向的一个T形范围
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic")
	TArray<FIntPoint> RangePattern;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill stats")
	float Cooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float StaminaCost = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float MPCost = 0.0f;
};
