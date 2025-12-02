// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillEffect.h"
#include "SkillEffect_Damage.generated.h"

/**
 * 伤害效果 - 对目标造成伤害
 */
UCLASS(Blueprintable, EditInlineNew, meta = (DisplayName = "Damage Effect"))
class GRIDTACTICS_API USkillEffect_Damage : public USkillEffect
{
    GENERATED_BODY()

public:
    USkillEffect_Damage();

    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors) override;

    // ========================================
    // 配置属性
    // ========================================

    /** 基础伤害值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0"))
    float BaseDamage = 10.0f;

    /** 是否对施法者自己造成伤害 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    bool bDamageSelf = false;

    /** 伤害衰减：根据距离减少伤害（0 = 无衰减，1 = 边缘无伤害） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DistanceFalloff = 0.0f;
};