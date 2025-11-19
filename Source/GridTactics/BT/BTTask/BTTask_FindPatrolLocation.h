// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolLocation.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API UBTTask_FindPatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_FindPatrolLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 巡逻半径
	UPROPERTY(EditAnywhere, Category = "AI")
	float PatrolRadius = 500.0f;

	// 存储巡逻中心点的黑板键
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector PatrolCenterKey;

	// 要写入的目标位置的黑板键
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector PatrolLocationKey;
};
