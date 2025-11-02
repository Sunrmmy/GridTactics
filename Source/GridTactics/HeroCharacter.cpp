// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GridCell.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

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

	if (!IMC_Hero || !IA_Move)
	{
		UE_LOG(LogTemp, Error, TEXT("IMC_Hero or IA_Move is NULL! Check HeroCharacter blueprint settings."));
		return;
	}

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
		FVector Target2D = FVector(TargetLocation.X, TargetLocation.Y, Current.Z); // 保持当前高度

		float Dist2D = FVector::DistXY(Current, TargetLocation);
		float MoveStep = MoveSpeed * DeltaTime;

		// 如果下一步会越过目标，直接吸附
		if (Dist2D <= MoveStep)
		{
			SetActorLocation(Target2D);
			bIsMoving = false;
			UE_LOG(LogTemp, Warning, TEXT("Snapped to target"));
		}
		else
		{
			// 向目标水平移动
			FVector Dir = (Target2D - Current).GetSafeNormal();
			FVector NewLocation = Current + Dir * MoveStep;
			SetActorLocation(NewLocation);
		}
	}
}

// Called to bind functionality to input
void AHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 绑定IA_Move到OnMove函数
		EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AHeroCharacter::OnMove);
	}
}


void AHeroCharacter::OnMove(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Log, TEXT("OnMove triggered"));

	if (bIsMoving) return;
	FVector2D MoveVector = Value.Get<FVector2D>();
	if (MoveVector.IsNearlyZero()) return;

	// 四个方向离散化（WASD）
	MoveVector = MoveVector.GetSafeNormal();
	int32 DeltaX = FMath::RoundToInt(MoveVector.X);
	int32 DeltaY = FMath::RoundToInt(MoveVector.Y);
	// 防止对角线移动
	if (FMath::Abs(DeltaX) == 1 && FMath::Abs(DeltaY) == 1) {
		return;
	}
	if (DeltaX != 0 || DeltaY != 0) {
		TryMoveOneStep(DeltaX, DeltaY);
	}
}

void AHeroCharacter::TryMoveOneStep(int32 DeltaX, int32 DeltaY)
{
	UE_LOG(LogTemp, Warning, TEXT("CallCheck: bIsMoving = %s"), bIsMoving ? TEXT("TRUE") : TEXT("FALSE"));
	if (bIsMoving) return;


	int32 CurrentX, CurrentY;
	GetCurrentGrid(CurrentX, CurrentY);
	int32 TargetX = CurrentX + DeltaX;
	int32 TargetY = CurrentY + DeltaY;

	// 转换为目标世界坐标
	FVector TargetWorld = GridToWorld(TargetX, TargetY);
	
	// 使用小范围球形重叠检测（半径略小于格子尺寸，避免误触相邻格）
	const float DetectionRadius = GridSizeCM * 0.4f;
	const FVector DetectionOrigin = TargetWorld + FVector(0, 0, 10.0f); // 抬高避免穿地
	// 可视化调试球体
	DrawDebugSphere(GetWorld(), DetectionOrigin, DetectionRadius, 12, FColor::Red, false, 2.0f);


	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = false;
	QueryParams.AddIgnoredActor(this); // 忽略自身
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);	// 匹配 GridCell 的 ObjectType


	TArray<FOverlapResult> OverlapResults;
	bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		DetectionOrigin,
		FQuat::Identity,
		ObjectQueryParams, // 检测所有对象类型
		FCollisionShape::MakeSphere(DetectionRadius),
		QueryParams
	);

	// 检查是否有可行走的格子
	bool bTargetWalkable = false;
	UE_LOG(LogTemp, Warning, TEXT("Overlap detected %d results"), OverlapResults.Num());
	for (const FOverlapResult& Result : OverlapResults)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), *Result.GetActor()->GetName());
		AGridCell* GridCell = Cast<AGridCell>(Result.GetActor());
		if (GridCell && GridCell->IsWalkable())
		{
			bTargetWalkable = true;
			break;
		}
	}

	if (!bTargetWalkable)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Target grid (%d, %d) is blocked or not found."), TargetX, TargetY);
		return;
	}

	TargetLocation = TargetWorld;
	bIsMoving = true;
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
	return FVector(X * GridSizeCM, Y * GridSizeCM, 0.0f);
}

void AHeroCharacter::GetCurrentGrid(int32& OutX, int32& OutY) const
{
	WorldToGrid(GetActorLocation(), OutX, OutY);
}