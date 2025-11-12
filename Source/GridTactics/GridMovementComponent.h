// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMovementComponent.generated.h"

class AGridTacticsPlayerState;
UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Moving
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

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsMoving() const { return CurrentState == EMovementState::Moving; }

	// 尝试向某一方向移动一格
	bool TryMoveOneStep(int32 DeltaX, int32 DeltaY);

	// 设置期望的旋转
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetTargetRotation(const FRotator& NewRotation);

	// 返回角色当前的实际移动速度，用于动画蓝图
	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetCurrentActualSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetBaseMoveSpeed() const { return BaseMoveSpeed; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// Tick中调用的状态处理函数
	void HandleMovement(float DeltaTime);

	UPROPERTY()
	ACharacter* OwnerCharacter;
	UPROPERTY()
	AGridTacticsPlayerState* GTPlayerState;

	EMovementState CurrentState = EMovementState::Idle;

	// 角色移动旋转的目标方向
	FRotator TargetRotation;
	// 角色移动的目标位置
	FVector TargetLocation;

	// 基础移动速度
	UPROPERTY(EditDefaultsOnly, Category = "Grid")
	float BaseMoveSpeed = 300.0f; // cm/s

	//网格尺寸
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float GridSizeCM = 100.0f; // 1m = 100cm

};
