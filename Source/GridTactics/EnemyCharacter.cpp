// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "GridMovementComponent.h"
#include "SkillComponent.h"
#include "AttributesComponent.h"
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

	if (HealthBarWidgetClass)
	{
		// 将指定的UI类设置给WidgetComponent
		HealthBarWidgetComponent->SetWidgetClass(HealthBarWidgetClass);

		// 获取刚刚创建的UI实例
		UUserWidget* WidgetObject = HealthBarWidgetComponent->GetUserWidgetObject();
		if (WidgetObject)
		{
			// 这是一个动态设置属性的方法，它会查找名为 "AttributesComponent" 的公开变量并设置它
			FObjectProperty* Prop = FindFProperty<FObjectProperty>(WidgetObject->GetClass(), "AttributesComponent");
			if (Prop)
			{
				Prop->SetObjectPropertyValue_InContainer(WidgetObject, AttributesComponent);
			}
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