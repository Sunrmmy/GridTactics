// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToGrid.h"
#include "AIController.h"
#include "GridTactics/EnemyCharacter.h"
#include "GridTactics//GridMovementComponent.h"
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
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: TargetActor from Blackboard is NULL."));
		return EBTNodeResult::Failed;
	}
	const FVector EnemyLocation = EnemyChar->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Calculating direction from Enemy at %s to Target at %s"), *EnemyLocation.ToString(), *TargetLocation.ToString());

	// 检查目标位置是否有效
	if (TargetLocation == FVector::ZeroVector)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: Target Actor '%s' is at location (0,0,0)! Check if it's spawned correctly."), *TargetActor->GetName());
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
	UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Calculated move direction (DeltaX: %d, DeltaY: %d)"), DeltaX, DeltaY);

	// 如果没有有效的移动方向（例如目标和自身在同一格子），则任务失败
	if (DeltaX == 0 && DeltaY == 0)
	{
		return EBTNodeResult::Failed;
	}

	// 尝试开始移动
	if (GridMovementComp->TryMoveOneStep(DeltaX, DeltaY))
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: TryMoveOneStep succeeded. Returning InProgress."));
		return EBTNodeResult::InProgress;		// 移动指令成功发出，任务进入“进行中”状态
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: TryMoveOneStep failed (blocked or no stamina?). Returning Failed."));
		return EBTNodeResult::Failed;		// 移动指令失败（例如被阻挡或体力不足），任务失败
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