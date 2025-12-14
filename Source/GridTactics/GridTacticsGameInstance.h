// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GridTacticsGameInstance.generated.h"

class USkillDataAsset;

/**
 * 游戏实例
 * 仅用于跨关卡传递准备阶段选择的技能
 */
UCLASS()
class GRIDTACTICS_API UGridTacticsGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UGridTacticsGameInstance();

    // ========================================
    // 准备阶段技能传递（唯一需要的功能）
    // ========================================

    /** 玩家在准备阶段选择的技能列表 */
    UPROPERTY(BlueprintReadWrite, Category = "Game Instance")
    TArray<TObjectPtr<USkillDataAsset>> PendingSkills;

    /** 清空技能列表 */
    UFUNCTION(BlueprintCallable, Category = "Game Instance")
    void ClearPendingSkills() { PendingSkills.Empty(); }

    /** 添加技能到待选列表 */
    UFUNCTION(BlueprintCallable, Category = "Game Instance")
    void AddPendingSkill(USkillDataAsset* Skill);

    /** 获取待选技能数量 */
    UFUNCTION(BlueprintPure, Category = "Game Instance")
    int32 GetPendingSkillCount() const { return PendingSkills.Num(); }
};
