// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HeroCharacter.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle,           // 空闲
	Moving,         // 移动中
	Aiming,         // 准备施法（显示范围）
	Casting,        // 正在施法（锁定操作）
};

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AGridTacticsPlayerState;
class USkillComponent;
class AGridCell;
UCLASS()
class GRIDTACTICS_API AHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHeroCharacter();

	// 为角色添加自定义的技能组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkillComponent> SkillComponent;

	// 根据技能数据的范围模式获取实际的世界坐标范围
	TArray<FIntPoint> GetSkillRangeInWorld(const TArray<FIntPoint>& Pattern) const;

	// 显示技能范围指示器（蓝图实现）
	UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
	void ShowRangeIndicators(const TArray<FIntPoint>& GridsToHighlight);
	// 隐藏所有范围指示器（蓝图实现）
	UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
	void HideRangeIndicators();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 角色移动旋转的目标方向
	FRotator TargetRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float BaseMoveSpeed = 300.0f; // cm/s

	// 用于显示玩家状态的UI控件蓝图类
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UHUDWidget> PlayerHUDClass;

	// 创建出的UI控件实例
	UPROPERTY()
	TObjectPtr<class UHUDWidget> PlayerHUD;

private:

	// 移动参数
	float GridSizeCM = 100.0f; // 1m = 100cm
	FVector TargetLocation;
	//bool bIsMoving = false;		// 使用状态机代替
	// 角色的当前状态机
	ECharacterState CurrentState = ECharacterState::Idle;

	// 正在准备的技能信息
	int32 AimingSkillIndex = -1;
	FTimerHandle CastingTimerHandle;

	// Tick中调用的状态处理函数
	void HandleMovement(float DeltaTime);
	void UpdateAimingDirection();

	UPROPERTY()
	AGridTacticsPlayerState* GTPlayerState;

public:
	UFUNCTION(BlueprintCallable, Category = "Character")
	AGridTacticsPlayerState* GetGridTacticsPlayerState() const;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Enhanced Input 响应函数
	void OnMove(const FInputActionValue& Value);
	void OnSkillButtonPressed(int32 SkillIndex);
	void OnConfirmSkill();

	// 施法结束时调用的函数
	void FinishCasting(); 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* IMC_Hero;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Move;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Skill_1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Skill_2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_PrimaryAttack;
	// ...其他输入动作...


	// 世界坐标与网格坐标转换
	bool WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY) const;
	FVector GridToWorld(int32 X, int32 Y) const;

	// 获取当前角色所在的网格坐标
	void GetCurrentGrid(int32& OutX, int32& OutY) const;

	// 尝试向某一方向移动一格
	void TryMoveOneStep(int32 DeltaX, int32 DeltaY);

	// 获取角色的基础移动速度
	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetBaseMoveSpeed() const { return BaseMoveSpeed; }

	// 返回角色当前的实际移动速度，用于动画蓝图
	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetCurrentActualSpeed() const;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	//返回CameraBoom subobject
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	// 返回CameraComponent subobject
	UCameraComponent* GetCamera() const { return Camera; }
};
