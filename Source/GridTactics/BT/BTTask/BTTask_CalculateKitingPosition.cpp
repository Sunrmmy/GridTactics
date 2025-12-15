// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_CalculateKitingPosition.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GridTactics/EnemyCharacter.h"
#include "GridTactics/GridMovement/GridMovementComponent.h"
#include "GridTactics/GridMovement/GridManager.h"
#include "GridTactics/AttributesComponent.h"
#include "Kismet/GameplayStatics.h"

UBTTask_CalculateKitingPosition::UBTTask_CalculateKitingPosition()
{
	NodeName = "Calculate Kiting Position";
	
	// 设置默认黑板键名
	TargetActorKey.SelectedKeyName = FName("TargetPlayer");
	KitingPositionKey.SelectedKeyName = FName("KitingPosition");
}

EBTNodeResult::Type UBTTask_CalculateKitingPosition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("========== BTTask_CalculateKitingPosition: ExecuteTask START =========="));
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_CalculateKitingPosition: No AIController!"));
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_CalculateKitingPosition: No EnemyCharacter!"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_CalculateKitingPosition: Enemy = %s"), *EnemyChar->GetName());

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_CalculateKitingPosition: No Blackboard!"));
		return EBTNodeResult::Failed;
	}

	// 获取玩家目标
	AActor* TargetPlayer = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_CalculateKitingPosition: No TargetPlayer in Blackboard!"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_CalculateKitingPosition: Target = %s"), *TargetPlayer->GetName());

	// 获取 GridManager
	AGridManager* GridMgr = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
	);
	if (!GridMgr)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_CalculateKitingPosition: No GridManager in world!"));
		return EBTNodeResult::Failed;
	}

	// 获取当前网格坐标
	FIntPoint EnemyGrid = GridMgr->GetActorCurrentGrid(EnemyChar);
	FIntPoint PlayerGrid = GridMgr->GetActorCurrentGrid(TargetPlayer);

	UE_LOG(LogTemp, Log, TEXT("BTTask_CalculateKitingPosition: EnemyGrid = %s, PlayerGrid = %s"), 
		*EnemyGrid.ToString(), *PlayerGrid.ToString());

	// 检查体力
	UAttributesComponent* AttributesComp = EnemyChar->FindComponentByClass<UAttributesComponent>();
	if (!AttributesComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_CalculateKitingPosition: No AttributesComponent!"));
		return EBTNodeResult::Failed;
	}

	float CurrentStamina = AttributesComp->GetStamina();
	UE_LOG(LogTemp, Log, TEXT("BTTask_CalculateKitingPosition: Current Stamina = %.1f"), CurrentStamina);

	if (CurrentStamina < 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_CalculateKitingPosition: Not enough stamina! Need 1.0, have %.1f"), 
			CurrentStamina);
		BlackboardComp->ClearValue(KitingPositionKey.SelectedKeyName);
		return EBTNodeResult::Failed;
	}

	// 计算当前距离
	int32 CurrentDistance = GetGridDistance(EnemyGrid, PlayerGrid);
	int32 MinDistanceGrids = FMath::RoundToInt(MinDesiredDistance / 100.0f);
	int32 MaxDistanceGrids = FMath::RoundToInt(MaxDesiredDistance / 100.0f);

	UE_LOG(LogTemp, Log, TEXT("BTTask_CalculateKitingPosition: Distance = %d, Range = [%d, %d]"), 
		CurrentDistance, MinDistanceGrids, MaxDistanceGrids);

	// 决策：接近 or 远离 or 横向移动
	TArray<FIntPoint> CandidatePositions;
	
	if (CurrentDistance < MinDistanceGrids)
	{
		// 太近了，需要后退
		UE_LOG(LogTemp, Log, TEXT("AI: Too close (%d grids), retreating..."), CurrentDistance);
		
		// 计算远离玩家的方向
		FIntPoint AwayDirection = EnemyGrid - PlayerGrid;
		AwayDirection.X = FMath::Clamp(AwayDirection.X, -1, 1);
		AwayDirection.Y = FMath::Clamp(AwayDirection.Y, -1, 1);
		
		// 只保留主要方向（四向）
		if (FMath::Abs(AwayDirection.X) > FMath::Abs(AwayDirection.Y))
		{
			AwayDirection.Y = 0;
		}
		else
		{
			AwayDirection.X = 0;
		}
		
		// 生成后退候选点
		for (int32 Step = 1; Step <= MaxMoveSteps; ++Step)
		{
			FIntPoint Candidate = EnemyGrid + (AwayDirection * Step);
			CandidatePositions.Add(Candidate);
		}
		
		// 添加侧向移动候选点（增加随机性）
		FIntPoint Perpendicular1 = FIntPoint(-AwayDirection.Y, AwayDirection.X);
		FIntPoint Perpendicular2 = FIntPoint(AwayDirection.Y, -AwayDirection.X);
		CandidatePositions.Add(EnemyGrid + Perpendicular1);
		CandidatePositions.Add(EnemyGrid + Perpendicular2);
	}
	else if (CurrentDistance > MaxDistanceGrids)
	{
		// 太远了，需要接近
		UE_LOG(LogTemp, Log, TEXT("AI: Too far (%d grids), approaching..."), CurrentDistance);
		
		// 计算接近玩家的方向
		FIntPoint TowardDirection = PlayerGrid - EnemyGrid;
		TowardDirection.X = FMath::Clamp(TowardDirection.X, -1, 1);
		TowardDirection.Y = FMath::Clamp(TowardDirection.Y, -1, 1);
		
		// 只保留主要方向（四向）
		if (FMath::Abs(TowardDirection.X) > FMath::Abs(TowardDirection.Y))
		{
			TowardDirection.Y = 0;
		}
		else
		{
			TowardDirection.X = 0;
		}
		
		// 生成接近候选点
		for (int32 Step = 1; Step <= MaxMoveSteps; ++Step)
		{
			FIntPoint Candidate = EnemyGrid + (TowardDirection * Step);
			CandidatePositions.Add(Candidate);
		}
	}
	else
	{
		// 距离合适，横向移动（拉扯）
		UE_LOG(LogTemp, Log, TEXT("AI: Distance OK (%d grids), strafing..."), CurrentDistance);
		
		// 计算垂直于玩家方向的两个侧向
		FIntPoint TowardPlayer = PlayerGrid - EnemyGrid;
		FIntPoint Perpendicular1 = FIntPoint(-TowardPlayer.Y, TowardPlayer.X);
		FIntPoint Perpendicular2 = FIntPoint(TowardPlayer.Y, -TowardPlayer.X);
		
		// 归一化为单位方向
		if (Perpendicular1.X != 0) Perpendicular1.X = FMath::Sign(Perpendicular1.X);
		if (Perpendicular1.Y != 0) Perpendicular1.Y = FMath::Sign(Perpendicular1.Y);
		if (Perpendicular2.X != 0) Perpendicular2.X = FMath::Sign(Perpendicular2.X);
		if (Perpendicular2.Y != 0) Perpendicular2.Y = FMath::Sign(Perpendicular2.Y);
		
		// 生成横向移动候选点
		for (int32 Step = 1; Step <= MaxMoveSteps; ++Step)
		{
			CandidatePositions.Add(EnemyGrid + (Perpendicular1 * Step));
			CandidatePositions.Add(EnemyGrid + (Perpendicular2 * Step));
		}
		
		// 添加一些随机偏移（增加不可预测性）
		TArray<FIntPoint> RandomOffsets = {
			FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)
		};
		for (const FIntPoint& Offset : RandomOffsets)
		{
			CandidatePositions.Add(EnemyGrid + Offset);
		}
	}

	// 过滤有效候选点
	TArray<FIntPoint> ValidPositions;
	for (const FIntPoint& Candidate : CandidatePositions)
	{
		if (IsPositionValid(GridMgr, Candidate, EnemyChar))
		{
			ValidPositions.Add(Candidate);
		}
	}

	if (ValidPositions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AI: No valid kiting positions found"));
		return EBTNodeResult::Failed;
	}

	// 随机选择一个有效位置（增加随机性）
	int32 RandomIndex = FMath::RandRange(0, ValidPositions.Num() - 1);
	FIntPoint SelectedGrid = ValidPositions[RandomIndex];
	FVector SelectedWorldPos = GridMgr->GridToWorld(SelectedGrid);

	// 写入黑板
	BlackboardComp->SetValueAsVector(KitingPositionKey.SelectedKeyName, SelectedWorldPos);

	UE_LOG(LogTemp, Warning, TEXT("BTTask_CalculateKitingPosition: SUCCESS! Selected Grid(%d, %d) at World %s"), 
		SelectedGrid.X, SelectedGrid.Y, *SelectedWorldPos.ToString());
	UE_LOG(LogTemp, Warning, TEXT("========== BTTask_CalculateKitingPosition: ExecuteTask END =========="));

	return EBTNodeResult::Succeeded;
}

bool UBTTask_CalculateKitingPosition::IsPositionValid(AGridManager* GridMgr, FIntPoint GridPos, AActor* SelfActor) const
{
	if (!GridMgr) return false;

	// 检查边界
	if (!GridMgr->IsGridValid(GridPos))
	{
		return false;
	}

	// 检查可行走性
	if (!GridMgr->IsGridWalkable(GridPos))
	{
		return false;
	}

	// 检查是否被其他角色占据
	AActor* OccupyingActor = GridMgr->GetActorAtGrid(GridPos);
	if (OccupyingActor && OccupyingActor != SelfActor)
	{
		return false;
	}

	return true;
}

int32 UBTTask_CalculateKitingPosition::GetGridDistance(FIntPoint A, FIntPoint B) const
{
	// 曼哈顿距离（四向移动）
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

