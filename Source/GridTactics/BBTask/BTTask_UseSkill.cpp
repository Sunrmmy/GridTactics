// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_UseSkill.h"
#include "AIController.h"
#include "../EnemyCharacter.h"
#include "../SkillComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_UseSkill::UBTTask_UseSkill()
{
    NodeName = "Use Skill";
}

EBTNodeResult::Type UBTTask_UseSkill::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return EBTNodeResult::Failed;

    AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
    if (!EnemyChar) return EBTNodeResult::Failed;

    USkillComponent* SkillComp = EnemyChar->GetSkillComponent();
    if (SkillComp)
    {
        // 设置朝向：让AI朝向目标玩家
        AActor* TargetPlayer = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject("TargetPlayer"));
        if (TargetPlayer)
        {
            FVector DirectionToTarget = (TargetPlayer->GetActorLocation() - EnemyChar->GetActorLocation()).GetSafeNormal();
            EnemyChar->SetActorRotation(DirectionToTarget.Rotation());
        }

        // 尝试使用技能 (这里简化为直接激活，您可以扩展为先瞄准再确认)
        //if (SkillComp->TryStartAiming(SkillIndex))
        //{
        //    SkillComp->TryConfirmSkill();
        //    return EBTNodeResult::Succeeded;
        //}
		return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}