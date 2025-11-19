// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_DetectPlayer.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UBTService_DetectPlayer::UBTService_DetectPlayer()
{
	NodeName = "Detect Player";
	// 设置服务每0.5秒运行一次，不需要每帧都检测
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService_DetectPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn()) return;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return;

	// 尝试获取玩家角色
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter)
	{
		// 如果世界中没有玩家，清空目标
		BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
		return;
	}

	// 检查玩家是否在索敌范围内
	if (FVector::Dist(AIController->GetPawn()->GetActorLocation(), PlayerCharacter->GetActorLocation()) <= DetectionRadius)
	{
		// 在范围内，将玩家设置为目标
		BlackboardComp->SetValueAsObject(TargetActorKey.SelectedKeyName, PlayerCharacter);
	}
	else
	{
		// 不在范围内，清空目标，AI将“失去兴趣”
		BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
	}
}