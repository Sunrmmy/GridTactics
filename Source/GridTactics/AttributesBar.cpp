// Fill out your copyright notice in the Description page of Project Settings.

#include "AttributesBar.h"
#include "AttributesComponent.h"

// ========================================
// 初始化与绑定
// ========================================

void UAttributesBar::SetAttributesComponent(UAttributesComponent* InAttributesComponent)
{
    AttributesComponent = InAttributesComponent;

    if (AttributesComponent.IsValid())
    {
        // 初始化缓存值
        LastHPPercent = GetHPPercent();
        LastMPPercent = GetMPPercent();
        LastShieldPercent = GetShield();
        LastStaminaPercent = GetStaminaPercent();

        bWasLowHealth = (LastHPPercent <= LowHPThreshold);
        bWasAlive = (LastHPPercent > 0.0f);
    }
}

void UAttributesBar::SetOwnerActor(AActor* InOwnerActor)
{
    if (!InOwnerActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("AttributesBar: SetOwnerActor called with null actor"));
        return;
    }

    UAttributesComponent* Comp = InOwnerActor->FindComponentByClass<UAttributesComponent>();
    if (Comp)
    {
        SetAttributesComponent(Comp);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AttributesBar: Actor %s has no AttributesComponent"), *InOwnerActor->GetName());
    }
}

// ========================================
// Health
// ========================================

float UAttributesBar::GetHPPercent() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxHP = AttributesComponent->GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(AttributesComponent->GetHP() / MaxHP, 0.0f, 1.0f);
}

FText UAttributesBar::GetHPText() const
{
    if (!AttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    float CurrentHP = AttributesComponent->GetHP();
    float MaxHP = AttributesComponent->GetMaxHP();

    return FText::Format(
        FText::FromString(TEXT("{0}/{1}")),
        FText::AsNumber(FMath::RoundToInt(CurrentHP)),
        FText::AsNumber(FMath::RoundToInt(MaxHP))
    );
}

float UAttributesBar::GetCurrentHP() const
{
    return AttributesComponent.IsValid() ? AttributesComponent->GetHP() : 0.0f;
}

float UAttributesBar::GetMaxHP() const
{
    return AttributesComponent.IsValid() ? AttributesComponent->GetMaxHP() : 100.0f;
}

// ========================================
// Shield
// ========================================

float UAttributesBar::GetShield() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }
    // 直接返回 Shield 数值（而不是百分比）
    return AttributesComponent->GetShield();
}

FText UAttributesBar::GetShieldText() const
{
    if (!AttributesComponent.IsValid())
    {
        return FText::GetEmpty();
    }

    float CurrentShield = AttributesComponent->GetShield();

    // 仅显示 Shield 数值
    if (CurrentShield > 0.0f)
    {
        return FText::Format(
            FText::FromString(TEXT("+{0}")),
            FText::AsNumber(FMath::RoundToInt(CurrentShield))
        );
    }
    else
    {
        return FText::GetEmpty(); // 无护盾时不显示
    }
}

bool UAttributesBar::HasShield() const
{
    return AttributesComponent.IsValid() && AttributesComponent->GetShield() > 0.0f;
}

// ========================================
// HP + Shield 组合显示
// ========================================

float UAttributesBar::GetTotalHealthPercent() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxHP = AttributesComponent->GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }

    // 总生命值 = HP + Shield
    float TotalHealth = AttributesComponent->GetHP() + AttributesComponent->GetShield();
    return FMath::Clamp(TotalHealth / MaxHP, 0.0f, 2.0f); // 允许超过 100%（Shield 可能让总值 > MaxHP）
}

float UAttributesBar::GetHealthOnlyPercent() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxHP = AttributesComponent->GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }

    // 仅 HP 的百分比
    return FMath::Clamp(AttributesComponent->GetHP() / MaxHP, 0.0f, 1.0f);
}

float UAttributesBar::GetShieldExtensionPercent() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxHP = AttributesComponent->GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }

    // Shield 占 MaxHP 的比例
    return FMath::Clamp(AttributesComponent->GetShield() / MaxHP, 0.0f, 1.0f);
}

FText UAttributesBar::GetCombinedHealthText() const
{
    if (!AttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    float CurrentHP = AttributesComponent->GetHP();
    float CurrentShield = AttributesComponent->GetShield();
    float MaxHP = AttributesComponent->GetMaxHP();

    if (CurrentShield > 0.0f)
    {
        // 格式：85+15/100（HP + Shield / MaxHP）
        return FText::Format(
            FText::FromString(TEXT("{0}+{1}/{2}")),
            FText::AsNumber(FMath::RoundToInt(CurrentHP)),
            FText::AsNumber(FMath::RoundToInt(CurrentShield)),
            FText::AsNumber(FMath::RoundToInt(MaxHP))
        );
    }
    else
    {
        // 无护盾时：85/100
        return FText::Format(
            FText::FromString(TEXT("{0}/{1}")),
            FText::AsNumber(FMath::RoundToInt(CurrentHP)),
            FText::AsNumber(FMath::RoundToInt(MaxHP))
        );
    }
}

float UAttributesBar::GetTotalHealth() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }

    return AttributesComponent->GetHP() + AttributesComponent->GetShield();
}

// ========================================
// MP
// ========================================

float UAttributesBar::GetMPPercent() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxMP = AttributesComponent->GetMaxMP();
    if (MaxMP <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(AttributesComponent->GetMP() / MaxMP, 0.0f, 1.0f);
}

FText UAttributesBar::GetMPText() const
{
    if (!AttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    float CurrentMP = AttributesComponent->GetMP();
    float MaxMP = AttributesComponent->GetMaxMP();

    return FText::Format(
        FText::FromString(TEXT("{0}/{1}")),
        FText::AsNumber(FMath::RoundToInt(CurrentMP)),
        FText::AsNumber(FMath::RoundToInt(MaxMP))
    );
}

// ========================================
// Stamina
// ========================================

float UAttributesBar::GetStaminaPercent() const
{
    if (!AttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxStamina = AttributesComponent->GetMaxStamina();
    if (MaxStamina <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(AttributesComponent->GetStamina() / MaxStamina, 0.0f, 1.0f);
}

FText UAttributesBar::GetStaminaText() const
{
    if (!AttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    float CurrentStamina = AttributesComponent->GetStamina();
    float MaxStamina = AttributesComponent->GetMaxStamina();

    return FText::Format(
        FText::FromString(TEXT("{0}/{1}")),
        FText::AsNumber(FMath::RoundToInt(CurrentStamina)),
        FText::AsNumber(FMath::RoundToInt(MaxStamina))
    );
}

// ========================================
// Tick 更新与事件触发
// ========================================

void UAttributesBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!AttributesComponent.IsValid())
    {
        return;
    }

    // 检测 HP 变化
    float CurrentHealthPercent = GetHPPercent();
    if (!FMath::IsNearlyEqual(CurrentHealthPercent, LastHPPercent, 0.01f))
    {
        OnHPChanged(CurrentHealthPercent);
        LastHPPercent = CurrentHealthPercent;
    }

    // 检测 Shield 变化
    if (bShowShield)
    {
        float CurrentShield = GetShield();
        if (!FMath::IsNearlyEqual(CurrentShield, LastShieldPercent, 0.01f))
        {
            OnShieldChanged(CurrentShield);
            LastShieldPercent = CurrentShield;
        }
    }

    // 检测 MP 变化
    if (bShowMana)
    {
        float CurrentManaPercent = GetMPPercent();
        if (!FMath::IsNearlyEqual(CurrentManaPercent, LastMPPercent, 0.01f))
        {
            OnMPChanged(CurrentManaPercent);
            LastMPPercent = CurrentManaPercent;
        }
    }

    // 检测 Stamina 变化
    if (bShowStamina)
    {
        float CurrentStaminaPercent = GetStaminaPercent();
        if (!FMath::IsNearlyEqual(CurrentStaminaPercent, LastStaminaPercent, 0.01f))
        {
            OnStaminaChanged(CurrentStaminaPercent);
            LastStaminaPercent = CurrentStaminaPercent;
        }
    }

    // 检测低血量
    if (!bWasLowHealth && CurrentHealthPercent <= LowHPThreshold && CurrentHealthPercent > 0.0f)
    {
        OnLowHealth();
        bWasLowHealth = true;
    }
    else if (bWasLowHealth && CurrentHealthPercent > LowHPThreshold)
    {
        bWasLowHealth = false;
    }

    // 检测死亡
    if (bWasAlive && CurrentHealthPercent <= 0.0f)
    {
        OnOwnerDied();
        bWasAlive = false;
    }
}

