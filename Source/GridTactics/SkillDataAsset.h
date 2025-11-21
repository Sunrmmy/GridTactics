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

	// 技能的范围模式，基于角色面向正前方(X+)	例如: {(1,0), (2,0), (3,0), (3,1), (3,-1)} 代表角色朝向的一个T形范围
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic")
	TArray<FIntPoint> RangePattern;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill stats")
	float Cooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float StaminaCost = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float MPCost = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float TimeCost = 0.0f;

	// 技能效果
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Effects")
	float Damage = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Effects", meta = (Tooltip = "施加给目标的效果"))
	TArray<FAttributeModifier> TargetModifiers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Effects", meta = (Tooltip = "施加给自己的效果"))
	TArray<FAttributeModifier> SelfModifiers;

	// 技能的位移属性
	// 角色向前移动距离
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Movement", meta = (ClampMin = "0"))
	int32 MovementDistance = 0;

	// 如果为true，此技能需要玩家在范围内选择一个目标格子来释放，而不是只选择方向
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic")
	bool bRequiresTargetGrid = false;

	// 如果是位移技能，定义最大可以移动的格子距离。0表示没有范围限制
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic", meta = (EditCondition = "bRequiresTargetGrid", ClampMin = "0"))
	int32 MaxMovementRange = 5;
};
