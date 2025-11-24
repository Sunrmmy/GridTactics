// Fill out your copyright notice in the Description page of Project Settings.

#include "GridMovementComponent.h"
#include "AttributesComponent.h"
#include "GridCell.h"
#include "GridManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UGridMovementComponent::UGridMovementComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.
    PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UGridMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        // 获取拥有者身上的 AttributesComponent
        AttributesComp = OwnerCharacter->FindComponentByClass<UAttributesComponent>();
        TargetRotation = OwnerCharacter->GetActorRotation(); // 初始化旋转
    }

    if (!AttributesComp)
    {
        UE_LOG(LogTemp, Error, TEXT("GridMovementComponent: AttributesComponent not found on owner!"));
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
        FRotator NewRotation = FMath::RInterpTo(
            OwnerCharacter->GetActorRotation(),
            TargetRotation,
            DeltaTime,
            12.0f // 旋转速度
        );
        OwnerCharacter->SetActorRotation(NewRotation);
    }

    // 处理移动
    if (CurrentState == EMovementState::DisplacementMoving)
    {
        HandleDisplacementMovement(DeltaTime);
    }
    else if (CurrentState == EMovementState::Moving)
    {
        HandleMovement(DeltaTime);
    }
}

// ========================================
// 原有接口实现（保持兼容）
// ========================================

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
    if (OwnerCharacter)
    {
        WorldToGrid(OwnerCharacter->GetActorLocation(), OutX, OutY);
    }
    else
    {
        OutX = 0;
        OutY = 0;
    }
}

bool UGridMovementComponent::TryMoveOneStep(int32 DeltaX, int32 DeltaY)
{
    // 状态检查
    if (!OwnerCharacter || !AttributesComp) return false;

    // 检查体力
    if (AttributesComp->GetStamina() < 1.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough stamina to move. Stamina: %f"),
            AttributesComp->GetStamina());
        return false;
    }

    int32 CurrentX, CurrentY;
    GetCurrentGrid(CurrentX, CurrentY);
    int32 TargetX = CurrentX + DeltaX;
    int32 TargetY = CurrentY + DeltaY;

    CurrentTargetGrid = FIntPoint(TargetX, TargetY);

    // 获取 GridManager（如果需要网格预定功能）
    AGridManager* GridManager = Cast<AGridManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
    );

    if (GridManager)
    {
        // 向 GridManager 请求预定目标格子
        if (!GridManager->ReserveGrid(OwnerCharacter, CurrentTargetGrid))
        {
            UE_LOG(LogTemp, Warning, TEXT("Grid (%d, %d) is reserved. Cannot move."), TargetX, TargetY);
            return false;
        }
    }

    // 转换为目标世界坐标
    FVector TargetWorld = GridToWorld(TargetX, TargetY);

    // 检查目标格子是否可行走（使用简化检测）
    if (!IsGridWalkableSimple(TargetX, TargetY))
    {
        UE_LOG(LogTemp, Verbose, TEXT("Target grid is blocked."));
        if (GridManager) GridManager->ReleaseGrid(CurrentTargetGrid);
        return false;
    }

    // 检查目标格子是否被其他角色占据
    AActor* OccupyingActor = GetActorAtGridSimple(TargetX, TargetY);
    if (OccupyingActor && OccupyingActor != GetOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("Grid (%d, %d) is occupied by %s. Cannot move."),
            TargetX, TargetY, *OccupyingActor->GetName());
        if (GridManager) GridManager->ReleaseGrid(CurrentTargetGrid);
        return false;
    }

    // 消耗体力并开始移动
    AttributesComp->ConsumeStamina(1.0f);
    UE_LOG(LogTemp, Log, TEXT("Moved. Stamina left: %f"), AttributesComp->GetStamina());

    TargetLocation = TargetWorld;
    CurrentState = EMovementState::Moving;
    TargetRotation = UKismetMathLibrary::MakeRotFromX(FVector(DeltaX, DeltaY, 0));

    return true;
}

// ========================================
// 新接口实现：位移系统
// ========================================

void UGridMovementComponent::ExecuteDisplacementPath(const TArray<FIntPoint>& Path, float Duration)
{
    if (Path.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExecuteDisplacementPath: Invalid path (< 2 points)"));
        return;
    }

    // 转换为世界坐标路径
    DisplacementWorldPath.Empty();
    for (const FIntPoint& Grid : Path)
    {
        FVector WorldPos = GridToWorld(Grid.X, Grid.Y);
        DisplacementWorldPath.Add(WorldPos);
    }

    // 初始化位移状态
    CurrentState = EMovementState::DisplacementMoving;
    DisplacementElapsedTime = 0.0f;
    DisplacementTotalDuration = Duration;

    // 计算朝向（面向路径终点）
    if (DisplacementWorldPath.Num() >= 2)
    {
        FVector StartPos = DisplacementWorldPath[0];
        FVector EndPos = DisplacementWorldPath.Last();
        FVector Direction = (EndPos - StartPos).GetSafeNormal();
        if (!Direction.IsNearlyZero())
        {
            TargetRotation = Direction.Rotation();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ExecuteDisplacementPath: %d waypoints over %.2fs"),
        DisplacementWorldPath.Num(), Duration);
}

void UGridMovementComponent::StopDisplacement()
{
    if (CurrentState == EMovementState::DisplacementMoving)
    {
        CurrentState = EMovementState::Idle;
        DisplacementWorldPath.Empty();
        DisplacementElapsedTime = 0.0f;

        UE_LOG(LogTemp, Log, TEXT("Displacement stopped manually"));
    }
}

// ========================================
// 内部处理函数
// ========================================

void UGridMovementComponent::HandleMovement(float DeltaTime)
{
    if (!OwnerCharacter || !AttributesComp) return;

    FVector Current = OwnerCharacter->GetActorLocation();
    FVector Target2D = FVector(TargetLocation.X, TargetLocation.Y, Current.Z);
    float Dist2D = FVector::DistXY(Current, TargetLocation);
    const float CurrentMoveSpeed = AttributesComp->GetMoveSpeed();
    float MoveStep = CurrentMoveSpeed * DeltaTime;

    if (Dist2D <= MoveStep)
    {
        // 到达目标
        OwnerCharacter->SetActorLocation(Target2D);
        CurrentState = EMovementState::Idle;

        // 释放网格预定
        AGridManager* GridManager = Cast<AGridManager>(
            UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
        );
        if (GridManager)
        {
            GridManager->ReleaseGrid(CurrentTargetGrid);
        }
    }
    else
    {
        // 继续移动
        FVector Dir = (Target2D - Current).GetSafeNormal();
        OwnerCharacter->SetActorLocation(Current + Dir * MoveStep);
    }
}

void UGridMovementComponent::HandleDisplacementMovement(float DeltaTime)
{
    if (DisplacementWorldPath.Num() < 2) return;

    DisplacementElapsedTime += DeltaTime;
    float Progress = FMath::Clamp(DisplacementElapsedTime / DisplacementTotalDuration, 0.0f, 1.0f);

    // 线性插值整个路径
    int32 TotalSegments = DisplacementWorldPath.Num() - 1;
    float SegmentProgress = Progress * TotalSegments;
    int32 CurrentSegment = FMath::FloorToInt(SegmentProgress);
    float LocalProgress = SegmentProgress - CurrentSegment;

    if (CurrentSegment >= TotalSegments)
    {
        // 完成位移
        FVector FinalPos = DisplacementWorldPath.Last();
        FinalPos.Z = OwnerCharacter->GetActorLocation().Z; // 保持 Z 轴
        OwnerCharacter->SetActorLocation(FinalPos);

        CurrentState = EMovementState::Idle;
        DisplacementWorldPath.Empty();

        UE_LOG(LogTemp, Log, TEXT("Displacement completed"));
        return;
    }

    // 插值当前段
    FVector StartPos = DisplacementWorldPath[CurrentSegment];
    FVector EndPos = DisplacementWorldPath[CurrentSegment + 1];
    FVector NewPos = FMath::Lerp(StartPos, EndPos, LocalProgress);

    // 保持 Z 轴（或者你可以根据需要添加抛物线效果）
    NewPos.Z = OwnerCharacter->GetActorLocation().Z;

    OwnerCharacter->SetActorLocation(NewPos);
}

// ========================================
// 辅助函数（简化版）
// ========================================

bool UGridMovementComponent::IsGridWalkableSimple(int32 X, int32 Y) const
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

AActor* UGridMovementComponent::GetActorAtGridSimple(int32 GridX, int32 GridY) const
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


float UGridMovementComponent::GetCurrentActualSpeed() const
{
    if (!AttributesComp)
    {
        return 0.0f;
    }

    // 根据当前状态返回速度
    if (CurrentState == EMovementState::Moving)
    {
        // WASD 移动：返回基础移动速度
        return AttributesComp->GetMoveSpeed();
    }
    else if (CurrentState == EMovementState::DisplacementMoving)
    {
        // 位移技能：返回一个固定的"冲刺速度"用于动画
        // 或者根据路径长度和持续时间计算
        if (DisplacementWorldPath.Num() >= 2 && DisplacementTotalDuration > 0.0f)
        {
            float TotalDistance = 0.0f;
            for (int32 i = 0; i < DisplacementWorldPath.Num() - 1; ++i)
            {
                TotalDistance += FVector::Dist(DisplacementWorldPath[i], DisplacementWorldPath[i + 1]);
            }
            return TotalDistance / DisplacementTotalDuration; // 平均速度
        }
        return AttributesComp->GetMoveSpeed() * 2.0f; // 默认为两倍速度
    }

    return 0.0f; // 静止状态
}