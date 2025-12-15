// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_MoveToGrid.h"
#include "AIController.h"
#include "GridTactics/EnemyCharacter.h"
#include "GridTactics/GridMovement/GridMovementComponent.h"
#include "GridTactics/AttributesComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UBTTask_MoveToGrid::UBTTask_MoveToGrid()
{
	NodeName = "Move To Target Location";
	bNotifyTick = true;
	
	// 设置默认黑板键名
	TargetLocationKey.SelectedKeyName = FName("KitingPosition");
	// TargetActorKey 留空（暂不使用）
}

EBTNodeResult::Type UBTTask_MoveToGrid::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("========== BTTask_MoveToGrid: ExecuteTask START =========="));

	// 获取AI控制器和它控制的Pawn
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: No AIController!"));
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: No EnemyCharacter!"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Enemy = %s"), *EnemyChar->GetName());

	// 从Pawn获取GridMovementComponent
	UGridMovementComponent* GridMovementComp = EnemyChar->GetGridMovementComponent();
	if (!GridMovementComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: No GridMovementComponent!"));
		return EBTNodeResult::Failed;
	}

	// 如果已经在移动，等待完成
	if (GridMovementComp->IsMoving())
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Already moving, returning InProgress"));
		return EBTNodeResult::InProgress;
	}

	// 从黑板获取目标Actor
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: No Blackboard!"));
		return EBTNodeResult::Failed;
	}

	FVector TargetLocation;
	bool bHasValidTarget = false;

	// ✅ 修复：直接检查黑板键名是否有效，而不是使用 IsSet()
	if (TargetLocationKey.SelectedKeyName != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Trying to read key '%s'"), 
			*TargetLocationKey.SelectedKeyName.ToString());
		
		if (BlackboardComp->IsVectorValueSet(TargetLocationKey.SelectedKeyName))
		{
			TargetLocation = BlackboardComp->GetValueAsVector(TargetLocationKey.SelectedKeyName);
			bHasValidTarget = true;
			UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: Got target from TargetLocationKey: %s"), 
				*TargetLocation.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: Key '%s' is not set in Blackboard!"), 
				*TargetLocationKey.SelectedKeyName.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: TargetLocationKey.SelectedKeyName is NAME_None!"));
	}

	// 备选：从 TargetActorKey 获取
	if (!bHasValidTarget && TargetActorKey.SelectedKeyName != NAME_None)
	{
		AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
		if (TargetActor)
		{
			TargetLocation = TargetActor->GetActorLocation();
			bHasValidTarget = true;
			UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Got target from TargetActorKey: %s at %s"), 
				*TargetActor->GetName(), *TargetLocation.ToString());
		}
	}

	if (!bHasValidTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: No valid target found!"));
		return EBTNodeResult::Failed;
	}

	const FVector EnemyLocation = EnemyChar->GetActorLocation();
	float DistanceToTarget = FVector::Dist(EnemyLocation, TargetLocation);

	UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Current = %s, Target = %s, Distance = %.1f"), 
		*EnemyLocation.ToString(), *TargetLocation.ToString(), DistanceToTarget);

	// 检查是否已到达目标
	if (FVector::DistSquared(EnemyLocation, TargetLocation) < 100.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: Already at target! Clearing target key."));
		if (TargetLocationKey.IsSet())
		{
			BlackboardComp->ClearValue(TargetLocationKey.SelectedKeyName);
		}
		return EBTNodeResult::Succeeded;
	}

	// 检查体力
	UAttributesComponent* AttributesComp = EnemyChar->FindComponentByClass<UAttributesComponent>();
	if (!AttributesComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: No AttributesComponent!"));
		return EBTNodeResult::Failed;
	}

	float CurrentStamina = AttributesComp->GetStamina();
	if (CurrentStamina < 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: Not enough stamina! Need 1.0, have %.1f"), 
			CurrentStamina);
		return EBTNodeResult::Failed;
	}

	// 计算移动方向
	FVector DirectionToTarget = (TargetLocation - EnemyLocation).GetSafeNormal();
	UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Direction = %s"), *DirectionToTarget.ToString());

	int32 DeltaX = 0;
	int32 DeltaY = 0;

	// 四方向离散化
	if (FMath::Abs(DirectionToTarget.X) > FMath::Abs(DirectionToTarget.Y))
	{
		DeltaX = (DirectionToTarget.X > 0) ? 1 : -1;
		DeltaY = 0;
	}
	else
	{
		DeltaY = (DirectionToTarget.Y > 0) ? 1 : -1;
		DeltaX = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_MoveToGrid: Delta = (%d, %d)"), DeltaX, DeltaY);

	if (DeltaX == 0 && DeltaY == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: Delta is zero, already at target"));
		return EBTNodeResult::Succeeded;
	}

	// 尝试移动
	bool bMoveSuccess = GridMovementComp->TryMoveOneStep(DeltaX, DeltaY);
	
	if (bMoveSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_MoveToGrid: Move started successfully! Returning InProgress"));
		return EBTNodeResult::InProgress;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_MoveToGrid: Move FAILED! GridMovementComponent->TryMoveOneStep returned false"));
		
		// 移动失败，清除目标点
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