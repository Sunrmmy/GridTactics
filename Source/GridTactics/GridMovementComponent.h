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
	Moving,				// WASD移动
	DisplacementMoving  // 位移技能移
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

    // --- 原有接口（保持兼容） ---

    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    FVector GridToWorld(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void GetCurrentGrid(int32& OutX, int32& OutY) const;

    UFUNCTION(BlueprintPure, Category = "Movement")
    bool IsMoving() const { return CurrentState != EMovementState::Idle; }

    bool TryMoveOneStep(int32 DeltaX, int32 DeltaY);

    // 设置目标旋转（供 HeroCharacter 使用）
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetTargetRotation(const FRotator& NewRotation) { TargetRotation = NewRotation; }
    // 获取当前移动速度（供动画蓝图使用）
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetCurrentActualSpeed() const;

    // --- 新接口：位移系统 ---

    /**
     * 执行位移路径（由GridManager调用）
     * @param Path 网格路径
     * @param Duration 总执行时间
     */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void ExecuteDisplacementPath(const TArray<FIntPoint>& Path, float Duration);

    /**
     * 是否正在执行位移
     */
    UFUNCTION(BlueprintPure, Category = "Movement")
    bool IsExecutingDisplacement() const { return CurrentState == EMovementState::DisplacementMoving; }

    /**
     * 立即停止位移
     */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopDisplacement();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<ACharacter> OwnerCharacter;

    UPROPERTY()
    TObjectPtr<class UAttributesComponent> AttributesComp;

    EMovementState CurrentState = EMovementState::Idle;

    // WASD移动数据
    FRotator TargetRotation;
    FVector TargetLocation;
    FIntPoint CurrentTargetGrid;

    // 位移系统数据
    TArray<FVector> DisplacementWorldPath;
    float DisplacementElapsedTime = 0.0f;
    float DisplacementTotalDuration = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float GridSizeCM = 100.0f;

    // Tick处理函数
    void HandleMovement(float DeltaTime);
    void HandleDisplacementMovement(float DeltaTime);

    // --- 用于WASD移动简单检测 ---
    // 检查网格可行走性
    bool IsGridWalkableSimple(int32 X, int32 Y) const;

    // 获取网格上的角色
    AActor* GetActorAtGridSimple(int32 GridX, int32 GridY) const;
};
