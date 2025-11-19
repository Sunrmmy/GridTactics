// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h" // 包含此文件以使用 EArithmeticKeyOperation（用于对黑板键的值进行加减运算）
#include "BTDecorator_IsInRange.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API UBTDecorator_IsInRange : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTDecorator_IsInRange();

protected:
	// 核心判断逻辑
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	// 在行为树编辑器中显示的描述
	virtual FString GetStaticDescription() const override;

private:
	// 要比较的第一个Actor的黑板键
	UPROPERTY(EditAnywhere, Category = "AI|Condition", meta = (DisplayName = "Actor 1"))
	FBlackboardKeySelector Actor1Key;

	// 要比较的第二个Actor的黑板键
	UPROPERTY(EditAnywhere, Category = "AI|Condition", meta = (DisplayName = "Actor 2"))
	FBlackboardKeySelector Actor2Key;

	// 距离阈值
	UPROPERTY(EditAnywhere, Category = "AI|Condition")
	float Distance = 500.f;

	// 距离比较操作符
	UPROPERTY(EditAnywhere, Category = "AI|Condition")
	TEnumAsByte<EArithmeticKeyOperation::Type> Operator = EArithmeticKeyOperation::Less;
};
