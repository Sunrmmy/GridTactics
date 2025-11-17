// Fill out your copyright notice in the Description page of Project Settings.


#include "GridTacticsPlayerState.h"
#include "HeroCharacter.h"
#include "GridMovementComponent.h"
#include "AttributesComponent.h"

AGridTacticsPlayerState::AGridTacticsPlayerState()
{
	// PlayerState的Tick默认是关闭的，我们让Character来驱动更新
	PrimaryActorTick.bCanEverTick = false;
}

void AGridTacticsPlayerState::BeginPlay()
{
	Super::BeginPlay();

}

// 从Pawn获取AttributesComponent
UAttributesComponent* GetAttributesComponentFromPawn(const APawn* Pawn)
{
	if (!Pawn) return nullptr;
	return Pawn->FindComponentByClass<UAttributesComponent>();
}


// 获取属性Attributes函数的实现
float AGridTacticsPlayerState::GetHP() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetHP();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetMaxHP() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetMaxHP();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetMP() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetMP();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetMaxMP() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetMaxMP();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetStamina() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetStamina();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetMaxStamina() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetMaxStamina();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetArmor() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetArmor();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetMoveSpeed() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetMoveSpeed();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetShield() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetShield();
	}
	return 0.0f;
}

float AGridTacticsPlayerState::GetMaxShield() const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetMaxShield();
	}
	return 0.0f;
}

void AGridTacticsPlayerState::ConsumeStamina(float Amount)
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		AttrComp->ConsumeStamina(Amount);
	}
}

void AGridTacticsPlayerState::ConsumeMP(float Amount)
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		AttrComp->ConsumeMP(Amount);
	}
}

void AGridTacticsPlayerState::ApplyDamage(float DamageAmount)
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		AttrComp->ApplyDamage(DamageAmount);
	}
}

void AGridTacticsPlayerState::AddShield(float Amount)
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		AttrComp->AddShield(Amount);
	}
}

float AGridTacticsPlayerState::GetModifiedAttributeValue(EAttributeType Attribute, float BaseValue) const
{
	if (UAttributesComponent* AttrComp = GetAttributesComponentFromPawn(GetPawn()))
	{
		return AttrComp->GetModifiedAttributeValue(Attribute, BaseValue);
	}
	return BaseValue;
}