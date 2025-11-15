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

	if (BehaviorTree && BehaviorTree->BlackboardAsset)
	{
		// 使用一个临时的原始指针来调用UseBlackboard
		UBlackboardComponent* BlackboardComp = BlackboardComponent.Get();
		if (UseBlackboard(BehaviorTree->BlackboardAsset, BlackboardComp))
		{
			// 如果UseBlackboard成功，它可能已经修改了BlackboardComp，
			BlackboardComponent = BlackboardComp;	// 将结果安全地存回我们的TObjectPtr成员变量中。
			RunBehaviorTree(BehaviorTree);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AI Controller for %s failed to UseBlackboard!"), *InPawn->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AI Controller for %s is missing BehaviorTree or BlackboardAsset!"), *InPawn->GetName());
	}
}

