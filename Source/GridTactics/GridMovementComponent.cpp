// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMovementComponent.h"
#include "AttributesComponent.h"
#include "GridCell.h"
#include "GridManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UGridMovementComponent::UGridMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGridMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		// 获取拥有者身上的AttributesComponent
		AttributesComp = OwnerCharacter->FindComponentByClass<UAttributesComponent>();
		TargetRotation = OwnerCharacter->GetActorRotation(); // 初始化旋转
	}
	if (!AttributesComp)
	{
		UE_LOG(LogTemp, Error, TEXT("GridMovementComponent: AttributesComponent not found on owner!"));
	}

	// 在游戏开始时找到唯一的GridManager实例
	if (GridManagerClass)
	{
		GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), GridManagerClass));
	}

	if (!GridManager)
	{
		UE_LOG(LogTemp, Error, TEXT("GridManager not found in the world! Check GridManagerClass setting in the owning character's blueprint."));
	}
}


// Called every frame
void UGridMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter) return;

	// 平滑旋转到目标方向
	if (!OwnerCharacter->GetActorRotation().Equals(TargetRotation, 0.1f))
	{
		FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 12.0f);	// 最后一个参数为旋转速度
		OwnerCharacter->SetActorRotation(NewRotation);
	}

	if (CurrentState == EMovementState::Moving || CurrentState == EMovementState::ForcedMoving)
	{
		HandleMovement(DeltaTime);
	}
}

void UGridMovementComponent::SetTargetRotation(const FRotator& NewRotation)
{
	TargetRotation = NewRotation;
}


bool UGridMovementComponent::IsGridWalkable(int32 X, int32 Y) const
{
	if (!GetWorld()) return false;

	FVector CheckPos = GridToWorld(X, Y);
	const float DetectionRadius = GridSizeCM * 0.4f;
	const FVector DetectionOrigin = CheckPos + FVector(0, 0, 10.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FOverlapResult> OverlapResults;
	GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		DetectionOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(DetectionRadius),
		QueryParams
	);

	for (const FOverlapResult& Result : OverlapResults)
	{
		if (AGridCell* GridCell = Cast<AGridCell>(Result.GetActor()))
		{
			if (GridCell->IsWalkable())
			{
				return true;
			}
		}
	}
	return false;
}

void UGridMovementComponent::HandleMovement(float DeltaTime)
{
	if (!OwnerCharacter) return;

	if (CurrentState == EMovementState::Moving)
	{
		// WASD移动逻辑
		if (!AttributesComp) return;

		FVector Current = OwnerCharacter->GetActorLocation();
		FVector Target2D = FVector(TargetLocation.X, TargetLocation.Y, Current.Z);
		float Dist2D = FVector::DistXY(Current, TargetLocation);
		const float CurrentMoveSpeed = AttributesComp->GetMoveSpeed();
		float MoveStep = CurrentMoveSpeed * DeltaTime;

		if (Dist2D <= MoveStep)
		{
			OwnerCharacter->SetActorLocation(Target2D);
			CurrentState = EMovementState::Idle;
			if (GridManager) GridManager->ReleaseGrid(CurrentTargetGrid);
		}
		else
		{
			FVector Dir = (Target2D - Current).GetSafeNormal();
			OwnerCharacter->SetActorLocation(Current + Dir * MoveStep);
		}
	}
	else if (CurrentState == EMovementState::ForcedMoving)
	{
		// 处理强制位移（插值移动）
		ForcedMoveElapsedTime += DeltaTime;
		float Alpha = FMath::Clamp(ForcedMoveElapsedTime / ForcedMoveTotalTime, 0.0f, 1.0f);

		// 使用简单的线性插值，如果需要类似跳跃的抛物线，可以在这里修改Z轴
		FVector NewLocation = FMath::Lerp(ForcedMoveStartLocation, TargetLocation, Alpha);

		// 保持原有的Z轴高度，或者根据需要处理高度变化
		NewLocation.Z = OwnerCharacter->GetActorLocation().Z;

		OwnerCharacter->SetActorLocation(NewLocation);

		if (Alpha >= 1.0f)
		{
			CurrentState = EMovementState::Idle;
			if (GridManager) GridManager->ReleaseGrid(CurrentTargetGrid);
			// 恢复胶囊体碰撞
			if (UCapsuleComponent* Cap = OwnerCharacter->GetCapsuleComponent())
			{
				Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			}
			UE_LOG(LogTemp, Log, TEXT("Forced move finished."));
		}
	}
}

bool UGridMovementComponent::TryMoveOneStep(int32 DeltaX, int32 DeltaY)
{
	// 状态检查移至OnMove
	if (!OwnerCharacter || !AttributesComp || !GridManager) return false;

	// 检查体力
	if (AttributesComp->GetStamina() < 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to move. Stamina: %f"), AttributesComp->GetStamina());
		return false;
	}

	int32 CurrentX, CurrentY;
	GetCurrentGrid(CurrentX, CurrentY);
	int32 TargetX = CurrentX + DeltaX;
	int32 TargetY = CurrentY + DeltaY;

	FIntPoint TargetGrid(TargetX, TargetY);		// 目标格子坐标

	// 向GridManager请求预定目标格子
	if (!GridManager->ReserveGrid(OwnerCharacter, TargetGrid))		// GridManager拒绝请求，格子已被其他人预定
	{
		UE_LOG(LogTemp, Warning, TEXT("Grid (%d, %d) is reserved. Cannot move."), TargetX, TargetY);
		return false;
	}

	// 转换为目标世界坐标
	FVector TargetWorld = GridToWorld(TargetX, TargetY);

	// 检查目标GridCell是否可行走
	if (!IsGridWalkable(TargetX, TargetY))
	{
		UE_LOG(LogTemp, Verbose, TEXT("Target grid is blocked."));
		return false;
	}

	// 检查目标格子是否被其他角色占据
	{
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(GetOwner());

		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

		FHitResult HitResult;
		bool bHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
			GetWorld(),
			TargetWorld,
			TargetWorld,
			20.0f, // 使用一个固定的、较小的半径
			ObjectTypes,
			false,
			ActorsToIgnore,
			EDrawDebugTrace::ForDuration, // 调试时显示红色球体
			HitResult,
			true
		);

		if (bHit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grid (%d, %d) is occupied by %s. Cannot move."), TargetX, TargetY, *HitResult.GetActor()->GetName());
			return false; // 目标格子被占据，移动失败
		}
	}

	// 消耗体力并开始移动
	AttributesComp->ConsumeStamina(1.0f);
	UE_LOG(LogTemp, Log, TEXT("Moved. Stamina left: %f"), AttributesComp->GetStamina());
	TargetLocation = TargetWorld;
	CurrentState = EMovementState::Moving; // 设置状态为移动
	TargetRotation = UKismetMathLibrary::MakeRotFromX(FVector(DeltaX, DeltaY, 0));		// 面向移动方向

	// 记录下我们正在移动的目标网格
	CurrentTargetGrid = TargetGrid;

	return true;
}

AActor* UGridMovementComponent::GetActorAtGrid(int32 GridX, int32 GridY) const
{
	if (!GetWorld()) return nullptr;

	FVector CheckPos = GridToWorld(GridX, GridY);
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(),
		CheckPos,
		CheckPos,
		20.0f,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResult,
		true
	);

	if (bHit)
	{
		return HitResult.GetActor();
	}
	return nullptr;
}

// 执行强制位移
void UGridMovementComponent::ExecuteForcedMove(FIntPoint InTargetGrid, float Duration)
{
	if (!OwnerCharacter || !GridManager) return;

	// 临时将胶囊体对 Pawn 的碰撞设置为 Ignore，防止冲锋时被敌人弹飞
	if (UCapsuleComponent* Cap = OwnerCharacter->GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	// 1. 设置目标参数
	ForcedMoveStartLocation = OwnerCharacter->GetActorLocation();
	CurrentTargetGrid = InTargetGrid;
	TargetLocation = GridToWorld(InTargetGrid.X, InTargetGrid.Y);
	ForcedMoveTotalTime = FMath::Max(Duration, 0.01f); // 避免除以0
	ForcedMoveElapsedTime = 0.0f;

	// 2. 强制预定网格（防止其他人在位移过程中走进目标格子）
	GridManager->ForceReserveGrid(OwnerCharacter, CurrentTargetGrid);

	// 3. 切换状态
	CurrentState = EMovementState::ForcedMoving;

	// 4. 旋转朝向目标
	FVector Dir = (TargetLocation - ForcedMoveStartLocation).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		TargetRotation = Dir.Rotation();
	}
}

// 接收击退
void UGridMovementComponent::ReceiveKnockback(FIntPoint KnockbackDirection, int32 Distance)
{
	int32 CurrentX, CurrentY;
	GetCurrentGrid(CurrentX, CurrentY);
	FIntPoint CurrentGrid(CurrentX, CurrentY);

	// 1. 计算理想击退目标
	FIntPoint IdealTarget = CurrentGrid + KnockbackDirection * Distance;
	FIntPoint FinalTarget = IdealTarget;

	// 2. 智能防卡墙逻辑：如果目标不可行走 (Block)，则搜寻周围 4 格
	if (!IsGridWalkable(FinalTarget.X, FinalTarget.Y))
	{
		TArray<FIntPoint> Offsets = { FIntPoint(0, 1), FIntPoint(0, -1), FIntPoint(1, 0), FIntPoint(-1, 0) };

		// 随机打乱顺序，实现随机弹射
		int32 LastIndex = Offsets.Num() - 1;
		for (int32 i = 0; i <= LastIndex; ++i)
		{
			int32 Index = FMath::RandRange(i, LastIndex);
			if (i != Index)
			{
				Offsets.Swap(i, Index);
			}
		}

		bool bFoundValidNeighbor = false;
		for (const FIntPoint& Offset : Offsets)
		{
			FIntPoint Neighbor = IdealTarget + Offset;
			// 避免弹回原位
			if (Neighbor == CurrentGrid) continue;

			if (IsGridWalkable(Neighbor.X, Neighbor.Y))
			{
				FinalTarget = Neighbor;
				bFoundValidNeighbor = true;
				break; // 找到一个就停，因为已经打乱过顺序了，相当于随机
			}
		}

		if (!bFoundValidNeighbor)
		{
			// 如果四周全是墙，被卡死，原地不动或眩晕
			UE_LOG(LogTemp, Log, TEXT("Knockback blocked completely by walls."));
			return;
		}
	}

	// 3. 连锁击退逻辑：检查落点是否有其他人
	AActor* Occupant = GetActorAtGrid(FinalTarget.X, FinalTarget.Y);
	if (Occupant && Occupant != GetOwner())
	{
		// 找到占位者的移动组件
		if (UGridMovementComponent* OccupantComp = Occupant->FindComponentByClass<UGridMovementComponent>())
		{
			UE_LOG(LogTemp, Log, TEXT("Chain Knockback: %s pushes %s"), *GetOwner()->GetName(), *Occupant->GetName());

			// 将占位者也击退
			// 为了体现“被挤开”的效果，我们沿用当前的击退方向，或者随机方向
			// 既然题目要求“向周围四格击退”，我们再次传入 KnockbackDirection，
			// 让占位者自己的 ReceiveKnockback 逻辑去处理“如果前方有阻挡就找周围”的逻辑。
			// 这样形成了递归：A撞B -> B去目标格 -> B发现墙/人 -> B找周围
			OccupantComp->ReceiveKnockback(KnockbackDirection, 1);
		}
	}

	// 4. 执行强制位移
	float KnockbackDuration = 0.2f; // 击退通常很快
	ExecuteForcedMove(FinalTarget, KnockbackDuration);
}




// 世界坐标转网格坐标
bool UGridMovementComponent::WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY) const
{
	OutX = FMath::RoundToInt(WorldPos.X / GridSizeCM);
	OutY = FMath::RoundToInt(WorldPos.Y / GridSizeCM);
	return true;
}

FVector UGridMovementComponent::GridToWorld(int32 X, int32 Y) const
{
	return FVector(X * GridSizeCM, Y * GridSizeCM, 0.0f);
}

void UGridMovementComponent::GetCurrentGrid(int32& OutX, int32& OutY) const
{
	if (OwnerCharacter) {
		WorldToGrid(OwnerCharacter->GetActorLocation(), OutX, OutY);
	}
}

float UGridMovementComponent::GetCurrentActualSpeed() const
{
	// 只在移动状态获取角色速度给动画蓝图
	if (CurrentState == EMovementState::Moving)
	{
		if (AttributesComp)
		{
			return AttributesComp->GetMoveSpeed();
		}
	}
	return 0.0f;
}

float UGridMovementComponent::GetBaseMoveSpeed() const
{
	if (AttributesComp)
	{
		// 基础速度现在也由AttributesComponent管理
		// 直接调用它的GetMoveSpeed，因为它内部会处理基础值和Modifier
		return AttributesComp->GetMoveSpeed();
	}
	// 如果组件无效，返回一个安全的默认值
	return 300.f;
}