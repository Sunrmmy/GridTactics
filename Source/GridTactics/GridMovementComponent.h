// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMovementComponent.generated.h"

class UAttributesComponent;
class AGridManager;
UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Moving,
	ForcedMoving // 强制位移状态（冲锋、击退等）
};
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GRIDTACTICS_API UGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGridMovementComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 世界坐标与网格坐标转换
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY) const;
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GridToWorld(int32 X, int32 Y) const;

	// 获取当前角色所在的网格坐标
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GetCurrentGrid(int32& OutX, int32& OutY) const;

	// 获取指定网格上的Actor（用于技能检测碰撞）
	UFUNCTION(BlueprintCallable, Category = "Grid")
	AActor* GetActorAtGrid(int32 GridX, int32 GridY) const;

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsMoving() const { return CurrentState == EMovementState::Moving || CurrentState == EMovementState::ForcedMoving;; }

	// 尝试向某一方向移动一格
	bool TryMoveOneStep(int32 DeltaX, int32 DeltaY);

	// 执行强制位移
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ExecuteForcedMove(FIntPoint TargetGrid, float Duration);
	// 接收击退效果
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ReceiveKnockback(FIntPoint KnockbackDirection, int32 Distance = 1);
	// 检查指定格子是否可行走 (暴露给技能使用)
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool IsGridWalkable(int32 X, int32 Y) const;


	// 设置期望的旋转
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetTargetRotation(const FRotator& NewRotation);

	// 返回角色当前的实际移动速度，用于动画蓝图
	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetCurrentActualSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetBaseMoveSpeed() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// Tick中调用的状态处理函数
	void HandleMovement(float DeltaTime);

	UPROPERTY()
	ACharacter* OwnerCharacter;

	// 属性组件引用
	UPROPERTY()
	TObjectPtr<UAttributesComponent> AttributesComp;

	EMovementState CurrentState = EMovementState::Idle;

	// 角色移动旋转的目标方向
	FRotator TargetRotation;
	// 角色移动的目标位置
	FVector TargetLocation;

	//// 基础移动速度	// 现在由AttributesComponent管理
	//UPROPERTY(EditDefaultsOnly, Category = "Grid")
	//float BaseMoveSpeed = 300.0f; // cm/s

	//网格尺寸
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float GridSizeCM = 100.0f; // 1m = 100cm


	// 网格仲裁者引用
	UPROPERTY()
	TObjectPtr<AGridManager> GridManager;
	// 允许在蓝图中指定要查找的GridManager类
	UPROPERTY(EditDefaultsOnly, Category = "Grid")
	TSubclassOf<AGridManager> GridManagerClass;
	// 记录当前正在移动的目标网格，以便在移动结束后释放
	FIntPoint CurrentTargetGrid;


	// 强制位移相关
	float ForcedMoveTotalTime = 0.0f;
	float ForcedMoveElapsedTime = 0.0f;
	FVector ForcedMoveStartLocation;
};
