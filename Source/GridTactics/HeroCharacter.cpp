// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroCharacter.h"
#include "GridTacticsPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.h"
#include "SkillComponent.h"
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
	TargetRotation = GetActorRotation();

	GridSizeCM = 100.0f; // 1m

	// 创建技能组件
	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
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

	if (GTPlayerState)
	{
		GTPlayerState->UpdateAttributes(DeltaTime);
	}

	// 平滑旋转到目标方向
	if (!GetActorRotation().Equals(TargetRotation, 0.1f))
	{
		// 使用 RInterpTo 进行平滑插值
		FRotator CurrentRotation = GetActorRotation();
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 10.0f); // 10.0f 是旋转速度，可以调整
		SetActorRotation(NewRotation);
	}

	// 根据不同状态执行不同逻辑
	switch (CurrentState)
	{
	case ECharacterState::Idle:
		// 空闲时可以恢复体力等
		break;
	case ECharacterState::Moving:
		// 处理移动逻辑
		HandleMovement(DeltaTime);
		break;
	case ECharacterState::Aiming:
		// 准备施法时，更新朝向和范围显示
		UpdateAimingDirection();
		break;
	case ECharacterState::Casting:
		// 施法中，锁定所有操作
		break;
	}
}

void AHeroCharacter::HandleMovement(float DeltaTime)
{
	FVector Current = GetActorLocation();
	FVector Target2D = FVector(TargetLocation.X, TargetLocation.Y, Current.Z); // 保持当前高度

	float Dist2D = FVector::DistXY(Current, TargetLocation);

	float CurrentMoveSpeed = BaseMoveSpeed;		// 默认速度
	if (GTPlayerState) {						// 从PlayerState获取移动速度
		CurrentMoveSpeed = GTPlayerState->GetMoveSpeed();
	}

	float MoveStep = CurrentMoveSpeed * DeltaTime;

	// 如果下一步会越过目标，直接吸附
	if (Dist2D <= MoveStep)
	{
		SetActorLocation(Target2D);
		CurrentState = ECharacterState::Idle; // 移动结束，返回Idle状态
		UE_LOG(LogTemp, Warning, TEXT("Snapped to target, state is now Idle"));
	}
	else
	{
		// 向目标水平移动
		FVector Dir = (Target2D - Current).GetSafeNormal();
		FVector NewLocation = Current + Dir * MoveStep;
		SetActorLocation(NewLocation);
	}
}

void AHeroCharacter::UpdateAimingDirection()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

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
	TargetRotation = UKismetMathLibrary::MakeRotFromX(FVector(DirX, DirY, 0));

	// 更新技能范围显示
	if (SkillComponent && AimingSkillIndex != -1)
	{
		// 用SkillComponent的GetSkillData函数来获取数据
		if (const USkillDataAsset* SkillData = SkillComponent->GetSkillData(AimingSkillIndex))
		{
			TArray<FIntPoint> WorldGrids = GetSkillRangeInWorld(SkillData->RangePattern);
			ShowRangeIndicators(WorldGrids);
		}
	}
}

void AHeroCharacter::OnSkillButtonPressed(int32 SkillIndex)
{
	// 只有在空闲状态下才能开始准备技能
	if (CurrentState == ECharacterState::Idle)
	{
		CurrentState = ECharacterState::Aiming;
		AimingSkillIndex = SkillIndex;
		UE_LOG(LogTemp, Log, TEXT("Entering Aiming mode for skill %d"), SkillIndex);
	}
	// 如果正在准备同一个技能，则取消
	else if (CurrentState == ECharacterState::Aiming && AimingSkillIndex == SkillIndex)
	{
		CurrentState = ECharacterState::Idle;
		AimingSkillIndex = -1;
		HideRangeIndicators(); // 调用蓝图事件隐藏范围显示
		UE_LOG(LogTemp, Log, TEXT("Canceled Aiming mode"));
	}
	// 如果正在准备一个技能，但按下了另一个技能键，则切换到新技能的准备状态
	else if (CurrentState == ECharacterState::Aiming && AimingSkillIndex != SkillIndex)
	{
		AimingSkillIndex = SkillIndex;
		// 范围显示会在Tick中自动更新，无需额外操作
		UE_LOG(LogTemp, Log, TEXT("Switched to Aiming mode for skill %d"), SkillIndex);
	}
}

void AHeroCharacter::OnConfirmSkill()
{
	// 只有在准备施法状态下，鼠标点击才有效
	if (CurrentState != ECharacterState::Aiming) return;

	if (SkillComponent && AimingSkillIndex != -1)
	{
		// 激活技能（此时会消耗资源并进入冷却）
		SkillComponent->TryActivateSkill(AimingSkillIndex);

		// 进入施法状态，锁定操作
		CurrentState = ECharacterState::Casting;
		HideRangeIndicators(); // 隐藏范围显示

		// 假设技能施法时间为0.5秒，这个值可以从SkillDataAsset中读取
		const float CastTime = 0.5f;
		GetWorldTimerManager().SetTimer(CastingTimerHandle, this, &AHeroCharacter::FinishCasting, CastTime, false);

		UE_LOG(LogTemp, Log, TEXT("Casting skill %d for %f seconds"), AimingSkillIndex, CastTime);
	}
}

void AHeroCharacter::FinishCasting()
{
	// 施法结束，返回空闲状态
	CurrentState = ECharacterState::Idle;
	AimingSkillIndex = -1;
	GetWorldTimerManager().ClearTimer(CastingTimerHandle);
	UE_LOG(LogTemp, Log, TEXT("Finished Casting, returning to Idle."));
}

void AHeroCharacter::HideRangeIndicators_Implementation()
{
	// 这个C++实现留空，所有逻辑都在蓝图中完成。蓝图中的实现会自动覆盖这个空函数。
}

void AHeroCharacter::OnMove(const FInputActionValue& Value)
{
	// 只有在Idle状态下才能开始移动
	if (CurrentState != ECharacterState::Idle) return;

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
	// 状态检查移至OnMove
	if (!GTPlayerState) return;

	// 检查体力
	if (GTPlayerState->GetStamina() < 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to move. Stamina: %f"), GTPlayerState->GetStamina());
		return;
	}

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
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (AGridCell* GridCell = Cast<AGridCell>(Result.GetActor()))
		{
			if (GridCell->IsWalkable())
			{
				bTargetWalkable = true;
				break;
			}
		}
	}

	if (!bTargetWalkable)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Target grid (%d, %d) is blocked or not found."), TargetX, TargetY);
		return;
	}

	// 消耗体力并开始移动
	GTPlayerState->ConsumeStamina(1.0f);
	UE_LOG(LogTemp, Log, TEXT("Moved. Stamina left: %f"), GTPlayerState->GetStamina());
	TargetLocation = TargetWorld;
	CurrentState = ECharacterState::Moving; // 设置状态为移动
	TargetRotation = UKismetMathLibrary::MakeRotFromX(FVector(DeltaX, DeltaY, 0));		// 面向移动方向
}


TArray<FIntPoint> AHeroCharacter::GetSkillRangeInWorld(const TArray<FIntPoint>& Pattern) const
{
	TArray<FIntPoint> WorldGrids;
	int32 CurrentX, CurrentY;
	GetCurrentGrid(CurrentX, CurrentY);

	// 获取角色当前的朝向向量 (离散化为网格方向)
	const FVector ForwardVector = GetActorForwardVector();
	const FIntPoint ForwardDir(FMath::RoundToInt(ForwardVector.X), FMath::RoundToInt(ForwardVector.Y));

	for (const FIntPoint& RelativePos : Pattern)
	{
		FIntPoint RotatedPos;

		// 处理四方向旋转的关键逻辑
		// 基础模式是面向 X+ (1, 0)
		if (ForwardDir.X == 1 && ForwardDir.Y == 0) // East (X+), 基准方向
		{
			RotatedPos = RelativePos;
		}
		else if (ForwardDir.X == -1 && ForwardDir.Y == 0) // West (X-), 旋转180度
		{
			RotatedPos.X = -RelativePos.X;
			RotatedPos.Y = -RelativePos.Y;
		}
		else if (ForwardDir.X == 0 && ForwardDir.Y == 1) // North (Y+), 逆时针旋转90度
		{
			RotatedPos.X = -RelativePos.Y;
			RotatedPos.Y = RelativePos.X;
		}
		else if(ForwardDir.X == 0 && ForwardDir.Y == -1)// South (Y-), 顺时针旋转90度
		{
			RotatedPos.X = RelativePos.Y;
			RotatedPos.Y = -RelativePos.X;
		}

		WorldGrids.Add(FIntPoint(CurrentX + RotatedPos.X, CurrentY + RotatedPos.Y));
	}

	return WorldGrids;
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

AGridTacticsPlayerState* AHeroCharacter::GetGridTacticsPlayerState() const
{
	return GTPlayerState;
}

float AHeroCharacter::GetCurrentActualSpeed() const
{
	// 只在移动状态获取角色速度给动画蓝图
	if (CurrentState == ECharacterState::Moving)
	{
		if (GTPlayerState)
		{
			return GTPlayerState->GetMoveSpeed();
		}
		// 如果PlayerState无效，返回基础速度作为备用
		return BaseMoveSpeed;
	}
	return 0.0f;
}