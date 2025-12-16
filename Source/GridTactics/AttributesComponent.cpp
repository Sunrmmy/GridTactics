// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributesComponent.h"

// Sets default values for this component's properties
UAttributesComponent::UAttributesComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// 确保它在物理更新之后，但在角色使用这些值之前运行
	SetTickGroup(ETickingGroup::TG_PostPhysics);
}


// Called when the game starts
void UAttributesComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化当前属性
	HP = MaxHP;
	MP = MaxMP;
	Stamina = MaxStamina;
	Shield = 0;
}

// Called every frame
void UAttributesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 在组件自己的Tick中更新属性
	UpdateAttributes(DeltaTime);
}

void UAttributesComponent::UpdateAttributes(float DeltaTime)
{
	UpdateModifiers(DeltaTime);

	// 属性恢复
	HP = FMath::Min(MaxHP, HP + GetModifiedAttributeValue(EAttributeType::HPRecoveryRate, BaseHPRecoveryRate) * DeltaTime);
	MP = FMath::Min(MaxMP, MP + GetModifiedAttributeValue(EAttributeType::MPRecoveryRate, BaseMPRecoveryRate) * DeltaTime);
	Stamina = FMath::Min(MaxStamina, Stamina + GetModifiedAttributeValue(EAttributeType::StaminaRecoveryRate, BaseStaminaRecoveryRate) * DeltaTime);
}

void UAttributesComponent::AddAttributeModifier(const FAttributeModifier& Modifier)
{
	FAttributeModifier NewMod = Modifier;
	NewMod.TimeRemaining = NewMod.Duration;
	ActiveModifiers.Add(NewMod);
}

void UAttributesComponent::RemoveAttributeModifier(const FGuid& ModifierID)
{
	ActiveModifiers.RemoveAll([&ModifierID](const FAttributeModifier& Mod)
		{
			return Mod.ID == ModifierID;
		});
}

float UAttributesComponent::GetArmor() const
{
	return GetModifiedAttributeValue(EAttributeType::Armor, BaseArmor);
}

float UAttributesComponent::GetMoveSpeed() const
{
	return GetModifiedAttributeValue(EAttributeType::MoveSpeed, BaseMoveSpeed);
}

void UAttributesComponent::ConsumeStamina(float Amount)
{
	Stamina = FMath::Max(0.f, Stamina - Amount);
}

void UAttributesComponent::ConsumeMP(float Amount)
{
	MP = FMath::Max(0.f, MP - Amount);
}

void UAttributesComponent::ApplyDamage(float DamageAmount)
{
    const float DamageAfterArmor = FMath::Max(0.f, DamageAmount - GetArmor());
    if (DamageAfterArmor <= 0.f) return;

    const float DamageToShield = FMath::Min(Shield, DamageAfterArmor);
    Shield -= DamageToShield;

    const float DamageToHP = DamageAfterArmor - DamageToShield;
    HP = FMath::Max(0.f, HP - DamageToHP);

    UE_LOG(LogTemp, Log, TEXT("Damage: %.1f -> %.1f after armor. HP: %.1f, Shield: %.1f"),
        DamageAmount, DamageAfterArmor, HP, Shield);

    // 广播受伤事件
    OnDamageTaken.Broadcast(DamageAfterArmor);

    // 检查死亡
    if (HP <= 0.0f && !bIsDead)
    {
        bIsDead = true;
        HandleDeath();
    }
}

void UAttributesComponent::AddShield(float Amount)
{
	Shield = FMath::Min(MaxShield, Shield + Amount);
}

void UAttributesComponent::UpdateModifiers(float DeltaTime)
{
	for (int32 i = ActiveModifiers.Num() - 1; i >= 0; --i)
	{
		FAttributeModifier& Mod = ActiveModifiers[i];
		if (Mod.Duration > 0)
		{
			Mod.TimeRemaining -= DeltaTime;
			if (Mod.TimeRemaining <= 0)
			{
				ActiveModifiers.RemoveAt(i);
			}
		}
	}
}

float UAttributesComponent::GetModifiedAttributeValue(EAttributeType Attribute, float BaseValue) const
{
	float Additive = 0.f;
	float Multiplicative = 1.f;

	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		if (Mod.AttributeToModify == Attribute)
		{
			if (Mod.Type == EModifierType::Additive)
			{
				Additive += Mod.Value;
			}
			else if (Mod.Type == EModifierType::Multiplicative)
			{
				Multiplicative *= Mod.Value;
			}
		}
	}

	return (BaseValue + Additive) * Multiplicative;
}

void UAttributesComponent::HandleDeath()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s died!"), *Owner->GetName());

	// 广播死亡事件
	OnCharacterDied.Broadcast(Owner);

	// 延迟销毁角色
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		[Owner]()
		{
			if (Owner && !Owner->IsPendingKillPending())
			{
				Owner->Destroy();
			}
		},
		1.5f,  // 1.5秒后销毁
		false
	);
}

bool UAttributesComponent::HasModifierForAttribute(EAttributeType Attribute) const
{
	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		if (Mod.AttributeToModify == Attribute)
		{
			return true;
		}
	}
	return false;
}

bool UAttributesComponent::HasModifierForAttributeAndType(EAttributeType Attribute, EModifierType Type) const
{
	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		if (Mod.AttributeToModify == Attribute && Mod.Type == Type)
		{
			return true;
		}
	}
	return false;
}

TArray<FAttributeModifier> UAttributesComponent::GetModifiersForAttribute(EAttributeType Attribute) const
{
	TArray<FAttributeModifier> Result;
	
	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		if (Mod.AttributeToModify == Attribute)
		{
			Result.Add(Mod);
		}
	}
	
	return Result;
}

void UAttributesComponent::RemoveModifiersForAttribute(EAttributeType Attribute)
{
	int32 RemovedCount = ActiveModifiers.RemoveAll([Attribute](const FAttributeModifier& Mod)
	{
		return Mod.AttributeToModify == Attribute;
	});

	UE_LOG(LogTemp, Log, TEXT("AttributesComponent: Removed %d modifiers for attribute %d"), 
		RemovedCount, (int32)Attribute);
}

void UAttributesComponent::RemoveModifiersForAttributeAndType(EAttributeType Attribute, EModifierType Type)
{
	int32 RemovedCount = ActiveModifiers.RemoveAll([Attribute, Type](const FAttributeModifier& Mod)
	{
		return Mod.AttributeToModify == Attribute && Mod.Type == Type;
	});

	UE_LOG(LogTemp, Log, TEXT("AttributesComponent: Removed %d modifiers for attribute %d type %d"), 
		RemovedCount, (int32)Attribute, (int32)Type);
}