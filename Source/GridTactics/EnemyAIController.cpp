// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	
	UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: Constructor called"));
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: OnPossess called for %s"), 
		InPawn ? *InPawn->GetName() : TEXT("NULL"));

	if (!BehaviorTree)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAIController: BehaviorTree is NULL! Please set it in BP_EnemyAIController."));
		return;
	}

	if (!BehaviorTree->BlackboardAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAIController: BlackboardAsset is NULL in BehaviorTree!"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("EnemyAIController: BehaviorTree and Blackboard are valid"));

	// 初始化黑板
	UBlackboardComponent* BlackboardComp = BlackboardComponent.Get();
	if (UseBlackboard(BehaviorTree->BlackboardAsset, BlackboardComp))
	{
		BlackboardComponent = BlackboardComp;

		// 设置初始值
		BlackboardComponent->SetValueAsVector("PatrolCenter", InPawn->GetActorLocation());
		BlackboardComponent->SetValueAsObject("SelfActor", InPawn);

		UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: Blackboard initialized successfully"));
		UE_LOG(LogTemp, Log, TEXT("  - PatrolCenter: %s"), *InPawn->GetActorLocation().ToString());

		// 运行行为树
		if (RunBehaviorTree(BehaviorTree))
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: BehaviorTree started successfully for %s"), 
				*InPawn->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("EnemyAIController: Failed to run BehaviorTree for %s"), 
				*InPawn->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAIController: Failed to UseBlackboard for %s"), 
			*InPawn->GetName());
	}
}

