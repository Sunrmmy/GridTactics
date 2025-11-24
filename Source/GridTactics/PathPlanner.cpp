// Fill out your copyright notice in the Description page of Project Settings.


#include "PathPlanner.h"
#include "GridTactics/GridManager.h"

FPathValidationResult UPathPlanner::PlanDashPath(
    AGridManager* GridManager,
    FIntPoint StartGrid,
    FIntPoint Direction,
    int32 MaxDistance,
    bool bCanCollide,
    bool bStopOnCollision,
    int32 KnockbackDistance,
    AActor* IgnoreActor)
{
    FPathValidationResult Result;
    Result.bIsValid = false;

    if (!GridManager || MaxDistance <= 0)
    {
        Result.BlockReason = EKnockbackBlockReason::InvalidPath;
        return Result;
    }

    FIntPoint CurrentGrid = StartGrid;
    Result.ValidPath.Add(CurrentGrid); // 起点

    for (int32 Step = 1; Step <= MaxDistance; ++Step)
    {
        FIntPoint NextGrid = CurrentGrid + Direction;

        // 1. 边界检查
        if (!GridManager->IsGridValid(NextGrid))
        {
            Result.BlockReason = EKnockbackBlockReason::OutOfBounds;
            Result.BlockedAtGrid = NextGrid;
            break;
        }

        // 2. 静态障碍物检查
        if (!GridManager->IsGridWalkable(NextGrid))
        {
            Result.BlockReason = EKnockbackBlockReason::StaticObstacle;
            Result.BlockedAtGrid = NextGrid;
            break;
        }

        // 3. 动态角色检查
        AActor* ActorAtGrid = GetActorAtGrid(GridManager, NextGrid, IgnoreActor);
        if (ActorAtGrid)
        {
            if (bCanCollide)
            {
                // 先验证击退路径是否有效
                bool bKnockbackValid = ValidateKnockbackPath(
                    GridManager,
                    NextGrid,
                    Direction,
                    KnockbackDistance,
                    ActorAtGrid
                );

                // 记录碰撞
                FCollisionInfo CollisionInfo;
                CollisionInfo.HitActor = ActorAtGrid;
                CollisionInfo.CollisionGrid = NextGrid;
                CollisionInfo.CollisionStep = Step;
                CollisionInfo.bContinueAfterCollision = !bStopOnCollision;
                Result.Collisions.Add(CollisionInfo);

                if (bStopOnCollision)
                {
                    // 只有击退有效时才移动到碰撞点
                    if (bKnockbackValid)
                    {
                        Result.ValidPath.Add(NextGrid);
                        CurrentGrid = NextGrid;
                        UE_LOG(LogTemp, Log, TEXT("  Collision with %s at %s, knockback valid"),
                            *ActorAtGrid->GetName(), *NextGrid.ToString());
                    }
                    // 否则停在前一格，不会卡在一起
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  Collision with %s at %s, knockback BLOCKED - stopping at previous grid"),
                            *ActorAtGrid->GetName(), *NextGrid.ToString());
                    }

                    Result.BlockReason = EKnockbackBlockReason::AnotherActor;
                    Result.BlockedAtGrid = NextGrid;
                    break;
                }
            }
            else
            {
                // 不能碰撞，停在前一格
                Result.BlockReason = EKnockbackBlockReason::AnotherActor;
                Result.BlockedAtGrid = NextGrid;
                break;
            }
        }
        else
        {
            // 空格子，正常添加
            Result.ValidPath.Add(NextGrid);
            CurrentGrid = NextGrid;
        }
    }

    Result.bIsValid = (Result.ValidPath.Num() > 1);
    return Result;
}

// 验证击退路径是否有效
bool UPathPlanner::ValidateKnockbackPath(
    AGridManager* GridManager,
    FIntPoint StartGrid,
    FIntPoint Direction,
    int32 Distance,
    AActor* IgnoreActor)
{
    FPathValidationResult KnockbackResult = PlanKnockbackPath(
        GridManager,
        StartGrid,
        Direction,
        Distance,
        IgnoreActor
    );

    // 只有击退路径完全有效时才返回 true
    return KnockbackResult.bIsValid && KnockbackResult.ValidPath.Num() > 1;
}

FPathValidationResult UPathPlanner::PlanKnockbackPath(
    AGridManager* GridManager,
    FIntPoint StartGrid,
    FIntPoint Direction,
    int32 Distance,
    AActor* IgnoreActor)
{
    FPathValidationResult Result;
    Result.bIsValid = false;

    if (!GridManager || Distance <= 0)
    {
        Result.BlockReason = EKnockbackBlockReason::InvalidPath;
        return Result;
    }

    FIntPoint CurrentGrid = StartGrid;
    Result.ValidPath.Add(CurrentGrid);

    for (int32 Step = 1; Step <= Distance; ++Step)
    {
        FIntPoint NextGrid = CurrentGrid + Direction;

        // 遇到阻挡时，停在当前有效位置
        if (!GridManager->IsGridValid(NextGrid))
        {
            Result.BlockReason = EKnockbackBlockReason::OutOfBounds;
            Result.BlockedAtGrid = NextGrid;
            UE_LOG(LogTemp, Warning, TEXT("    Knockback stopped: Out of bounds at %s (moved %d/%d grids)"), 
                *NextGrid.ToString(), Step - 1, Distance);
            break;
        }

        if (!GridManager->IsGridWalkable(NextGrid))
        {
            Result.BlockReason = EKnockbackBlockReason::StaticObstacle;
            Result.BlockedAtGrid = NextGrid;
            UE_LOG(LogTemp, Warning, TEXT("    Knockback stopped: Static obstacle at %s (moved %d/%d grids)"), 
                *NextGrid.ToString(), Step - 1, Distance);
            break;
        }

        AActor* ActorAtGrid = GetActorAtGrid(GridManager, NextGrid, IgnoreActor);
        if (ActorAtGrid)
        {
            // 击退路径上有其他角色
            FCollisionInfo CollisionInfo;
            CollisionInfo.HitActor = ActorAtGrid;
            CollisionInfo.CollisionGrid = NextGrid;
            CollisionInfo.CollisionStep = Step;
            Result.Collisions.Add(CollisionInfo);

            Result.BlockReason = EKnockbackBlockReason::AnotherActor;
            Result.BlockedAtGrid = NextGrid;
            UE_LOG(LogTemp, Warning, TEXT("    Knockback stopped: Another actor (%s) at %s (moved %d/%d grids)"), 
                *ActorAtGrid->GetName(), *NextGrid.ToString(), Step - 1, Distance);
            break;
        }

        Result.ValidPath.Add(NextGrid);
        CurrentGrid = NextGrid;
    }

    // 只要移动了至少一格，就认为击退成功
    if (Result.ValidPath.Num() > 1)
    {
        Result.bIsValid = true;
        UE_LOG(LogTemp, Log, TEXT("    Knockback SUCCESS: Moved %d grids (requested %d)"), 
            Result.ValidPath.Num() - 1, Distance);
    }
    else
    {
        // 完全没有移动，才算失败
        Result.bIsValid = false;
        UE_LOG(LogTemp, Warning, TEXT("    Knockback FAILED: Unable to move at all"));
    }

    return Result;
}

FPathValidationResult UPathPlanner::PlanTeleportPath(
    AGridManager* GridManager,
    FIntPoint StartGrid,
    FIntPoint TargetGrid,
    AActor* IgnoreActor)
{
    FPathValidationResult Result;
    Result.bIsValid = false;

    if (!GridManager)
    {
        Result.BlockReason = EKnockbackBlockReason::InvalidPath;
        return Result;
    }

    Result.ValidPath.Add(StartGrid);

    // 传送只检查目标点
    if (!GridManager->IsGridValid(TargetGrid))
    {
        Result.BlockReason = EKnockbackBlockReason::OutOfBounds;
        Result.BlockedAtGrid = TargetGrid;
        return Result;
    }

    if (!GridManager->IsGridWalkable(TargetGrid))
    {
        Result.BlockReason = EKnockbackBlockReason::StaticObstacle;
        Result.BlockedAtGrid = TargetGrid;
        return Result;
    }

    AActor* ActorAtTarget = GetActorAtGrid(GridManager, TargetGrid, IgnoreActor);
    if (ActorAtTarget)
    {
        Result.BlockReason = EKnockbackBlockReason::AnotherActor;
        Result.BlockedAtGrid = TargetGrid;
        return Result;
    }

    Result.ValidPath.Add(TargetGrid);
    Result.bIsValid = true;
    return Result;
}

bool UPathPlanner::ValidatePath(
    AGridManager* GridManager,
    const TArray<FIntPoint>& Path,
    AActor* IgnoreActor)
{
    if (Path.Num() < 2) return false;

    for (const FIntPoint& Grid : Path)
    {
        if (!IsGridPassable(GridManager, Grid, IgnoreActor))
        {
            return false;
        }
    }

    return true;
}

bool UPathPlanner::IsGridPassable(
    AGridManager* GridManager,
    FIntPoint Grid,
    AActor* IgnoreActor)
{
    if (!GridManager->IsGridValid(Grid)) return false;
    if (!GridManager->IsGridWalkable(Grid)) return false;

    AActor* ActorAtGrid = GetActorAtGrid(GridManager, Grid, IgnoreActor);
    return ActorAtGrid == nullptr;
}

AActor* UPathPlanner::GetActorAtGrid(
    AGridManager* GridManager,
    FIntPoint Grid,
    AActor* IgnoreActor)
{
    AActor* ActorAtGrid = GridManager->GetActorAtGrid(Grid);
    if (ActorAtGrid == IgnoreActor)
    {
        return nullptr;
    }
    return ActorAtGrid;
}