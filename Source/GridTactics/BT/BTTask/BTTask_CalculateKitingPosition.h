// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CalculateKitingPosition.generated.h"

/**
 * 计算围绕玩家拉扯的目标位置
 * 特点：随机性 + 保持距离 + 尊重体力限制
 */
UCLASS()
class GRIDTACTICS_API UBTTask_CalculateKitingPosition : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_CalculateKitingPosition();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 目标玩家的黑板键 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** 输出：计算出的目标位置 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector KitingPositionKey;

	/** 期望距离范围（最小） */
	UPROPERTY(EditAnywhere, Category = "Kiting", meta = (ClampMin = "1.0"))
	float MinDesiredDistance = 200.0f;  // 2格

	/** 期望距离范围（最大） */
	UPROPERTY(EditAnywhere, Category = "Kiting", meta = (ClampMin = "1.0"))
	float MaxDesiredDistance = 500.0f;  // 4格

	/** 随机偏移量（网格数） */
	UPROPERTY(EditAnywhere, Category = "Kiting", meta = (ClampMin = "0", ClampMax = "5"))
	int32 RandomOffset = 2;

	/** 最大移动步数（体力限制） */
	UPROPERTY(EditAnywhere, Category = "Kiting", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxMoveSteps = 3;

private:
	/** 检查位置是否可行走 */
	bool IsPositionValid(class AGridManager* GridMgr, FIntPoint GridPos, AActor* SelfActor) const;

	/** 计算网格距离 */
	int32 GetGridDistance(FIntPoint A, FIntPoint B) const;
};
