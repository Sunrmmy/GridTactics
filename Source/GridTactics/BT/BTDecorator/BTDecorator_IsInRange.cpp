// Fill out your copyright notice in the Description page of Project Settings.


#include "BTDecorator_IsInRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "AIController.h"

UBTDecorator_IsInRange::UBTDecorator_IsInRange()
{
	NodeName = "Is In Range";

	// 初始化黑板键，确保它们只接受Actor类型的对象
	Actor1Key.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInRange, Actor1Key), AActor::StaticClass());
	Actor2Key.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInRange, Actor2Key), AActor::StaticClass());
}

bool UBTDecorator_IsInRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	// 从黑板获取两个Actor
	AActor* Actor1 = Cast<AActor>(BlackboardComp->GetValueAsObject(Actor1Key.SelectedKeyName));
	AActor* Actor2 = Cast<AActor>(BlackboardComp->GetValueAsObject(Actor2Key.SelectedKeyName));

	if (!Actor1 || !Actor2)
	{
		// 如果任何一个Actor无效，则条件不满足
		return false;
	}

	// 计算两个Actor之间的水平距离
	const float ActualDistance = FVector::DistXY(Actor1->GetActorLocation(), Actor2->GetActorLocation());

	// 根据设置的操作符进行比较
	switch (Operator)
	{
	case EArithmeticKeyOperation::Less:
		return ActualDistance < Distance;
	case EArithmeticKeyOperation::LessOrEqual:
		return ActualDistance <= Distance;
	case EArithmeticKeyOperation::Greater:
		return ActualDistance > Distance;
	case EArithmeticKeyOperation::GreaterOrEqual:
		return ActualDistance >= Distance;
	case EArithmeticKeyOperation::Equal:
		return FMath::IsNearlyEqual(ActualDistance, Distance);
	case EArithmeticKeyOperation::NotEqual:
		return !FMath::IsNearlyEqual(ActualDistance, Distance);
	default:
		return false;
	}
}

FString UBTDecorator_IsInRange::GetStaticDescription() const
{
	// 生成一个清晰的描述，显示在行为树编辑器中
	FString OperatorDesc;
	switch (Operator)
	{
	case EArithmeticKeyOperation::Less:				OperatorDesc = "<"; break;
	case EArithmeticKeyOperation::LessOrEqual:		OperatorDesc = "<="; break;
	case EArithmeticKeyOperation::Greater:			OperatorDesc = ">"; break;
	case EArithmeticKeyOperation::GreaterOrEqual:	OperatorDesc = ">="; break;
	case EArithmeticKeyOperation::Equal:			OperatorDesc = "=="; break;
	case EArithmeticKeyOperation::NotEqual:			OperatorDesc = "!="; break;
	}

	return FString::Printf(TEXT("Distance between %s and %s %s %.1f"),
		*Actor1Key.SelectedKeyName.ToString(),	// 使用 FBlackboardKeySelector 的 ToString() 方法来获取键名
		*Actor2Key.SelectedKeyName.ToString(),
		*OperatorDesc,
		Distance);
}

