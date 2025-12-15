// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAIConfig.generated.h"

/**
 * 敌人 AI 配置组件
 * 用于配置不同类型敌人的行为参数
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Blueprintable)
class GRIDTACTICS_API UEnemyAIConfig : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyAIConfig();

	// ========================================
	// 拉扯距离配置
	// ========================================

	/** 最小战斗距离（厘米） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat Distance")
	float MinCombatDistance = 200.0f;  // 2格

	/** 最大战斗距离（厘米） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat Distance")
	float MaxCombatDistance = 400.0f;  // 4格

	// ========================================
	// 移动配置
	// ========================================

	/** 每次拉扯的最大移动步数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxKitingSteps = 3;

	/** 移动随机偏移量（格子数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "0", ClampMax = "5"))
	int32 MovementRandomness = 2;

	// ========================================
	// 技能配置
	// ========================================

	/** 技能使用优先级（索引） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Skills")
	TArray<int32> SkillPriority = { 0, 1 };

	/** 技能使用间隔（秒，额外的AI思考时间） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Skills", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float SkillUsageInterval = 1.0f;

	// ========================================
	// 行为配置
	// ========================================

	/** 是否使用激进模式（总是接近玩家） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	bool bAggressiveMode = false;

	/** 是否使用防守模式（总是保持距离） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
	bool bDefensiveMode = false;
};