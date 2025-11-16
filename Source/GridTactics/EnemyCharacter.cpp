// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "GridMovementComponent.h"
#include "SkillComponent.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 禁用默认的角色移动旋转，以便我们通过组件精确控制
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	GridMovementComponent = CreateDefaultSubobject<UGridMovementComponent>(TEXT("GridMovementComponent"));
	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));

	// 设置AI控制器来默认附身这个Character
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float AEnemyCharacter::GetCurrentActualSpeed() const
{
	if (GridMovementComponent && GridMovementComponent->IsMoving())
	{
		return GridMovementComponent->GetCurrentActualSpeed();
	}
	return 0.0f;
}