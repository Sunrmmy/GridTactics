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
UCLASS()
class GRIDTACTICS_API AHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHeroCharacter();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

	// 移动参数
	float GridSizeCM = 100.0f; // 1m = 100cm
	FVector TargetLocation;
	bool bIsMoving = false;
	float MoveSpeed = 300.0f; // cm/s

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Enhanced Input 响应函数
	void OnMove(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* IMC_Hero;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Move;


	// 世界坐标与网格坐标转换
	bool WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY) const;
	FVector GridToWorld(int32 X, int32 Y) const;

	// 获取当前角色所在的网格坐标
	void GetCurrentGrid(int32& OutX, int32& OutY) const;

	// 尝试向某一方向移动一格
	void TryMoveOneStep(int32 DeltaX, int32 DeltaY);

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
