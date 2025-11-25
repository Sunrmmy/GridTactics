// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridTactics/BaseSkill.h"
#include "Skill_TargetedEffect.generated.h"

/**
 * 通用精确目标技能基类
 * 支持：AOE 伤害、AOE 治疗、精确传送等
 */
UCLASS()
class GRIDTACTICS_API USkill_TargetedEffect : public UBaseSkill
{
    GENERATED_BODY()

public:
    USkill_TargetedEffect();

    virtual bool CanActivate_Implementation() override;
    virtual void Activate_Implementation() override;

protected:
    // 获取鼠标指向的目标格子
    FIntPoint GetMouseTargetGrid() const;

    // 验证目标格子是否在施法范围内
    bool IsTargetInRange(FIntPoint TargetGrid) const;

    // 获取效果影响的所有格子（基于 EffectPattern）
    TArray<FIntPoint> GetEffectGrids(FIntPoint TargetGrid) const;

    // 获取效果范围内的所有角色
    TArray<AActor*> GetActorsInEffect(FIntPoint TargetGrid) const;

    // 执行技能效果（由子类重写）
    virtual void ExecuteEffect(FIntPoint TargetGrid);

    // 对单个目标应用效果
    virtual void ApplyEffectToActor(AActor* Target);

private:
    class AGridManager* GetGridManager() const;
};
