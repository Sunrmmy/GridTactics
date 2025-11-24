// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToGrid.h"
#include "AIController.h"
#include "GridTactics/EnemyCharacter.h"
#include "GridTactics/GridMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UBTTask_MoveToGrid::UBTTask_MoveToGrid()
{
	// 为任务节点在行为树编辑器中设置一个易于理解的名称
	NodeName = "Move One Step To Target";

	// 告诉行为树这个任务要每帧调用TickTask
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToGrid::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 获取AI控制器和它控制的Pawn
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: AIController is NULL."));
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: EnemyCharacter is NULL."));
		return EBTNodeResult::Failed;
	}

	// 从Pawn获取GridMovementComponent
	UGridMovementComponent* GridMovementComp = EnemyChar->GetGridMovementComponent();
	if (!GridMovementComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: GridMovementComponent is NULL."));
		return EBTNodeResult::Failed;
	}

	// 如果角色已经在移动，则认为之前的移动任务仍在进行中
	if (GridMovementComp->IsMoving())
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Already moving, returning InProgress."));
		return EBTNodeResult::InProgress;
	}

	// 从黑板获取目标Actor
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return EBTNodeResult::Failed;

	FVector TargetLocation;
	bool bHasValidTarget = false;

	// 1. 优先尝试从 TargetLocationKey 获取目标位置
	if (TargetLocationKey.IsSet() && BlackboardComp->IsVectorValueSet(TargetLocationKey.SelectedKeyName))
	{
		TargetLocation = BlackboardComp->GetValueAsVector(TargetLocationKey.SelectedKeyName);
		bHasValidTarget = true;
	}

	// 2. 如果没有有效的目标位置，再尝试从 TargetActorKey 获取
	if (!bHasValidTarget && TargetActorKey.IsSet())
	{
		AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
		if (TargetActor)
		{
			TargetLocation = TargetActor->GetActorLocation();
			bHasValidTarget = true;
		}
	}

	if (!bHasValidTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: No valid target location or actor found."));
		return EBTNodeResult::Failed;
	}

	const FVector EnemyLocation = EnemyChar->GetActorLocation();

	// 如果AI已经非常接近目标点，则认为任务成功
	if (FVector::DistSquared(EnemyLocation, TargetLocation) < 100.f)
	{
		// 如果是巡逻移动，到达后清空目标点，以便下次重新寻找
		if (TargetLocationKey.IsSet())
		{
			BlackboardComp->ClearValue(TargetLocationKey.SelectedKeyName);
		}
		return EBTNodeResult::Succeeded;
	}

	// 使用统一的 TargetLocation 来计算方向
	FVector DirectionToTarget = (TargetLocation - EnemyLocation).GetSafeNormal();

	int32 DeltaX = 0;
	int32 DeltaY = 0;

	// 四方向离散化逻辑
		if (FMath::Abs(DirectionToTarget.X) > FMath::Abs(DirectionToTarget.Y))
		{
			// X轴为主导方向
			DeltaX = (DirectionToTarget.X > 0) ? 1 : -1;
			DeltaY = 0; // 强制Y轴为0
		}
		else
		{
			// Y轴为主导方向（或与X轴相等）
			DeltaY = (DirectionToTarget.Y > 0) ? 1 : -1;
			DeltaX = 0; // 强制X轴为0
		}

	if (DeltaX == 0 && DeltaY == 0)
	{
		// 如果目标就在当前格子上，但距离检查没通过（可能在格子边缘），也视为成功
		return EBTNodeResult::Succeeded;
	}

	if (GridMovementComp->TryMoveOneStep(DeltaX, DeltaY))
	{
		return EBTNodeResult::InProgress;
	}
	else
	{
		if (TargetLocationKey.IsSet())
		{
			BlackboardComp->ClearValue(TargetLocationKey.SelectedKeyName);
		}
		return EBTNodeResult::Failed;
	}
}

void UBTTask_MoveToGrid::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid (Tick): AIController is NULL. Finishing as Failed."));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid (Tick): EnemyCharacter is NULL. Finishing as Failed."));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UGridMovementComponent* GridMovementComp = EnemyChar->GetGridMovementComponent();
	if (!GridMovementComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid (Tick): GridMovementComponent is NULL. Finishing as Failed."));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 每帧检查移动组件是否还在移动
	if (!GridMovementComp->IsMoving())
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid (Tick): Movement finished. Finishing as Succeeded."));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);		// 如果移动已经停止，说明任务完成，通知行为树任务成功
	}	// 如果仍在移动，则不执行任何操作，等待下一帧Tick
}

EBTNodeResult::Type UBTTask_MoveToGrid::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: Aborted by Behavior Tree"));
	return EBTNodeResult::Aborted;
}