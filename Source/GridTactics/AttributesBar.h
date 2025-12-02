// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributesBar.generated.h"

class UAttributesComponent;

/**
 * 通用属性条 Widget
 * 可用于显示任何角色的 HP、MP、Stamina、Shield 等属性
 * 支持玩家、敌人、友军等所有拥有 AttributesComponent 的角色
 */
UCLASS()
class GRIDTACTICS_API UAttributesBar : public UUserWidget
{
    GENERATED_BODY()

public:
    // ========================================
    // 初始化与绑定
    // ========================================

    /** 设置要显示的 AttributesComponent（可从蓝图或 C++ 调用） */
    UFUNCTION(BlueprintCallable, Category = "Attributes Bar")
    void SetAttributesComponent(UAttributesComponent* InAttributesComponent);

    /** 获取当前绑定的 AttributesComponent */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar")
    UAttributesComponent* GetAttributesComponent() const { return AttributesComponent.Get(); }

    /** 设置拥有者角色（自动查找其 AttributesComponent） */
    UFUNCTION(BlueprintCallable, Category = "Attributes Bar")
    void SetOwnerActor(AActor* InOwnerActor);

    // ========================================
    // 属性获取接口（供蓝图绑定）
    // ========================================

    /** 获取 HP 百分比（0.0 - 1.0） */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Health")
    float GetHPPercent() const;

    /** 获取 HP 文本（例如 "85/100"） */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Health")
    FText GetHPText() const;

    /** 获取当前 HP */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Health")
    float GetCurrentHP() const;

    /** 获取最大 HP */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Health")
    float GetMaxHP() const;

    // --- Shield ---
    
    /** 获取 Shield */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Shield")
    float GetShield() const;

    /** 获取 Shield 文本 */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Shield")
    FText GetShieldText() const;

    /** 是否有护盾 */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Shield")
    bool HasShield() const;

    // --- MP ---

    /** 获取 MP 百分比（0.0 - 1.0） */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Mana")
    float GetMPPercent() const;

    /** 获取 MP 文本 */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Mana")
    FText GetMPText() const;

    // --- Stamina ---

    /** 获取 Stamina 百分比（0.0 - 1.0） */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Stamina")
    float GetStaminaPercent() const;

    /** 获取 Stamina 文本 */
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Stamina")
    FText GetStaminaText() const;

    // ========================================
    // 新增：HP + Shield 组合显示接口
    // ========================================

    /** 获取总生命值百分比（HP + Shield / MaxHP）*/
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Combined")
    float GetTotalHealthPercent() const;

    /** 获取纯 HP 部分的百分比（HP / (HP + Shield)）*/
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Combined")
    float GetHealthOnlyPercent() const;

    /** 获取 Shield 占总条的百分比（Shield / MaxHP）*/
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Combined")
    float GetShieldExtensionPercent() const;

    /** 获取组合文本（例如 "85+15/100"，表示 85 HP + 15 Shield）*/
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Combined")
    FText GetCombinedHealthText() const;

    /** 获取当前总生命值（HP + Shield）*/
    UFUNCTION(BlueprintPure, Category = "Attributes Bar|Combined")
    float GetTotalHealth() const;

    // ========================================
    // 可配置选项
    // ========================================
    
    /** 是否显示护盾条 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes Bar|Display")
    bool bShowShield = true;

    /** 是否显示 MP 条 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes Bar|Display")
    bool bShowMana = true;

    /** 是否显示 Stamina 条 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes Bar|Display")
    bool bShowStamina = false;

    /** 是否显示数值文本（如 "85/100"） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes Bar|Display")
    bool bShowNumericText = true;

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ========================================
    // 蓝图事件（用于自定义动画/特效）
    // ========================================

    /** 当 HP 变化时触发（参数：新百分比） */
    UFUNCTION(BlueprintImplementableEvent, Category = "Attributes Bar|Events")
    void OnHPChanged(float NewPercent);

    /** 当护盾变化时触发 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Attributes Bar|Events")
    void OnShieldChanged(float NewPercent);

    /** 当 MP 变化时触发 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Attributes Bar|Events")
    void OnMPChanged(float NewPercent);

    /** 当 Stamina 变化时触发 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Attributes Bar|Events")
    void OnStaminaChanged(float NewPercent);

    /** 当角色死亡时触发 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Attributes Bar|Events")
    void OnOwnerDied();

    /** 当 HP 低于警戒线时触发（默认 30%） */
    UFUNCTION(BlueprintImplementableEvent, Category = "Attributes Bar|Events")
    void OnLowHealth();

private:
    /** 绑定的 AttributesComponent */
    UPROPERTY()
    TWeakObjectPtr<UAttributesComponent> AttributesComponent;

    /** 缓存上一帧的属性值，用于检测变化 */
    float LastHPPercent = 1.0f;
    float LastShieldPercent = 0.0f;
    float LastMPPercent = 1.0f;
    float LastStaminaPercent = 1.0f;

    bool bWasLowHealth = false;
    bool bWasAlive = true;

    /** 低血量警戒线（百分比） */
    UPROPERTY(EditAnywhere, Category = "Attributes Bar|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LowHPThreshold = 0.3f;
};
