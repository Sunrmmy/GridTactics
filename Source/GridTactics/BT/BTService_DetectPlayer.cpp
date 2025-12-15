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
	
	UE_LOG(LogTemp, Log, TEXT("BTService_DetectPlayer: Constructor called"));
}

void UBTService_DetectPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		UE_LOG(LogTemp, Error, TEXT("BTService_DetectPlayer: No AI Controller or Pawn"));
		return;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTService_DetectPlayer: No Blackboard Component"));
		return;
	}

	// 尝试获取玩家角色
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTService_DetectPlayer: No player character found in world"));
		BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
		return;
	}

	FVector AILocation = AIController->GetPawn()->GetActorLocation();
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	float Distance = FVector::Dist(AILocation, PlayerLocation);

	// 检查玩家是否在索敌范围内
	if (Distance <= DetectionRadius)
	{
		BlackboardComp->SetValueAsObject(TargetActorKey.SelectedKeyName, PlayerCharacter);
		
		UE_LOG(LogTemp, Log, TEXT("BTService_DetectPlayer: Player detected! Distance: %.1f / %.1f"), 
			Distance, DetectionRadius);
		UE_LOG(LogTemp, Log, TEXT("  - AI: %s at %s"), 
			*AIController->GetPawn()->GetName(), *AILocation.ToString());
		UE_LOG(LogTemp, Log, TEXT("  - Player: %s at %s"), 
			*PlayerCharacter->GetName(), *PlayerLocation.ToString());
	}
	else
	{
		BlackboardComp->ClearValue(TargetActorKey.SelectedKeyName);
		
		UE_LOG(LogTemp, Verbose, TEXT("BTService_DetectPlayer: Player out of range. Distance: %.1f / %.1f"), 
			Distance, DetectionRadius);
	}
}