// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SelectAndUseSkill.generated.h"

/**
 * 选择并使用最佳技能
 * 逻辑：
 * 1. 检查所有技能的可用性（冷却、资源）
 * 2. 检查玩家是否在技能范围内
 * 3. 选择优先级最高的可用技能
 * 4. 执行技能
 */
UCLASS()
class GRIDTACTICS_API UBTTask_SelectAndUseSkill : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_SelectAndUseSkill();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** 目标玩家的黑板键 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** 技能优先级（索引小的优先） */
	UPROPERTY(EditAnywhere, Category = "Skill")
	TArray<int32> SkillPriority = {0, 1};

	/** 技能使用间隔（秒），防止连续释放技能 */
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float SkillUsageCooldown = 2.0f;

	/** 上次技能使用时间的黑板键 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector LastSkillTimeKey;

private:
	/** 任务内存结构 */
	struct FBTSelectAndUseSkillMemory
	{
		bool bSkillConfirmed = false;
		float SkillExecutionTimer = 0.0f;
		int32 ExecutingSkillIndex = -1;
	};

	virtual uint16 GetInstanceMemorySize() const override
	{
		return sizeof(FBTSelectAndUseSkillMemory);
	}

	/** 检查技能是否可用且玩家在范围内 */
	bool IsSkillUsable(class USkillComponent* SkillComp, int32 SkillIndex, AActor* Target, AActor* Self) const;

	/** 计算目标方向 */
	FIntPoint CalculateDirectionToTarget(AActor* Self, AActor* Target) const;

	///** 正在执行的技能索引 */
	//int32 ExecutingSkillIndex = -1;

	///** 技能执行计时器 */
	//float SkillExecutionTimer = 0.0f;
};
