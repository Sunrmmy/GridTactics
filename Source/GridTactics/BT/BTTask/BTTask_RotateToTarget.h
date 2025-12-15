// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateToTarget.generated.h"

/**
 * 旋转角色朝向目标（对齐到四个方向）
 */
UCLASS()
class GRIDTACTICS_API UBTTask_RotateToTarget : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_RotateToTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 目标玩家的黑板键 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;
};
