// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BehaviorTree || !BehaviorTree->BlackboardAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Controller for %s is missing BehaviorTree or BlackboardAsset!"), *InPawn->GetName());
		return;
	}

	// 初始化黑板
	UBlackboardComponent* BlackboardComp = BlackboardComponent.Get();
	if (UseBlackboard(BehaviorTree->BlackboardAsset, BlackboardComp))
	{
		BlackboardComponent = BlackboardComp;

		// 在初始化成功后，设置初始值，将AI的出生位置设为巡逻中心
		BlackboardComponent->SetValueAsVector("PatrolCenter", InPawn->GetActorLocation());
		// 也可以在这里设置其他初始值，例如 SelfActor
		BlackboardComponent->SetValueAsObject("SelfActor", InPawn);

		// 运行行为树
		if (!RunBehaviorTree(BehaviorTree))
		{
			UE_LOG(LogTemp, Error, TEXT("AI Controller for %s failed to RunBehaviorTree!"), *InPawn->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AI Controller for %s failed to UseBlackboard!"), *InPawn->GetName());
	}
}

