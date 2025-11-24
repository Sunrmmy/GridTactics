// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"
#include "GridMovementComponent.h"
#include "AttributesComponent.h"
#include "GridCell.h"
#include "DisplacementTypes.h"
#include "PathPlanner.h"
#include "ConflictResolver.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AGridManager::AGridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AGridManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AGridManager::ReserveGrid(AActor* Requester, FIntPoint TargetGrid)
{
    // 检查是否已经有其他Actor预定了这个格子
    if (GridReservations.Contains(TargetGrid))
    {
        return false;
    }

    // 没有被预定，立即为请求者预定该格子
    GridReservations.Add(TargetGrid, Requester);
    return true;
}

void AGridManager::ForceReserveGrid(AActor* Requester, FIntPoint TargetGrid)
{
    // 强制覆盖预定（如果有之前的预定，直接覆盖）
    GridReservations.Add(TargetGrid, Requester);
}

void AGridManager::ReleaseGrid(FIntPoint GridToRelease)
{
    GridReservations.Remove(GridToRelease);
}

// --- 新的位移请求接口 ---

void AGridManager::RequestDash(
    AActor* Requester, 
    FIntPoint Direction, 
    int32 Distance, 
    bool bCanKnockback, 
    int32 KnockbackDist)
{
    if (!Requester) return;

    FGridDisplacementRequest Request;
    Request.Requester = Requester;
    Request.Type = EDisplacementType::Dash;
    Request.Priority = EDisplacementPriority::Active;
    Request.StartGrid = GetActorCurrentGrid(Requester);
    Request.Direction = Direction;
    Request.MaxDistance = Distance;
    Request.bCanCollideWithActors = bCanKnockback;
    Request.KnockbackDistance = KnockbackDist;

    PendingDisplacements.Add(Request);
}

void AGridManager::RequestTeleport(AActor* Requester, FIntPoint TargetGrid)
{
    if (!Requester) return;

    FGridDisplacementRequest Request;
    Request.Requester = Requester;
    Request.Type = EDisplacementType::Teleport;
    Request.Priority = EDisplacementPriority::Active;
    Request.StartGrid = GetActorCurrentGrid(Requester);
    Request.TargetGrid = TargetGrid;
    Request.MaxDistance = FMath::Abs(TargetGrid.X - Request.StartGrid.X) + FMath::Abs(TargetGrid.Y - Request.StartGrid.Y);

    PendingDisplacements.Add(Request);
}

void AGridManager::RequestKnockback(AActor* Target, FIntPoint Direction, int32 Distance)
{
    if (!Target) return;

    FGridDisplacementRequest Request;
    Request.Requester = Target;
    Request.Type = EDisplacementType::Knockback;
    Request.Priority = EDisplacementPriority::Passive;
    Request.StartGrid = GetActorCurrentGrid(Target);
    Request.Direction = Direction;
    Request.MaxDistance = Distance;

    PendingDisplacements.Add(Request);
}

void AGridManager::ProcessDisplacements()
{
    if (PendingDisplacements.Num() == 0) return;

    UE_LOG(LogTemp, Log, TEXT("========== Processing %d Displacement Requests =========="),
        PendingDisplacements.Num());

    CurrentRecursionDepth = 0;

    // 阶段1：路径规划
    PlanAllPaths();

    // 阶段2：冲突解决
    ResolveAllConflicts();

    // 阶段3：执行
    ExecuteAllDisplacements();

    PendingDisplacements.Empty();
    UE_LOG(LogTemp, Log, TEXT("========== Displacement Processing Complete =========="));
}

void AGridManager::PlanAllPaths()
{
    UE_LOG(LogTemp, Log, TEXT("[PHASE 1] Planning Paths..."));

    for (FGridDisplacementRequest& Request : PendingDisplacements)
    {
        if (!Request.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("  Invalid request for actor %s"),
                Request.Requester ? *Request.Requester->GetName() : TEXT("NULL"));
            continue;
        }

        // 使用PathPlanner规划路径
        FPathValidationResult ValidationResult;

        switch (Request.Type)
        {
        case EDisplacementType::Dash:
        case EDisplacementType::Push:
            ValidationResult = UPathPlanner::PlanDashPath(
                this,
                Request.StartGrid,
                Request.Direction,
                Request.MaxDistance,
                Request.bCanCollideWithActors,
                Request.bStopOnCollision,
                Request.KnockbackDistance,
                Request.Requester
            );
            break;

        case EDisplacementType::Knockback:
            ValidationResult = UPathPlanner::PlanKnockbackPath(
                this,
                Request.StartGrid,
                Request.Direction,
                Request.MaxDistance,
                Request.Requester
            );
            break;

        case EDisplacementType::Teleport:
            ValidationResult = UPathPlanner::PlanTeleportPath(
                this,
                Request.StartGrid,
                Request.TargetGrid,
                Request.Requester
            );
            break;
        }

        // 保存结果
        Request.ValidationResult = ValidationResult;
        Request.Path = ValidationResult.ValidPath;
        Request.ActualEndGrid = ValidationResult.ValidPath.Num() > 0
            ? ValidationResult.ValidPath.Last()
            : Request.StartGrid;
        Request.CollisionResults = ValidationResult.Collisions;

        if (ValidationResult.bIsValid)
        {
            UE_LOG(LogTemp, Log, TEXT(" %s: %s -> %s (%d steps)"),
                *Request.Requester->GetName(),
                *Request.StartGrid.ToString(),
                *Request.ActualEndGrid.ToString(),
                Request.Path.Num() - 1);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT(" %s: Path blocked at %s (Reason: %d)"),
                *Request.Requester->GetName(),
                *ValidationResult.BlockedAtGrid.ToString(),
                static_cast<uint8>(ValidationResult.BlockReason));
        }
    }
}

void AGridManager::ResolveAllConflicts()
{
    UE_LOG(LogTemp, Log, TEXT("[PHASE 2] Resolving Conflicts..."));

    TArray<FGridDisplacementRequest> GeneratedKnockbacks;
    UConflictResolver::ResolveAllConflicts(PendingDisplacements, GeneratedKnockbacks);

    // 递归处理生成的击退
    if (GeneratedKnockbacks.Num() > 0 && CurrentRecursionDepth < MaxKnockbackRecursionDepth)
    {
        CurrentRecursionDepth++;
        UE_LOG(LogTemp, Log, TEXT("  Processing %d generated knockbacks (Depth: %d)"),
            GeneratedKnockbacks.Num(), CurrentRecursionDepth);

        ProcessKnockbackQueue(GeneratedKnockbacks);
    }
}

void AGridManager::ProcessKnockbackQueue(TArray<FGridDisplacementRequest>& KnockbackQueue)
{
    // 为击退请求规划路径
    for (FGridDisplacementRequest& KbReq : KnockbackQueue)
    {
        KbReq.StartGrid = GetActorCurrentGrid(KbReq.Requester);

        FPathValidationResult Result = UPathPlanner::PlanKnockbackPath(
            this,
            KbReq.StartGrid,
            KbReq.Direction,
            KbReq.MaxDistance,
            KbReq.Requester
        );

        KbReq.ValidationResult = Result;
        KbReq.Path = Result.ValidPath;
        KbReq.ActualEndGrid = Result.ValidPath.Num() > 0
            ? Result.ValidPath.Last()
            : KbReq.StartGrid;
        KbReq.CollisionResults = Result.Collisions;

        // 区分"完全失败"和"部分成功"
        if (!Result.bIsValid)
        {
            // 完全无法移动，施加撞墙伤害
            HandleKnockbackFailure(
                KbReq.Requester,
                Result.BlockedAtGrid,
                Result.BlockReason
            );
            KbReq.ExecutionResult = EDisplacementResult::Blocked;
        }
        else if (Result.ValidPath.Num() - 1 < KbReq.MaxDistance)
        {
            // 部分成功：移动了，但没有达到预期距离
            UE_LOG(LogTemp, Warning, TEXT("  Knockback partial: %s moved %d/%d grids, hit obstacle"),
                *KbReq.Requester->GetName(),
                Result.ValidPath.Num() - 1,
                KbReq.MaxDistance);

            // 依然施加撞墙伤害（对所有阻挡）
            if (Result.BlockReason == EKnockbackBlockReason::StaticObstacle ||
                Result.BlockReason == EKnockbackBlockReason::OutOfBounds)
            {
                HandleKnockbackFailure(
                    KbReq.Requester,
                    Result.BlockedAtGrid,
                    Result.BlockReason
                );
            }

            KbReq.ExecutionResult = EDisplacementResult::Success;
        }
        else
        {
            // 完全成功
            KbReq.ExecutionResult = EDisplacementResult::Success;
        }
    }

    // 将有效的击退添加到主队列
    for (FGridDisplacementRequest& KbReq : KnockbackQueue)
    {
        if (KbReq.ExecutionResult == EDisplacementResult::Success)
        {
            PendingDisplacements.Add(KbReq);
        }
    }

}

void AGridManager::HandleKnockbackFailure(
    AActor* Target,
    FIntPoint BlockedGrid,
    EKnockbackBlockReason Reason)
{
    UE_LOG(LogTemp, Warning, TEXT("  Knockback failed for %s at %s (Reason: %d)"),
        *Target->GetName(),
        *BlockedGrid.ToString(),
        static_cast<uint8>(Reason));

    // 造成额外伤害
    if (UAttributesComponent* Attributes = Target->FindComponentByClass<UAttributesComponent>())
    {
        float WallCrashDamage = 15.0f;
        Attributes->ApplyDamage(WallCrashDamage);
        UE_LOG(LogTemp, Log, TEXT("    Applied %f wall crash damage to %s"),
            WallCrashDamage, *Target->GetName());
    }

    // TODO: 播放特效、施加眩晕等
}

void AGridManager::ExecuteAllDisplacements()
{
    UE_LOG(LogTemp, Log, TEXT("[PHASE 3] Executing Displacements..."));

    for (const FGridDisplacementRequest& Request : PendingDisplacements)
    {
        if (Request.ExecutionResult == EDisplacementResult::Cancelled)
            continue;

        if (Request.ActualEndGrid == Request.StartGrid)
            continue; // 没有实际移动

        UGridMovementComponent* MovementComp = Request.Requester->FindComponentByClass<UGridMovementComponent>();
        if (!MovementComp)
        {
            UE_LOG(LogTemp, Error, TEXT("  No GridMovementComponent on %s"),
                *Request.Requester->GetName());
            continue;
        }

        // 执行移动（新接口）
        MovementComp->ExecuteDisplacementPath(Request.Path, Request.ExecutionDuration);

        UE_LOG(LogTemp, Log, TEXT("  Executing %s: %d steps over %.2fs"),
            *Request.Requester->GetName(),
            Request.Path.Num() - 1,
            Request.ExecutionDuration);
    }
}







// --- 辅助函数 ---

bool AGridManager::IsGridWalkable(FIntPoint Grid) const
{
    const float GridSizeCM = 100.0f;
    FVector TargetWorld(Grid.X * GridSizeCM, Grid.Y * GridSizeCM, 0.0f);

    const float DetectionRadius = GridSizeCM * 0.4f;
    const FVector DetectionOrigin = TargetWorld + FVector(0, 0, 10.0f);

    FCollisionQueryParams QueryParams;
    QueryParams.bReturnPhysicalMaterial = false;

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic); // 匹配 GridCell 的 ObjectType

    TArray<FOverlapResult> OverlapResults;
    bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
        OverlapResults,
        DetectionOrigin,
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(DetectionRadius),
        QueryParams
    );

    if (!bHasOverlap)
    {
        // 没有检测到任何 GridCell，认为不可通行
        return false;
    }

    // 检查是否有可行走的格子
    for (const FOverlapResult& Result : OverlapResults)
    {
        if (AGridCell* GridCell = Cast<AGridCell>(Result.GetActor()))
        {
            if (GridCell->IsWalkable())
            {
                return true; // 找到了可行走的格子
            }
        }
    }

    // 没有找到可行走的格子，说明这里是墙壁或空地
    return false;
}

AActor* AGridManager::GetActorAtGrid(FIntPoint Grid) const
{
    // 遍历场景中的所有角色,检查其网格坐标
    // 这里简化示例,实际应该用更高效的数据结构
    TArray<AActor*> AllCharacters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllCharacters);

    for (AActor* Actor : AllCharacters)
    {
        if (GetActorCurrentGrid(Actor) == Grid)
        {
            return Actor;
        }
    }
    return nullptr;
}

FIntPoint AGridManager::GetActorCurrentGrid(AActor* Actor) const
{
    if (UGridMovementComponent* MovementComp = Actor->FindComponentByClass<UGridMovementComponent>())
    {
        int32 X, Y;
        MovementComp->GetCurrentGrid(X, Y);
        return FIntPoint(X, Y);
    }
    return FIntPoint::ZeroValue;
}

bool AGridManager::IsGridValid(FIntPoint Grid) const
{
    // 实时检查该坐标是否存在 GridCell
    const float GridSizeCM = 100.0f;
    FVector WorldPos = FVector(Grid.X * GridSizeCM, Grid.Y * GridSizeCM, 0.0f);

    TArray<AActor*> FoundGridCells;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridCell::StaticClass(), FoundGridCells);

    for (AActor* Actor : FoundGridCells)
    {
        AGridCell* GridCell = Cast<AGridCell>(Actor);
        if (GridCell && GridCell->GridCoordinate == Grid)
        {
            return true; // 找到了该坐标的 GridCell
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("Can't Found GridCell"));
    return false; // 该坐标没有 GridCell
}

FVector AGridManager::GridToWorld(FIntPoint Grid) const
{
    const float GridSizeCM = 100.0f;
    return FVector(Grid.X * GridSizeCM, Grid.Y * GridSizeCM, 0.0f);
}

FIntPoint AGridManager::WorldToGrid(FVector WorldPos) const
{
    const float GridSizeCM = 100.0f;
    int32 X = FMath::RoundToInt(WorldPos.X / GridSizeCM);
    int32 Y = FMath::RoundToInt(WorldPos.Y / GridSizeCM);
    return FIntPoint(X, Y);
}

void AGridManager::SubmitCustomRequest(const FGridDisplacementRequest& Request)
{
    if (Request.IsValid())
    {
        PendingDisplacements.Add(Request);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SubmitCustomRequest: Invalid request!"));
    }
}

void AGridManager::ProcessDisplacementsAsync()
{
    // 异步处理的简单实现（可以后续优化为真正的异步）
    ProcessDisplacements();
}