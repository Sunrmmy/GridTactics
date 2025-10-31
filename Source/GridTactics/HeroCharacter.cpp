// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/KismetMathLibrary.h"
#include "GridMapManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AHeroCharacter::AHeroCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 创建弹簧臂并将其附加到根组件
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = false; // 关闭弹簧臂基于控制器的旋转
	CameraBoom->bDoCollisionTest = false;

	// 设置弹簧臂的位置以实现俯视视角
	CameraBoom->TargetArmLength = 800.f; // distance to character
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

	// 创建摄像机并将其附加到弹簧臂
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;	// 摄像机不相对于弹簧臂旋转

	GridSizeCM = 100.0f; // 1m
	MoveSpeed = 300.0f;  // 3m/s
}

// Called when the game starts or when spawned
void AHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	// 绑定 Enhanced Input
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(IMC_Hero, 0);
		}
	}
}

// Called every frame
void AHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMoving)
	{
		FVector Current = GetActorLocation();
		float Dist = FVector::Distance(Current, TargetLocation);
		if (Dist < 10.0f)
		{
			SetActorLocation(TargetLocation);
			bIsMoving = false;
		}
		else
		{
			FVector Dir = (TargetLocation - Current).GetSafeNormal();
			SetActorLocation(Current + Dir * MoveSpeed * DeltaTime);
		}
	}
}

// Called to bind functionality to input
void AHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

// 获取 GridMapManager 实例
AGridMapManager* AHeroCharacter::GetGridMapManager()
{
	if (CachedGridManager == nullptr)
	{
		if (UWorld* World = GetWorld())
		{
			CachedGridManager = Cast<AGridMapManager>(
				UGameplayStatics::GetActorOfClass(World, AGridMapManager::StaticClass())
			);
			if (CachedGridManager == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("No AGridMapManager found in world!"));
			}
		}
	}
	return CachedGridManager;
}

void AHeroCharacter::OnMove(const FInputActionValue& Value)
{
	if (bIsMoving) return;
	FVector2D MoveVector = Value.Get<FVector2D>();
	if (MoveVector.IsNearlyZero()) return;

	// 四个方向离散化（WASD）
	MoveVector = MoveVector.GetSafeNormal();
	int32 DeltaX = FMath::RoundToInt(MoveVector.X);
	int32 DeltaY = FMath::RoundToInt(MoveVector.Y);
	// 防止对角线移动
	if (FMath::Abs(DeltaX) == 1 && FMath::Abs(DeltaY) == 1) {
		///////////////////////

		return;
	}
	if (DeltaX != 0 || DeltaY != 0) {
		TryMoveOneStep(DeltaX, DeltaY);
	}
}

void AHeroCharacter::TryMoveOneStep(int32 DeltaX, int32 DeltaY)
{
	int32 CurrentX, CurrentY;
	GetCurrentGrid(CurrentX, CurrentY);

	int32 TargetX = CurrentX + DeltaX;
	int32 TargetY = CurrentY + DeltaY;

	AGridMapManager* Manager = GetGridMapManager();
	if (!Manager)
	{
		UE_LOG(LogTemp, Error, TEXT("GridMapManager not found!"));
		return;
	}

	if (!Manager->IsWalkable(TargetX, TargetY))
	{
		return; // blocked
	}

	TargetLocation = GridToWorld(TargetX, TargetY);
	bIsMoving = true;

	// 使角色朝向移动方向
	if (DeltaX != 0 || DeltaY != 0)
	{
		FRotator NewRot = UKismetMathLibrary::MakeRotFromX(FVector(DeltaX, DeltaY, 0));
		SetActorRotation(NewRot);
	}
}


// 世界坐标转网格坐标
bool AHeroCharacter::WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY) const
{
	OutX = FMath::RoundToInt(WorldPos.X / GridSizeCM);
	OutY = FMath::RoundToInt(WorldPos.Y / GridSizeCM);
	return true;
}

FVector AHeroCharacter::GridToWorld(int32 X, int32 Y) const
{
	return FVector(X * GridSizeCM, Y * GridSizeCM, GetActorLocation().Z);
}

void AHeroCharacter::GetCurrentGrid(int32& OutX, int32& OutY) const
{
	WorldToGrid(GetActorLocation(), OutX, OutY);
}