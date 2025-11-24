// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridTactics/BaseSkill.h"
#include "Skill_Charge.generated.h"

/**
 * 冲锋技能：向指定方向冲刺，撞击敌人并击退
 * 完全使用 SkillDataAsset 配置，不再硬编码属性
 */
UCLASS()
class GRIDTACTICS_API USkill_Charge : public UBaseSkill
{
    GENERATED_BODY()

public:
    USkill_Charge();

    virtual bool CanActivate_Implementation() override;
    virtual void Activate_Implementation() override;

private:
    // --- 辅助函数 ---

    /** 获取 GridManager */
    class AGridManager* GetGridManager() const;

    /** 根据角色朝向或鼠标方向计算离散的四方向 */
    FIntPoint GetChargeDirection() const;

    /** 计算基于角色当前朝向的冲刺方向 */
    FIntPoint GetDirectionFromRotation() const;

    /** 计算基于鼠标位置的冲刺方向 */
    FIntPoint GetDirectionFromMouse() const;

    /** 对碰撞的敌人造成伤害 */
    void ApplyCollisionDamage(AActor* HitActor) const;
};