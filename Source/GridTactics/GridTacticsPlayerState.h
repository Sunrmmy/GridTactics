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
	MoveSpeed
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

	// 由Character的Tick调用
	void UpdateAttributes(float DeltaTime);

	// Modifier Management
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void AddAttributeModifier(const FAttributeModifier& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void RemoveAttributeModifier(const FGuid& ModifierID);

	// 获取属性Attributes
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetHP() const { return HP; }
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMP() const { return MP; }
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxMP() const { return MaxMP; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetStamina() const { return Stamina; }
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetArmor() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMoveSpeed() const;


	//Attribute Setters/Consumers Begin
	void ConsumeStamina(float Amount);

protected:
	virtual void BeginPlay() override;

	// Attributes 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float MaxHP = 100.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Current")
	float HP = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseHPRecoveryRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float MaxMP = 10.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Current")
	float MP = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseMPRecoveryRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float MaxStamina = 5.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Current")
	float Stamina = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseStaminaRecoveryRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseArmor = 10.0f;

private:
	// 属性修改器
	UPROPERTY()
	TArray<FAttributeModifier> ActiveModifiers;

	void UpdateModifiers(float DeltaTime);
	float GetModifiedAttributeValue(EAttributeType Attribute, float BaseValue) const;
};