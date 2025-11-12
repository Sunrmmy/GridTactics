// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMovementComponent.h"
#include "GridTacticsPlayerState.h"
#include "GridCell.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Math/UnrealMathUtility.h"

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
		GTPlayerState = OwnerCharacter->GetPlayerState<AGridTacticsPlayerState>();
		TargetRotation = OwnerCharacter->GetActorRotation(); // 初始化旋转
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

	if (CurrentState == EMovementState::Moving)
	{
		HandleMovement(DeltaTime);
	}
}

void UGridMovementComponent::SetTargetRotation(const FRotator& NewRotation)
{
	TargetRotation = NewRotation;
}

void UGridMovementComponent::HandleMovement(float DeltaTime)
{
	if (!OwnerCharacter) return;

	FVector Current = OwnerCharacter->GetActorLocation();
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
		OwnerCharacter->SetActorLocation(Target2D);
		CurrentState = EMovementState::Idle; // 移动结束，返回Idle状态
		UE_LOG(LogTemp, Warning, TEXT("Snapped to target, state is now Idle"));
	}
	else
	{
		// 向目标水平移动
		FVector Dir = (Target2D - Current).GetSafeNormal();
		FVector NewLocation = Current + Dir * MoveStep;
		OwnerCharacter->SetActorLocation(NewLocation);
	}
}

bool UGridMovementComponent::TryMoveOneStep(int32 DeltaX, int32 DeltaY)
{
	// 状态检查移至OnMove
	if (!OwnerCharacter || !GTPlayerState) return false;

	// 检查体力
	if (GTPlayerState->GetStamina() < 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to move. Stamina: %f"), GTPlayerState->GetStamina());
		return false;
	}

	int32 CurrentX, CurrentY;
	GetCurrentGrid(CurrentX, CurrentY);
	int32 TargetX = CurrentX + DeltaX;
	int32 TargetY = CurrentY + DeltaY;

	// 转换为目标世界坐标
	FVector TargetWorld = GridToWorld(TargetX, TargetY);

	// 使用小范围球形重叠检测（半径略小于格子尺寸，避免误触相邻格）
	const float DetectionRadius = GridSizeCM * 0.4f;
	const FVector DetectionOrigin = TargetWorld + FVector(0, 0, 10.0f);
	// 可视化调试球体
	DrawDebugSphere(GetWorld(), DetectionOrigin, DetectionRadius, 12, FColor::Red, false, 2.0f);


	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = false;
	QueryParams.AddIgnoredActor(OwnerCharacter); // 忽略自身
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
		return false;
	}

	// 消耗体力并开始移动
	GTPlayerState->ConsumeStamina(1.0f);
	UE_LOG(LogTemp, Log, TEXT("Moved. Stamina left: %f"), GTPlayerState->GetStamina());
	TargetLocation = TargetWorld;
	CurrentState = EMovementState::Moving; // 设置状态为移动
	TargetRotation = UKismetMathLibrary::MakeRotFromX(FVector(DeltaX, DeltaY, 0));		// 面向移动方向
	return true;
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
		if (GTPlayerState)
		{
			return GTPlayerState->GetMoveSpeed();
		}
		// 如果PlayerState无效，返回基础速度作为备用
		return BaseMoveSpeed;
	}
	return 0.0f;
}