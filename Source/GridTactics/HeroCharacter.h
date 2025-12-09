// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HeroCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AGridTacticsPlayerState;
class USkillComponent;
class AGridCell;
class UGridMovementComponent;
class USkillDataAsset;
class UAttributesComponent;
class APlayerController;

UENUM(BlueprintType)
enum class ECharacterRootState : uint8
{
	Idle,           // 空闲
	Busy            // 使用通用的忙碌状态来表示角色正在执行移动或正在施法等（具体状态迁移到其它组件中）
};

UCLASS()
class GRIDTACTICS_API AHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHeroCharacter();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;
	//返回CameraBoom subobject
	TObjectPtr<USpringArmComponent> GetCameraBoom() const { return CameraBoom; }
	// 返回CameraComponent subobject
	TObjectPtr<UCameraComponent> GetCamera() const { return Camera; }


	// 为角色添加网格移动组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UGridMovementComponent> GridMovementComponent;
	// 为角色添加自定义的技能组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkillComponent> SkillComponent;
	// 为角色添加新的属性组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttributesComponent> AttributesComponent;

	// ========================================
	// Input 变量
	// ========================================

	/** Enhanced Input Mapping Context */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_Hero;

	/** Input Actions */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_PrimaryAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Cancel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Skill_0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Skill_1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Skill_2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Skill_3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Skill_4;

	// ========================================
	// 公开接口
	// ========================================
	
	UFUNCTION(BlueprintPure, Category = "Skills")
	TArray<FIntPoint> GetSkillRangeInWorld(const TArray<FIntPoint>& Pattern) const;

	UFUNCTION(BlueprintPure, Category = "Skills")
	TArray<FIntPoint> GetSkillRangeInWorldFromCenter(const TArray<FIntPoint>& Pattern, FIntPoint CenterGrid) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
	void ShowRangeIndicators(const TArray<FIntPoint>& GridsToHighlight);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skills")
	void ShowEffectIndicators(const TArray<FIntPoint>& GridsToHighlight);

	// 隐藏所有范围指示器（蓝图实现,可以作为函数被调用）
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Skills")
	void HideRangeIndicators();
	//virtual void HideRangeIndicators_Implementation();


	UFUNCTION(BlueprintPure, Category = "Attributes")
	AGridTacticsPlayerState* GetGridTacticsPlayerState() const;

	UFUNCTION(BlueprintPure, Category = "Animation")
	float GetCurrentActualSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	UGridMovementComponent* GetGridMovementComponent() const { return GridMovementComponent; }

	// 新增：UI 类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UHUDWidget> PlayerHUDClass;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// ========================================
	// Input 响应函数（可以保持 protected）
	// ========================================

	void OnMove(const FInputActionValue& Value);
	void OnSkillButtonPressed(int32 SkillIndex);
	void OnConfirmSkill();
	void OnCancelSkill();

	// 技能范围更新相关函数
	void UpdateAimingDirection();
	void UpdateDirectionalSkillRange(const USkillDataAsset* SkillData);
	void UpdateTargetedSkillRange(const USkillDataAsset* SkillData, APlayerController* PC);

	// 辅助函数
	FVector GetDirectionToMouse() const;

	// 覆盖 PossessedBy（在 Pawn 被控制器占有时调用）
	virtual void PossessedBy(AController* NewController) override;

private:
	// 角色状态
	UPROPERTY()
	TObjectPtr<AGridTacticsPlayerState> GTPlayerState;

	// 创建出的UI控件实例
	UPROPERTY()
	TObjectPtr<class UHUDWidget> PlayerHUD;

	// 角色当前的根状态
	ECharacterRootState RootState = ECharacterRootState::Idle;
};

