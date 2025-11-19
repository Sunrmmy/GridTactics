// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FindPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UBTTask_FindPlayer::UBTTask_FindPlayer()
{
    NodeName = "Find Player";
}

EBTNodeResult::Type UBTTask_FindPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取玩家角色
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (PlayerCharacter)
    {
        // 将找到的玩家写入黑板
        OwnerComp.GetBlackboardComponent()->SetValueAsObject(TargetPlayerKey.SelectedKeyName, PlayerCharacter);
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}
