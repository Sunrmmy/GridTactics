// Fill out your copyright notice in the Description page of Project Settings.


#include "HUDWidget.h"
#include "GameFramework/PlayerState.h"
#include "HeroCharacter.h"
#include "AttributesComponent.h"
#include "GridTactics/Skills/SkillComponent.h"
#include "GridTactics/Skills/SkillDataAsset.h"
#include "GridTacticsPlayerState.h"
#include "Kismet/GameplayStatics.h"

AGridTacticsPlayerState* UHUDWidget::GetGridTacticsPlayerState() const
{
    return CachedPlayerState.Get();
}

void UHUDWidget::SetOwnerCharacter(AHeroCharacter* InOwnerCharacter)
{
    OwnerCharacter = InOwnerCharacter;

    if (OwnerCharacter.IsValid())
    {
        // 缓存组件引用
        CachedAttributesComponent = OwnerCharacter->FindComponentByClass<UAttributesComponent>();
        CachedSkillComponent = OwnerCharacter->FindComponentByClass<USkillComponent>();

        // 新增：绑定技能变更事件
        if (CachedSkillComponent.IsValid())
        {
            CachedSkillComponent->OnSkillAdded.AddDynamic(this, &UHUDWidget::OnSkillChanged);
            CachedSkillComponent->OnSkillReplaced.AddDynamic(this, &UHUDWidget::OnSkillReplaced);

            UE_LOG(LogTemp, Log, TEXT("HUDWidget: Skill change events bound successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("HUDWidget: SkillComponent not found on owner character"));
        }

        UE_LOG(LogTemp, Log, TEXT("HUDWidget: Owner character set to %s"), *InOwnerCharacter->GetName());
    }
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 尝试获取 PlayerState（保留原有逻辑）
    if (!CachedPlayerState.IsValid())
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        if (PlayerController)
        {
            CachedPlayerState = PlayerController->GetPlayerState<AGridTacticsPlayerState>();
        }
    }

    // 新增：尝试获取 OwnerCharacter
    if (!OwnerCharacter.IsValid())
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        if (PlayerController)
        {
            OwnerCharacter = Cast<AHeroCharacter>(PlayerController->GetPawn());
            if (OwnerCharacter.IsValid())
            {
                CachedAttributesComponent = OwnerCharacter->FindComponentByClass<UAttributesComponent>();
                CachedSkillComponent = OwnerCharacter->FindComponentByClass<USkillComponent>();

                // 如果在 Tick 中获取到了 OwnerCharacter，也绑定事件
                if (CachedSkillComponent.IsValid())
                {
                    CachedSkillComponent->OnSkillAdded.AddDynamic(this, &UHUDWidget::OnSkillChanged);
                    CachedSkillComponent->OnSkillReplaced.AddDynamic(this, &UHUDWidget::OnSkillReplaced);

                    UE_LOG(LogTemp, Log, TEXT("HUDWidget: Skill change events bound in NativeTick"));
                }
            }
        }
    }

    // 只有在有有效数据时才更新 UI
    if (CachedPlayerState.IsValid() || CachedAttributesComponent.IsValid())
    {
        OnUpdatePlayerState();
    }
}

//技能变更回调
void UHUDWidget::OnSkillChanged(int32 SlotIndex)
{
    UE_LOG(LogTemp, Log, TEXT("HUDWidget: Skill added at slot %d, refreshing UI"), SlotIndex);
    RefreshSkillSlots();
}

void UHUDWidget::OnSkillReplaced(int32 SlotIndex, USkillDataAsset* NewSkillData)
{
    UE_LOG(LogTemp, Log, TEXT("HUDWidget: Skill replaced at slot %d with %s, refreshing UI"), 
        SlotIndex, 
        NewSkillData ? *NewSkillData->SkillName.ToString() : TEXT("Unknown"));
    RefreshSkillSlots();
}

// ========================================
// 属性访问器实现
// ========================================

float UHUDWidget::GetCurrentHP() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetHP();
    }
    return 0.0f;
}

float UHUDWidget::GetMaxHP() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetMaxHP();
    }
    return 100.0f;
}

float UHUDWidget::GetCurrentMP() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetMP();
    }
    return 0.0f;
}

float UHUDWidget::GetMaxMP() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetMaxMP();
    }
    return 10.0f;
}

float UHUDWidget::GetCurrentStamina() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetStamina();
    }
    return 0.0f;
}

float UHUDWidget::GetMaxStamina() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetMaxStamina();
    }
    return 5.0f;
}

float UHUDWidget::GetCurrentShield() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetShield();
    }
    return 0.0f;
}

float UHUDWidget::GetMaxShield() const
{
    if (CachedAttributesComponent.IsValid())
    {
        return CachedAttributesComponent->GetMaxShield();
    }
    return 0.0f;
}

// ========================================
// 百分比计算
// ========================================

float UHUDWidget::GetHPPercent() const
{
    float Max = GetMaxHP();
    return (Max > 0.0f) ? (GetCurrentHP() / Max) : 0.0f;
}

float UHUDWidget::GetMPPercent() const
{
    float Max = GetMaxMP();
    return (Max > 0.0f) ? (GetCurrentMP() / Max) : 0.0f;
}

float UHUDWidget::GetStaminaPercent() const
{
    float Max = GetMaxStamina();
    return (Max > 0.0f) ? (GetCurrentStamina() / Max) : 0.0f;
}

float UHUDWidget::GetShieldPercent() const
{
    float Max = GetMaxShield();
    return (Max > 0.0f) ? (GetCurrentShield() / Max) : 0.0f;
}

// ========================================
// HP + Shield 组合显示（与 AttributesBar 一致）
// ========================================

float UHUDWidget::GetHealthOnlyPercent() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxHP = CachedAttributesComponent->GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }

    // 仅 HP 的百分比（用于 HP 条主体）
    return FMath::Clamp(CachedAttributesComponent->GetHP() / MaxHP, 0.0f, 1.0f);
}

float UHUDWidget::GetShieldExtensionPercent() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxHP = CachedAttributesComponent->GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }

    // Shield 占 MaxHP 的比例（用于 Shield 延伸条）
    return FMath::Clamp(CachedAttributesComponent->GetShield() / MaxHP, 0.0f, 1.0f);
}

float UHUDWidget::GetTotalHealthPercent() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return 0.0f;
    }

    float MaxHP = CachedAttributesComponent->GetMaxHP();
    if (MaxHP <= 0.0f)
    {
        return 0.0f;
    }

    // 总生命值 = HP + Shield（可能超过 MaxHP）
    float TotalHealth = CachedAttributesComponent->GetHP() + CachedAttributesComponent->GetShield();
    return FMath::Clamp(TotalHealth / MaxHP, 0.0f, 2.0f); // 允许超过 100%
}

FText UHUDWidget::GetCombinedHealthText() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    float CurrentHP = CachedAttributesComponent->GetHP();
    float CurrentShield = CachedAttributesComponent->GetShield();
    float MaxHP = CachedAttributesComponent->GetMaxHP();

    // 创建格式化选项：保留1位小数
    FNumberFormattingOptions NumberFormat;
    NumberFormat.MinimumFractionalDigits = 1;
    NumberFormat.MaximumFractionalDigits = 1;

    if (CurrentShield > 0.0f)
    {
        // 格式：85.0+15.0/100.0（HP + Shield / MaxHP）
        return FText::Format(
            FText::FromString(TEXT("{0}+{1}/{2}")),
            FText::AsNumber(CurrentHP, &NumberFormat),
            FText::AsNumber(CurrentShield, &NumberFormat),
            FText::AsNumber(MaxHP, &NumberFormat)
        );
    }
    else
    {
        // 无护盾时：85.0/100.0
        return FText::Format(
            FText::FromString(TEXT("{0}/{1}")),
            FText::AsNumber(CurrentHP, &NumberFormat),
            FText::AsNumber(MaxHP, &NumberFormat)
        );
    }
}

float UHUDWidget::GetTotalHealth() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return 0.0f;
    }

    return CachedAttributesComponent->GetHP() + CachedAttributesComponent->GetShield();
}

bool UHUDWidget::HasShield() const
{
    return CachedAttributesComponent.IsValid() && CachedAttributesComponent->GetShield() > 0.0f;
}



FText UHUDWidget::GetHPText() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    FNumberFormattingOptions NumberFormat;
    NumberFormat.MinimumFractionalDigits = 1;
    NumberFormat.MaximumFractionalDigits = 1;

    float CurrentHP = CachedAttributesComponent->GetHP();
    float MaxHP = CachedAttributesComponent->GetMaxHP();

    return FText::Format(
        FText::FromString(TEXT("{0}/{1}")),
        FText::AsNumber(CurrentHP, &NumberFormat),
        FText::AsNumber(MaxHP, &NumberFormat)
    );
}

FText UHUDWidget::GetMPText() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    FNumberFormattingOptions NumberFormat;
    NumberFormat.MinimumFractionalDigits = 1;
    NumberFormat.MaximumFractionalDigits = 1;

    float CurrentMP = CachedAttributesComponent->GetMP();
    float MaxMP = CachedAttributesComponent->GetMaxMP();

    return FText::Format(
        FText::FromString(TEXT("{0}/{1}")),
        FText::AsNumber(CurrentMP, &NumberFormat),
        FText::AsNumber(MaxMP, &NumberFormat)
    );
}

FText UHUDWidget::GetStaminaText() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return FText::FromString(TEXT("--/--"));
    }

    FNumberFormattingOptions NumberFormat;
    NumberFormat.MinimumFractionalDigits = 1;
    NumberFormat.MaximumFractionalDigits = 1;

    float CurrentStamina = CachedAttributesComponent->GetStamina();
    float MaxStamina = CachedAttributesComponent->GetMaxStamina();

    return FText::Format(
        FText::FromString(TEXT("{0}/{1}")),
        FText::AsNumber(CurrentStamina, &NumberFormat),
        FText::AsNumber(MaxStamina, &NumberFormat)
    );
}

FText UHUDWidget::GetShieldText() const
{
    if (!CachedAttributesComponent.IsValid())
    {
        return FText::GetEmpty();
    }

    float CurrentShield = CachedAttributesComponent->GetShield();

    if (CurrentShield > 0.0f)
    {
        FNumberFormattingOptions NumberFormat;
        NumberFormat.MinimumFractionalDigits = 1;
        NumberFormat.MaximumFractionalDigits = 1;

        return FText::Format(
            FText::FromString(TEXT("+{0}")),
            FText::AsNumber(CurrentShield, &NumberFormat)
        );
    }
    
    return FText::GetEmpty();
}

// ========================================
// 技能组件访问
// ========================================

USkillComponent* UHUDWidget::GetSkillComponent() const
{
    return CachedSkillComponent.Get();
}