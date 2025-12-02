// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "GridMovementComponent.h"
#include "SkillComponent.h"
#include "AttributesComponent.h"
#include "AttributesBar.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"

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
	AttributesComponent = CreateDefaultSubobject<UAttributesComponent>(TEXT("AttributesComponent"));

	// 设置AI控制器来默认附身这个Character
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 创建头顶血条的WidgetComponent
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidgetComponent->SetupAttachment(RootComponent);
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); // 设置为屏幕空间，它会始终面向镜头
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 调整到角色头顶位置
	HealthBarWidgetComponent->SetDrawSize(FVector2D(100.f, 20.f)); // 设置控件的绘制大小
	HealthBarWidgetComponent->SetWidget(nullptr);	// 默认为空

}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthBarWidgetClass && HealthBarWidgetComponent)
	{
		// 将指定的UI类设置给WidgetComponent
		HealthBarWidgetComponent->SetWidgetClass(HealthBarWidgetClass);

		// 使用 AttributesBar 的接口
		if (UAttributesBar* AttributesBar = Cast<UAttributesBar>(HealthBarWidgetComponent->GetUserWidgetObject()))
		{
			AttributesBar->SetOwnerActor(this);
			UE_LOG(LogTemp, Log, TEXT("EnemyCharacter: AttributesBar bound for %s"), *GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyCharacter: HealthBarWidgetClass is not an AttributesBar! "));
		}
	}
	else
	{
		if (!HealthBarWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("EnemyCharacter: HealthBarWidgetClass is NULL for %s! Please set it in BP_EnemyCharacter Class Defaults."), *GetName());
		}
		if (!HealthBarWidgetComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("EnemyCharacter: HealthBarWidgetComponent is NULL for %s!"), *GetName());
		}
	}
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