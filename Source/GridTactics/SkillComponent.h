// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillComponent.generated.h"

class USkillDataAsset;
class UBaseSkill;

USTRUCT(BlueprintType)
struct FSkillEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBaseSkill> SkillInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CooldownRemaining = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	const USkillDataAsset* SkillData = nullptr;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GRIDTACTICS_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillComponent();

	// 尝试激活指定索引的技能
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void TryActivateSkill(int32 SkillIndex);

	// 获取指定索引的技能数据资产（SkillIndex为技能在插槽中的索引）
	UFUNCTION(BlueprintPure, Category = "Skills")
	const USkillDataAsset* GetSkillData(int32 SkillIndex) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// 在准备阶段或游戏开始时设置的技能列表
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
	TArray<TObjectPtr<USkillDataAsset>> EquippedSkillsData;

	// 运行时实例化的技能
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skills")
	TArray<FSkillEntry> SkillSlots;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
