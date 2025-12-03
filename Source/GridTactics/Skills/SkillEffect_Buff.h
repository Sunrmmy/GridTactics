// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillEffect.h"
#include "GridTactics/GridTacticsPlayerState.h"
#include "SkillEffect_Buff.generated.h"

/**
 * Buff/Debuff 效果 - 为目标添加属性修改器
 */
UCLASS(Blueprintable, EditInlineNew, meta = (DisplayName = "Buff Effect"))
class GRIDTACTICS_API USkillEffect_Buff : public USkillEffect
{
	GENERATED_BODY()
	
public:
    USkillEffect_Buff();

    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors) override;

    // ========================================
    // 配置属性
    // ========================================

    /** 要应用的 Buff/Debuff 列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    TArray<FAttributeModifier> Modifiers;

    /** 是否对施法者自己应用 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
    bool bApplyToSelf = false;
};
