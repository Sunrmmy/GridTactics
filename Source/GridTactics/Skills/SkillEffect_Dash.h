// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillEffect.h"
#include "SkillEffect_Dash.generated.h"

/**
 * 冲刺效果 - 将施法者向指定方向冲刺，并可击退路径上的敌人
 * 功能：
 * 1. 施法者向指定方向冲刺 N 格
 * 2. 碰撞到敌人时可选择：停止 / 穿透
 * 3. 被撞击的敌人会被击退
 * 4. 支持链式击退（敌人撞到敌人）
 */
UCLASS(Blueprintable, EditInlineNew, meta = (DisplayName = "Dash Effect"))
class GRIDTACTICS_API USkillEffect_Dash : public USkillEffect
{
    GENERATED_BODY()

public:
    USkillEffect_Dash();

    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors) override;
    virtual bool CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const override;

    // ========================================
    // 配置属性
    // ========================================

    /** 冲刺距离（格子数） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "1", ClampMax = "10"))
    int32 DashDistance = 4;

    /** 冲刺方向（相对于施法者朝向，通常从 TargetType::Direction 获取） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
    FIntPoint DashDirection = FIntPoint(1, 0);  // 默认向前

    /** 是否在碰到敌人时停止（false = 穿透） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
    bool bStopOnCollision = true;

    /** 击退距离（被撞击的敌人向后退几格） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Knockback", meta = (ClampMin = "0", ClampMax = "5"))
    int32 KnockbackDistance = 2;

    /** 是否启用链式击退（敌人A被击退撞到敌人B，B也会被击退） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Knockback")
    bool bEnableChainKnockback = false;

    /** 冲刺持续时间（秒，用于动画） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Animation", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float ChargeDuration = 0.3f;

    /** 碰撞伤害 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Damage", meta = (ClampMin = "0.0"))
    float CollisionDamage = 0.0f;

    /** 是否对撞击的敌人造成伤害 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Damage")
    bool bDamageCollidedEnemies = true;

protected:
    /** 获取冲刺方向（考虑施法者朝向） */
    FIntPoint GetActualDashDirection(AActor* Instigator) const;
};
