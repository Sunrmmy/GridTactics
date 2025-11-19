// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_DetectPlayer.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API UBTService_DetectPlayer : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_DetectPlayer();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 在编辑器中设置索敌范围
	UPROPERTY(EditAnywhere, Category = "AI")
	float DetectionRadius = 1000.0f;

	// 在编辑器中设置要写入的目标黑板键
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector TargetActorKey;
};
