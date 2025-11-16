// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UGridMovementComponent;
class USkillComponent;
class AEnemyAIController;
UCLASS()
class GRIDTACTICS_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	// 让AI控制器可以获取这些组件来下达指令
	UGridMovementComponent* GetGridMovementComponent() const { return GridMovementComponent; }
	USkillComponent* GetSkillComponent() const { return SkillComponent; }

	UFUNCTION(BlueprintPure, Category = "Animation")
	float GetCurrentActualSpeed() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridMovementComponent> GridMovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkillComponent> SkillComponent;



};
