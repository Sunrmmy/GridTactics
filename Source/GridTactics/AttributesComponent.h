// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridTacticsPlayerState.h"		// 引用 EModifierType、FAttributeModifier结构体
#include "AttributesComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GRIDTACTICS_API UAttributesComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttributesComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 由所属Actor的Tick调用
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

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetShield() const { return Shield; }
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float GetMaxShield() const { return MaxShield; }

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

	// 新增：死亡事件委托
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDied, AActor*, Character);
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCharacterDied OnCharacterDied;

	/** 受伤事件委托 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageTaken, float, DamageAmount);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDamageTaken OnDamageTaken;

	// 新增：检查是否已有特定属性的修改器
	UFUNCTION(BlueprintPure, Category = "Attributes")
	bool HasModifierForAttribute(EAttributeType Attribute) const;

	// 新增：检查是否已有特定属性和类型的修改器
	UFUNCTION(BlueprintPure, Category = "Attributes")
	bool HasModifierForAttributeAndType(EAttributeType Attribute, EModifierType Type) const;

	// 新增：获取特定属性的所有修改器（用于调试）
	UFUNCTION(BlueprintPure, Category = "Attributes")
	TArray<FAttributeModifier> GetModifiersForAttribute(EAttributeType Attribute) const;

	// 新增：移除特定属性的所有修改器（用于 Buff 刷新）
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void RemoveModifiersForAttribute(EAttributeType Attribute);

	// 新增：移除特定属性和类型的所有修改器
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void RemoveModifiersForAttributeAndType(EAttributeType Attribute, EModifierType Type);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// --- Base Attributes ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float MaxHP = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseHPRecoveryRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float MaxMP = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseMPRecoveryRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float MaxStamina = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseStaminaRecoveryRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseArmor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base")
	float BaseMoveSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Base", meta = (Tooltip = "护盾的最大值"))
	float MaxShield = 100.0f;

	// --- Current Attributes ---
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Current", meta = (AllowPrivateAccess = "true"))
	float HP;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Current", meta = (AllowPrivateAccess = "true"))
	float MP;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Current", meta = (AllowPrivateAccess = "true"))
	float Stamina;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attributes|Current", meta = (AllowPrivateAccess = "true"))
	float Shield;
		
	/** 处理角色死亡 */
	void HandleDeath();

private:
	// 属性修改器
	UPROPERTY()
	TArray<FAttributeModifier> ActiveModifiers;

	void UpdateModifiers(float DeltaTime);

	// 死亡状态
	bool bIsDead = false;
};
