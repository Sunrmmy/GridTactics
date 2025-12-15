// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_RotateToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GridTactics/EnemyCharacter.h"
#include "GridTactics/GridMovement/GridMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_RotateToTarget::UBTTask_RotateToTarget()
{
	NodeName = "Rotate To Target";
	TargetActorKey.SelectedKeyName = FName("TargetPlayer");
}

EBTNodeResult::Type UBTTask_RotateToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar) return EBTNodeResult::Failed;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return EBTNodeResult::Failed;

	// 获取目标
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_RotateToTarget: No target actor!"));
		return EBTNodeResult::Failed;
	}

	// 计算方向
	FVector DirectionToTarget = (TargetActor->GetActorLocation() - EnemyChar->GetActorLocation()).GetSafeNormal();
	if (DirectionToTarget.IsNearlyZero())
	{
		return EBTNodeResult::Succeeded;
	}

	FRotator TargetRotation = DirectionToTarget.Rotation();
	
	// 对齐到四个方向
	float Yaw = TargetRotation.Yaw;
	float SnappedYaw;
	
	if (Yaw >= -45.0f && Yaw < 45.0f)
	{
		SnappedYaw = 0.0f;   // 东（X+）
	}
	else if (Yaw >= 45.0f && Yaw < 135.0f)
	{
		SnappedYaw = 90.0f;  // 北（Y+）
	}
	else if (Yaw >= 135.0f || Yaw < -135.0f)
	{
		SnappedYaw = 180.0f; // 西（X-）
	}
	else
	{
		SnappedYaw = -90.0f;
	}
	
	FRotator SnappedRotation(0, SnappedYaw, 0);
	
	// 同时设置Actor旋转和GridMovementComponent的TargetRotation
	EnemyChar->SetActorRotation(SnappedRotation);
	
	// 同步更新 GridMovementComponent 的 TargetRotation
	if (UGridMovementComponent* MovementComp = EnemyChar->GetGridMovementComponent())
	{
		MovementComp->SetTargetRotation(SnappedRotation);
		UE_LOG(LogTemp, Log, TEXT("BTTask_RotateToTarget: Updated TargetRotation to %.1f°"), SnappedYaw);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("BTTask_RotateToTarget: %s rotated to %.1f° (facing %s)"), 
		*EnemyChar->GetName(), SnappedYaw, *TargetActor->GetName());

	return EBTNodeResult::Succeeded;
}

