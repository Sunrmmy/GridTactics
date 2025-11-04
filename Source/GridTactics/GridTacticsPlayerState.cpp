// Fill out your copyright notice in the Description page of Project Settings.


#include "GridTacticsPlayerState.h"
#include "HeroCharacter.h"

AGridTacticsPlayerState::AGridTacticsPlayerState()
{
	// PlayerState的Tick默认是关闭的，我们让Character来驱动更新
	PrimaryActorTick.bCanEverTick = false;
}

void AGridTacticsPlayerState::BeginPlay()
{
	Super::BeginPlay();
	// 初始化当前属性
	HP = MaxHP;
	MP = MaxMP;
	Stamina = MaxStamina;
}

void AGridTacticsPlayerState::UpdateAttributes(float DeltaTime)
{
	UpdateModifiers(DeltaTime);

	// 属性恢复
	HP = FMath::Min(MaxHP, HP + GetModifiedAttributeValue(EAttributeType::HPRecoveryRate, BaseHPRecoveryRate) * DeltaTime);
	MP = FMath::Min(MaxMP, MP + GetModifiedAttributeValue(EAttributeType::MPRecoveryRate, BaseMPRecoveryRate) * DeltaTime);
	Stamina = FMath::Min(MaxStamina, Stamina + GetModifiedAttributeValue(EAttributeType::StaminaRecoveryRate, BaseStaminaRecoveryRate) * DeltaTime);
}

void AGridTacticsPlayerState::AddAttributeModifier(const FAttributeModifier& Modifier)
{
	FAttributeModifier NewMod = Modifier;
	NewMod.TimeRemaining = NewMod.Duration;
	ActiveModifiers.Add(NewMod);
}

void AGridTacticsPlayerState::RemoveAttributeModifier(const FGuid& ModifierID)
{
	ActiveModifiers.RemoveAll([&ModifierID](const FAttributeModifier& Mod)
		{
			return Mod.ID == ModifierID;
		});
}

float AGridTacticsPlayerState::GetArmor() const
{
	return GetModifiedAttributeValue(EAttributeType::Armor, BaseArmor);
}

float AGridTacticsPlayerState::GetMoveSpeed() const
{
	// 获取此PlayerState对应的Pawn
	if (const AHeroCharacter* HeroCharacter = Cast<AHeroCharacter>(GetPawn()))
	{
		// 从角色获取基础速度，然后计算修改后的值
		return GetModifiedAttributeValue(EAttributeType::MoveSpeed, HeroCharacter->GetBaseMoveSpeed());
	}
	return 0.0f;
}

void AGridTacticsPlayerState::ConsumeStamina(float Amount)
{
	Stamina = FMath::Max(0.f, Stamina - Amount);
}

void AGridTacticsPlayerState::UpdateModifiers(float DeltaTime)
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

float AGridTacticsPlayerState::GetModifiedAttributeValue(EAttributeType Attribute, float BaseValue) const
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