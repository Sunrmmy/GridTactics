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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Movement", meta = (ClampMin = "0"))
	int32 MovementDistance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic")
	bool bRequiresTargetGrid = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic", meta = (EditCondition = "bRequiresTargetGrid", ClampMin = "0"))
	int32 MaxMovementRange = 5;

	// 冲锋属性
	// 冲刺距离（网格数）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Dash", meta = (ClampMin = "0", ClampMax = "10"))
	int32 DashDistance = 3;

	// 击退距离（网格数）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Dash", meta = (ClampMin = "0", ClampMax = "5"))
	int32 KnockbackDistance = 2;

	// 是否在碰撞后停止（false=穿透）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Dash")
	bool bStopOnCollision = true;

	// 是否启用链式击退
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Dash")
	bool bEnableChainKnockback = false;

	// 冲刺持续时间（秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Dash", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float ChargeDuration = 0.3f;

	// 对被撞击敌人造成的额外伤害（叠加到 Damage）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Dash", meta = (ClampMin = "0"))
	float CollisionDamage = 0.0f;
};
