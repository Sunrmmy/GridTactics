// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTacticsPlayerState.h"
#include "SkillDataAsset.generated.h"

// 技能目标类型
UENUM(BlueprintType)
enum class ESkillTargetType : uint8
{
	Self            UMETA(DisplayName = "Self"),           // 自身（无需选择）
	Direction       UMETA(DisplayName = "Direction"),      // 方向性（如冲锋）
	TargetGrid      UMETA(DisplayName = "TargetGrid"),     // 精确网格（如传送、AOE）
	TargetEnemy     UMETA(DisplayName = "TargetEnemy")     // 指定敌人（自动寻敌）
};

//// 技能效果类型
//UENUM(BlueprintType)
//enum class ESkillEffectType : uint8
//{
//	Damage          UMETA(DisplayName = "Damage"),
//	Heal            UMETA(DisplayName = "Heal"),
//	Buff            UMETA(DisplayName = "Buff"),
//	Debuff          UMETA(DisplayName = "Debuff"),
//	Displacement    UMETA(DisplayName = "Displacement")    // 位移
//};

UCLASS(BlueprintType)
class GRIDTACTICS_API USkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// --- 技能基础信息 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Info")
	FText SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Info")
	TObjectPtr<UTexture2D> SkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Logic")
	TSubclassOf<class UBaseSkill> SkillLogicClass;

	// --- 范围定义 ---

	/**
	 * 施法范围模式（相对于角色当前位置）
	 * 用于：显示可施法的格子、验证鼠标点击是否在范围内
	 */
	// 技能的范围模式，基于角色面向正前方(X+)	例如: {(1,0), (2,0), (3,0), (3,1), (3,-1)} 代表角色朝向的一个T形范围
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Range")
	TArray<FIntPoint> RangePattern;
	/**
	 * 效果范围模式（相对于目标点）
	 * 用于：显示技能效果范围、计算受影响的敌人
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Range")
	TArray<FIntPoint> EffectPattern;

	// --- 技能类型 ---

	// 技能目标类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Type")
	ESkillTargetType TargetType = ESkillTargetType::Direction;
	//// 技能效果类型
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Type")
	//ESkillEffectType EffectType = ESkillEffectType::Damage;

	// --- 技能消耗 ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill stats")
	float Cooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float StaminaCost = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float MPCost = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Stats")
	float TimeCost = 0.0f;

	// --- 施法延迟 ---

	//// 施法延迟（秒），0 = 瞬发
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Advanced", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	//float CastDelay = 0.0f;


	// ========================================
	// 可组合的 Effect 列表
	// ========================================

	/**
	 * 技能效果列表（在编辑器中实例化并配置）
	 * 按顺序执行，如果某个 Effect 返回 false 则中断后续执行
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "Skill Effects", meta = (DisplayName = "Skill Effects (Required)"))
	TArray<TObjectPtr<class USkillEffect>> SkillEffects;

};
