// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class AGridTacticsPlayerState;
class AHeroCharacter;
class UAttributesComponent;
class USkillComponent;
class USkillDataAsset;
UCLASS()
class GRIDTACTICS_API UHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ========================================
    // 设置拥有者角色（在生成 HUD 时调用）
    // ========================================

    /** 设置 HUD 的拥有者角色 */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetOwnerCharacter(AHeroCharacter* InOwnerCharacter);

    // ========================================
    // 属性访问器（供蓝图使用）
    // ========================================

    /** 获取当前 HP */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetCurrentHP() const;

    /** 获取最大 HP */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetMaxHP() const;

    /** 获取当前 MP */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetCurrentMP() const;

    /** 获取最大 MP */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetMaxMP() const;

    /** 获取当前 Stamina */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetCurrentStamina() const;

    /** 获取最大 Stamina */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetMaxStamina() const;

    /** 获取当前 Shield */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetCurrentShield() const;

    /** 获取最大 Shield */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetMaxShield() const;

    /** 获取 HP 文本（保留1位小数） */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    FText GetHPText() const;

    /** 获取 MP 文本（保留1位小数） */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    FText GetMPText() const;

    /** 获取 Stamina 文本（保留1位小数） */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    FText GetStaminaText() const;

    /** 获取 Shield 文本（保留1位小数） */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    FText GetShieldText() const;

    // ========================================
    // 百分比计算（方便进度条）
    // ========================================

    /** 获取 HP 百分比 (0-1) */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetHPPercent() const;

    /** 获取 MP 百分比 (0-1) */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetMPPercent() const;

    /** 获取 Stamina 百分比 (0-1) */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetStaminaPercent() const;

    /** 获取 Shield 百分比 (0-1) */
    UFUNCTION(BlueprintPure, Category = "UI|Attributes")
    float GetShieldPercent() const;

    // ========================================
    // HP + Shield 组合显示接口（与 AttributesBar 一致）
    // ========================================

    /** 获取纯 HP 部分的百分比（HP / MaxHP），用于 HP 条主体 */
    UFUNCTION(BlueprintPure, Category = "UI|Combined Health")
    float GetHealthOnlyPercent() const;

    /** 获取 Shield 占 MaxHP 的百分比（Shield / MaxHP），用于 Shield 延伸条 */
    UFUNCTION(BlueprintPure, Category = "UI|Combined Health")
    float GetShieldExtensionPercent() const;

    /** 获取总生命值百分比（(HP + Shield) / MaxHP），可能超过 1.0 */
    UFUNCTION(BlueprintPure, Category = "UI|Combined Health")
    float GetTotalHealthPercent() const;

    /** 获取组合文本（例如 "85+15/100" 或 "85/100"） */
    UFUNCTION(BlueprintPure, Category = "UI|Combined Health")
    FText GetCombinedHealthText() const;

    /** 获取当前总生命值（HP + Shield） */
    UFUNCTION(BlueprintPure, Category = "UI|Combined Health")
    float GetTotalHealth() const;

    /** 是否有护盾 */
    UFUNCTION(BlueprintPure, Category = "UI|Combined Health")
    bool HasShield() const;

    // ========================================
    // 技能信息访问
    // ========================================

    /** 获取技能组件 */
    UFUNCTION(BlueprintPure, Category = "UI|Skills")
    USkillComponent* GetSkillComponent() const;

    /** 刷新技能槽显示 */
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UI|Skills")
    void RefreshSkillSlots();

    // 技能冷却相关函数

    /** 获取指定技能槽的冷却剩余时间（秒） */
    UFUNCTION(BlueprintPure, Category = "UI|Skills")
    float GetSkillCooldownRemaining(int32 SkillIndex) const;

    /** 获取指定技能槽的冷却百分比（0-1，用于进度条） */
    UFUNCTION(BlueprintPure, Category = "UI|Skills")
    float GetSkillCooldownPercent(int32 SkillIndex) const;

    /** 检查指定技能槽是否在冷却中 */
    UFUNCTION(BlueprintPure, Category = "UI|Skills")
    bool IsSkillOnCooldown(int32 SkillIndex) const;

    /** 获取指定技能槽的冷却时间文本（如 "3.2s"） */
    UFUNCTION(BlueprintPure, Category = "UI|Skills")
    FText GetSkillCooldownText(int32 SkillIndex) const;

protected:
    /** 蓝图事件：通知更新 UI */
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnUpdatePlayerState();

    /** 获取 PlayerState */
    UFUNCTION(BlueprintPure, Category = "UI")
    AGridTacticsPlayerState* GetGridTacticsPlayerState() const;

    /** 每帧更新 */
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    /** 缓存的 PlayerState */
    UPROPERTY()
    TWeakObjectPtr<AGridTacticsPlayerState> CachedPlayerState;

    /** 缓存的拥有者角色 */
    UPROPERTY()
    TWeakObjectPtr<AHeroCharacter> OwnerCharacter;

    /** 缓存的属性组件 */
    UPROPERTY()
    TWeakObjectPtr<UAttributesComponent> CachedAttributesComponent;

    /** 缓存的技能组件 */
    UPROPERTY()
    TWeakObjectPtr<USkillComponent> CachedSkillComponent;

    /** 技能变更回调 */
    UFUNCTION()
    void OnSkillChanged(int32 SlotIndex);

    UFUNCTION()
    void OnSkillReplaced(int32 SlotIndex, USkillDataAsset* NewSkillData);
};
