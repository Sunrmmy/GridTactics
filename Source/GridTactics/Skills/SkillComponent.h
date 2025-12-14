// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillComponent.generated.h"

class USkillDataAsset;
class UBaseSkill;
class AHeroCharacter;

// 技能变更委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillAdded, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillReplaced, int32, SlotIndex, USkillDataAsset*, NewSkillData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillRemoved, int32, SlotIndex);

// 定义技能组件自身的状态
UENUM(BlueprintType)
enum class ESkillState : uint8
{
	Idle,
	Aiming,
	Casting
};

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
	USkillComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 技能变更事件
	UPROPERTY(BlueprintAssignable, Category = "Skills|Events")
	FOnSkillAdded OnSkillAdded;

	UPROPERTY(BlueprintAssignable, Category = "Skills|Events")
	FOnSkillReplaced OnSkillReplaced;

	UPROPERTY(BlueprintAssignable, Category = "Skills|Events")
	FOnSkillRemoved OnSkillRemoved;

	// 尝试开始瞄准一个技能
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void TryStartAiming(int32 SkillIndex);
	// 尝试确认并激活当前正在瞄准的技能
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void TryConfirmSkill();
	// 取消瞄准
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CancelAiming();

	// 根据技能实例获取其在技能插槽中的索引
	int32 GetSkillIndex(const UBaseSkill* SkillInstance) const;

	// 获取指定索引的技能数据资产（SkillIndex为技能在插槽中的索引）
	UFUNCTION(BlueprintPure, Category = "Skills")
	const USkillDataAsset* GetSkillData(int32 SkillIndex) const;

	// 获取指定索引技能的剩余冷却时间
	UFUNCTION(BlueprintPure, Category = "Skills")
	float GetCooldownRemaining(int32 SkillIndex) const;

	// 获取当前状态
	UFUNCTION(BlueprintPure, Category = "Skills")
	ESkillState GetCurrentSkillState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Skills")
	int32 GetAimingSkillIndex() const { return AimingSkillIndex; }

	// 获取当前瞄准的目标格子
	UFUNCTION(BlueprintPure, Category = "Skills")
	FIntPoint GetAimingTargetGrid() const { return AimingTargetGrid; }

	// 设置瞄准目标格子（在 Tick 或鼠标移动时更新）
	void SetAimingTargetGrid(FIntPoint TargetGrid) { AimingTargetGrid = TargetGrid; }

	/** 运行时添加技能 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	bool AddSkill(USkillDataAsset* NewSkillData);

	/** 替换技能 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	bool ReplaceSkill(int32 SlotIndex, USkillDataAsset* NewSkillData);

	/** 获取当前技能数量 */
	UFUNCTION(BlueprintPure, Category = "Skills")
	int32 GetSkillCount() const { return SkillSlots.Num(); }

	/** 检查技能是否已满 */
	UFUNCTION(BlueprintPure, Category = "Skills")
	bool IsSkillSlotsFull() const { return SkillSlots.Num() >= 5; }

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Skills")
	int32 ReplacedSkillIndex;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// 在准备阶段或游戏开始时设置的技能列表
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
	TArray<TObjectPtr<USkillDataAsset>> EquippedSkillsData;

	// 运行时实例化的技能
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skills")
	TArray<FSkillEntry> SkillSlots;

private:
	// 尝试激活指定索引的技能
	bool TryActivateSkill(int32 SkillIndex);
	void FinishCasting();
	// 新增：处理施法延迟结束后的逻辑
	void OnCastDelayFinished();

	UPROPERTY()
	TObjectPtr<AHeroCharacter> OwnerCharacter;

	// 技能状态变量
	ESkillState CurrentState = ESkillState::Idle;
	int32 AimingSkillIndex = -1;

	// 缓存瞄准的目标格子
	FIntPoint AimingTargetGrid = FIntPoint::ZeroValue;

	FTimerHandle CastDelayTimerHandle;

	// TimeCost 计时器（施法后的持续时间）
	FTimerHandle TimeCostTimerHandle;

	// 缓存当前施法的技能索引
	int32 CurrentCastingSkillIndex = -1;
};
