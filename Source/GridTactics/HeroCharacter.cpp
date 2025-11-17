// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroCharacter.h"
#include "GridTacticsPlayerState.h"
#include "AttributesComponent.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.h"
#include "SkillComponent.h"
#include "GridMovementComponent.h"
#include "SkillDataAsset.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GridCell.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	// 不继承角色的旋转
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	// 创建摄像机并将其附加到弹簧臂
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;	// 摄像机不相对于弹簧臂旋转

	// 禁用由 CharacterMovementComponent 引起的自动旋转才能通过 SetActorRotation()手动控制角色朝向
	bUseControllerRotationYaw = false; // 角色不跟随控制器的Yaw旋转
	GetCharacterMovement()->bOrientRotationToMovement = false; // 角色不自动朝向移动方向
	GetCharacterMovement()->bUseControllerDesiredRotation = false; // 角色不使用控制器期望的旋转

	// 创建技能组件
	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	// 创建网格移动组件
	GridMovementComponent = CreateDefaultSubobject<UGridMovementComponent>(TEXT("GridMovementComponent"));
	// 创建属性组件
	AttributesComponent = CreateDefaultSubobject<UAttributesComponent>(TEXT("AttributesComponent"));
}


// Called to bind functionality to input
void AHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 绑定IA_Move到OnMove函数
		EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AHeroCharacter::OnMove);
		EnhancedInput->BindAction(IA_Skill_1, ETriggerEvent::Started, this, &AHeroCharacter::OnSkillButtonPressed, 0);
		EnhancedInput->BindAction(IA_Skill_2, ETriggerEvent::Started, this, &AHeroCharacter::OnSkillButtonPressed, 1);
		// 绑定其他技能键 ...

		// 绑定取消施法
		EnhancedInput->BindAction(IA_PrimaryAttack, ETriggerEvent::Started, this, &AHeroCharacter::OnConfirmSkill);
		// 绑定鼠标点击用于确认施法
		EnhancedInput->BindAction(IA_PrimaryAttack, ETriggerEvent::Started, this, &AHeroCharacter::OnConfirmSkill);
	}
}


// Called when the game starts or when spawned
void AHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 缓存PlayerState的引用
	GTPlayerState = GetPlayerState<AGridTacticsPlayerState>();

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

		// 创建并显示UI
		if (PlayerHUDClass && IsLocallyControlled())
		{
			PlayerHUD = CreateWidget<UHUDWidget>(PlayerController, PlayerHUDClass);
			if (PlayerHUD)
			{
				PlayerHUD->AddToViewport();
			}
		}
	}
}


// Called every frame
void AHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 不再需要手动更新属性组件，它现在自己会Tick（与HeroCharacter解耦）
	//if (AttributesComponent)
	//{
	//	AttributesComponent->UpdateAttributes(DeltaTime);
	//}

	if (SkillComponent)
	{
		// 根据技能组件的状态来决定角色是否可以行动
		if (SkillComponent->GetCurrentSkillState() == ESkillState::Casting)
		{
			RootState = ECharacterRootState::Busy;
		}
		else
		{
			RootState = ECharacterRootState::Idle;
		}

		// 只有在技能组件处于瞄准状态时，才更新方向和范围显示
		if (SkillComponent->GetCurrentSkillState() == ESkillState::Aiming)
		{
			UpdateAimingDirection();
		}
	}
}



void AHeroCharacter::UpdateAimingDirection()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !GridMovementComponent || !SkillComponent) return;

	// 从鼠标位置获取世界方向
	FVector WorldLocation, WorldDirection;
	PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);

	// 计算鼠标指向的地面位置
	FVector PlaneOrigin = GetActorLocation();
	FVector MouseGroundPos = FMath::LinePlaneIntersection(WorldLocation, WorldLocation + WorldDirection * 10000.f, PlaneOrigin, FVector::UpVector);

	// 计算相对于角色的方向并离散化
	FVector DirToMouse = (MouseGroundPos - GetActorLocation()).GetSafeNormal();
	int32 DirX = FMath::RoundToInt(DirToMouse.X);
	int32 DirY = FMath::RoundToInt(DirToMouse.Y);

	// 优先Y轴，如果Y为0再考虑X轴，确保是四个纯方向
	if (FMath::Abs(DirY) >= FMath::Abs(DirX))
	{
		DirX = 0;
		DirY = (DirY > 0) ? 1 : -1;
	}
	else
	{
		DirY = 0;
		DirX = (DirX > 0) ? 1 : -1;
	}

	// 设置目标旋转，Tick中的平滑旋转逻辑会自动处理
	const FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(FVector(DirX, DirY, 0));
	GridMovementComponent->SetTargetRotation(TargetRotation);

	const int32 AimingSkillIndex = SkillComponent->GetAimingSkillIndex();
	// 更新技能范围显示
		// 用SkillComponent的GetSkillData函数来获取数据
	if (const USkillDataAsset* SkillData = SkillComponent->GetSkillData(AimingSkillIndex))
	{
		TArray<FIntPoint> WorldGrids = GetSkillRangeInWorld(SkillData->RangePattern);
		ShowRangeIndicators(WorldGrids);
	}
}


void AHeroCharacter::OnSkillButtonPressed(int32 SkillIndex)
{
	// 只有在角色不忙的时候才能开始技能操作
	if (RootState == ECharacterRootState::Idle && SkillComponent)
	{
		SkillComponent->TryStartAiming(SkillIndex);
	}
}
void AHeroCharacter::OnConfirmSkill()
{
	// 直接将输入转发给SkillComponent
	if (SkillComponent)
	{
		SkillComponent->TryConfirmSkill();
	}
}
void AHeroCharacter::OnCancelSkill()
{
	if (SkillComponent)
	{
		SkillComponent->CancelAiming();
	}
}


void AHeroCharacter::OnMove(const FInputActionValue& Value)
{
	// 只有在Idle状态下才能开始移动
	if (RootState != ECharacterRootState::Idle ||
		(SkillComponent && SkillComponent->GetCurrentSkillState() == ESkillState::Aiming) ||
		(GridMovementComponent && GridMovementComponent->IsMoving())) return;

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
		if (GridMovementComponent) {
			GridMovementComponent->TryMoveOneStep(DeltaX, DeltaY);
		}
	}
}



TArray<FIntPoint> AHeroCharacter::GetSkillRangeInWorld(const TArray<FIntPoint>& Pattern) const
{
	TArray<FIntPoint> WorldGrids;
	if (!GridMovementComponent) {
		return WorldGrids;
	}
	int32 CurrentX, CurrentY;
	GridMovementComponent->GetCurrentGrid(CurrentX, CurrentY);


	const FRotator CurrentRotation = GetActorRotation();

	for (const FIntPoint& RelativePos : Pattern)
	{
		// 将模板中的本地坐标 (相对于+X) 转换为 FVector
		const FVector LocalPosVec(RelativePos.X, RelativePos.Y, 0);

		// 使用角色当前的旋转来旋转这个向量
		const FVector RotatedVec = CurrentRotation.RotateVector(LocalPosVec);

		// 将旋转后的世界空间偏移向量，四舍五入为网格偏移
		const FIntPoint RotatedOffset(FMath::RoundToInt(RotatedVec.X), FMath::RoundToInt(RotatedVec.Y));

		WorldGrids.Add(FIntPoint(CurrentX + RotatedOffset.X, CurrentY + RotatedOffset.Y));
	}

	return WorldGrids;
	//for (const FIntPoint& RelativePos : Pattern)
	//{
	//	FIntPoint RotatedPos = FIntPoint::ZeroValue;

	//	// 处理四方向旋转的关键逻辑
	//	// 基础模式是面向 X+ (1, 0)
	//	if (Direction.X == 1 && Direction.Y == 0) // East (X+), 基准方向
	//	{
	//		RotatedPos = RelativePos;
	//	}
	//	else if (Direction.X == -1 && Direction.Y == 0) // West (X-), 旋转180度
	//	{
	//		RotatedPos.X = -RelativePos.X;
	//		RotatedPos.Y = -RelativePos.Y;
	//	}
	//	else if (Direction.X == 0 && Direction.Y == 1) // North (Y+), 逆时针旋转90度
	//	{
	//		RotatedPos.X = -RelativePos.Y;
	//		RotatedPos.Y = RelativePos.X;
	//	}
	//	else if(Direction.X == 0 && Direction.Y == -1)// South (Y-), 顺时针旋转90度
	//	{
	//		RotatedPos.X = RelativePos.Y;
	//		RotatedPos.Y = -RelativePos.X;
	//	}

	//	WorldGrids.Add(FIntPoint(CurrentX + RotatedPos.X, CurrentY + RotatedPos.Y));
	//}

	//return WorldGrids;
}


AGridTacticsPlayerState* AHeroCharacter::GetGridTacticsPlayerState() const
{
	return GTPlayerState;
}

float AHeroCharacter::GetCurrentActualSpeed() const
{
	if (GridMovementComponent)
	{
		return GridMovementComponent->GetCurrentActualSpeed();
	}
	return 0.0f;
}
