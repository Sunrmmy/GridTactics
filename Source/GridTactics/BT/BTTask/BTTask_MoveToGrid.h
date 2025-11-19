// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToGrid.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API UBTTask_MoveToGrid : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToGrid();

protected:
	// 任务的核心逻辑
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 每帧调用，用于检查任务是否完成
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 当任务被行为树中止时调用
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;


	// 用于在编辑器中选择黑板中的目标Actor（例如玩家）
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;
};