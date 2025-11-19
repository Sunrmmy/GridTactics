// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UseSkill.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API UBTTask_UseSkill : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
    UBTTask_UseSkill();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 要使用的技能索引
    UPROPERTY(EditAnywhere, Category = "Skill")
    int32 SkillIndex = 0;
};
