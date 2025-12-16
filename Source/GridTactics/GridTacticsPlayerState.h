// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GridTacticsPlayerState.generated.h"

UENUM(BlueprintType)
enum class EAttributeType : uint8
{
	HP,
	MP,
	Stamina,
	HPRecoveryRate,
	MPRecoveryRate,
	StaminaRecoveryRate,
	Armor,
	MoveSpeed,
	Shield,
};

UENUM(BlueprintType)
enum class EModifierType : uint8
{
	Additive,      // 加法
	Multiplicative // 乘法
};

USTRUCT(BlueprintType)
struct FAttributeModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeType AttributeToModify;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EModifierType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration; // 持续时间，<= 0 表示永久

	float TimeRemaining;

	FAttributeModifier()
		: ID(FGuid::NewGuid()), Type(EModifierType::Additive), Value(0.f), Duration(-1.f), TimeRemaining(-1.f)
	{
	}
};

/**
 *
 */
UCLASS()
class GRIDTACTICS_API AGridTacticsPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AGridTacticsPlayerState();

	// 属性定义、修改属性函数 全部迁移到 AttributesComponent
	// 获取属性Attributes		
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetHP() const;
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxHP() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMP() const;
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxMP() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetStamina() const;
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetArmor() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMoveSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetShield() const;
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxShield() const;

	//Attribute Setter/Consumer
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ConsumeStamina(float Amount);
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ConsumeMP(float Amount);
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ApplyDamage(float DamageAmount);
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void AddShield(float Amount);

	float GetModifiedAttributeValue(EAttributeType Attribute, float BaseValue) const;


protected:
	virtual void BeginPlay() override;

};