// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FindPatrolLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "GridTactics/EnemyCharacter.h"
#include "GridTactics/GridMovementComponent.h"

UBTTask_FindPatrolLocation::UBTTask_FindPatrolLocation()
{
	NodeName = "Find Patrol Grid Location";
}

EBTNodeResult::Type UBTTask_FindPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar) return EBTNodeResult::Failed;

	// 获取GridMovementComponent以使用其坐标转换功能
	UGridMovementComponent* GridMovementComp = EnemyChar->GetGridMovementComponent();
	if (!GridMovementComp) return EBTNodeResult::Failed;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return EBTNodeResult::Failed;

	// 获取巡逻中心点
	FVector PatrolCenter = BlackboardComp->GetValueAsVector(PatrolCenterKey.SelectedKeyName);

	// 在导航系统上，围绕巡逻中心点寻找一个随机的可达位置
	FNavLocation RandomPoint;
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys && NavSys->GetRandomReachablePointInRadius(PatrolCenter, PatrolRadius, RandomPoint))
	{
		int32 GridX, GridY;
		GridMovementComp->WorldToGrid(RandomPoint.Location, GridX, GridY);	// 将连续的世界坐标转换为网格坐标
		// 将网格坐标转换回位置对齐到网格的世界坐标
		FVector SnappedLocation = GridMovementComp->GridToWorld(GridX, GridY);

		BlackboardComp->SetValueAsVector(PatrolLocationKey.SelectedKeyName, SnappedLocation);

		UE_LOG(LogTemp, Log, TEXT("Found new patrol location. Random: %s -> Snapped: %s"), *RandomPoint.Location.ToString(), *SnappedLocation.ToString());
		return EBTNodeResult::Succeeded;
	}

	// 没找到，任务失败
	return EBTNodeResult::Failed;
}