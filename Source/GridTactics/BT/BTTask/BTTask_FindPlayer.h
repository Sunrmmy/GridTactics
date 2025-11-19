// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPlayer.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API UBTTask_FindPlayer : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
    UBTTask_FindPlayer();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 黑板键选择器，让我们可以再编辑器里指定要写入哪个键
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetPlayerKey;
};
