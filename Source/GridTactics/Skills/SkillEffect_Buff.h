// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillEffect.h"
#include "GridTactics/GridTacticsPlayerState.h"
#include "SkillEffect_Buff.generated.h"

/**
 * Buff 效果类型
 */
UENUM(BlueprintType)
enum class EBuffEffectType : uint8
{
    Buff        UMETA(DisplayName = "增益 (Buff)"),
    Debuff      UMETA(DisplayName = "减益 (Debuff)"),
    Neutral     UMETA(DisplayName = "中性 (Neutral)")
};

/**
 * 目标过滤类型
 */
UENUM(BlueprintType)
enum class EBuffTargetFilter : uint8
{
    All         UMETA(DisplayName = "所有角色"),
    Allies      UMETA(DisplayName = "仅友军"),
    Enemies     UMETA(DisplayName = "仅敌军"),
    Self        UMETA(DisplayName = "仅自己")
};

/**
 * Buff/Debuff 效果 - 为目标添加属性修改器
 * 
 * 功能：
 * - 支持多种属性修改（HP、MP、护甲、移速等）
 * - 支持持续时间和永久 Buff
 * - 支持目标过滤（友军/敌军）
 * - 支持堆叠机制
 */
UCLASS(Blueprintable, EditInlineNew, meta = (DisplayName = "Buff Effect"))
class GRIDTACTICS_API USkillEffect_Buff : public USkillEffect
{
	GENERATED_BODY()
	
public:
    USkillEffect_Buff();

    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors) override;

    // ========================================
    // 核心配置
    // ========================================

    /** Buff 类型（用于 UI 显示和逻辑区分） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Core")
    EBuffEffectType BuffType = EBuffEffectType::Buff;

    /** 要应用的属性修改器列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Core", meta = (TitleProperty = "AttributeToModify"))
    TArray<FAttributeModifier> Modifiers;

    /** Buff 名称（用于调试和 UI 显示） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Core")
    FText BuffName = FText::FromString(TEXT("Unnamed Buff"));

    /** Buff 描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Core", meta = (MultiLine = true))
    FText BuffDescription;

    // ========================================
    // 目标过滤
    // ========================================

    /** 目标过滤类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Target")
    EBuffTargetFilter TargetFilter = EBuffTargetFilter::Allies;

    /** 是否对施法者自己应用（当 TargetFilter = Allies 时生效） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Target", 
        meta = (EditCondition = "TargetFilter == EBuffTargetFilter::Allies"))
    bool bApplyToSelf = true;

    // ========================================
    // 堆叠机制
    // ========================================

    /** 是否允许堆叠 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Stacking")
    bool bAllowStacking = false;

    /** 最大堆叠层数（仅当 bAllowStacking = true 时生效） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Stacking", 
        meta = (EditCondition = "bAllowStacking", ClampMin = "1", ClampMax = "99"))
    int32 MaxStackCount = 5;

    /** 重复应用时是否刷新持续时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Stacking")
    bool bRefreshDurationOnReapply = true;

    // ========================================
    // 视觉效果（可选）
    // ========================================

    ///** Buff 应用时的音效 */
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Feedback")
    //TObjectPtr<USoundBase> BuffApplySound;

    ///** Buff 应用时的粒子特效 */
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Feedback")
    //TObjectPtr<UParticleSystem> BuffApplyVFX;

    ///** 特效附着在角色身上的 Socket 名称 */
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Feedback")
    //FName VFXAttachSocketName = NAME_None;

protected:
    /**
     * 检查目标是否符合过滤条件
     */
    bool IsValidTarget(AActor* Target, AActor* Instigator) const;

    /**
     * 检查目标是否已有相同的 Buff（用于堆叠逻辑）
     */
    bool HasSameBuff(class UAttributesComponent* TargetAttrs, const FAttributeModifier& Modifier) const;

    /**
     * 播放视觉反馈（音效和特效）
     */
    //void PlayFeedback(AActor* Target);
};
