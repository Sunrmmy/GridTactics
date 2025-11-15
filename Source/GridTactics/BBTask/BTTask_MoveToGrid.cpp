// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToGrid.h"
#include "AIController.h"
#include "../EnemyCharacter.h"
#include "../GridMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UBTTask_MoveToGrid::UBTTask_MoveToGrid()
{
	// 为任务节点在行为树编辑器中设置一个易于理解的名称
	NodeName = "Move One Step To Target";
}

EBTNodeResult::Type UBTTask_MoveToGrid::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 获取AI控制器和它控制的Pawn
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar)
	{
		return EBTNodeResult::Failed;
	}

	// 从Pawn获取GridMovementComponent
	UGridMovementComponent* GridMovementComp = EnemyChar->GetGridMovementComponent();
	if (!GridMovementComp)
	{
		return EBTNodeResult::Failed;
	}

	// 从黑板获取目标Actor
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	// 计算朝向目标的方向，并离散化为单步移动
	FVector DirectionToTarget = (TargetActor->GetActorLocation() - EnemyChar->GetActorLocation()).GetSafeNormal();

	int32 DeltaX = 0;
	int32 DeltaY = 0;

	// 优先移动绝对值更大的轴，以确保是四方向移动，而不是对角线
	if (FMath::Abs(DirectionToTarget.X) > FMath::Abs(DirectionToTarget.Y))
	{
		DeltaX = (DirectionToTarget.X > 0) ? 1 : -1;
	}
	else
	{
		DeltaY = (DirectionToTarget.Y > 0) ? 1 : -1;
	}

	// 调用组件的移动函数
	GridMovementComp->TryMoveOneStep(DeltaX, DeltaY);

	// 任务成功发出移动指令
	return EBTNodeResult::Succeeded;
}