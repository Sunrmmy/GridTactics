// Fill out your copyright notice in the Description page of Project Settings.


#include "ConflictResolver.h"
#include "PathPlanner.h"
#include "GridTactics/GridManager.h"

void UConflictResolver::ResolveAllConflicts(
    TArray<FGridDisplacementRequest>& Requests,
    TArray<FGridDisplacementRequest>& OutGeneratedKnockbacks)
{
    OutGeneratedKnockbacks.Empty();

    // 第1步：检测终点冲突
    TArray<FConflictInfo> Conflicts = DetectEndPointConflicts(Requests);

    // 第2步：解决冲突
    for (const FConflictInfo& Conflict : Conflicts)
    {
        ResolveSingleConflict(Requests, Conflict);
    }

    // 第3步：生成击退请求
    GenerateKnockbackRequests(Requests, OutGeneratedKnockbacks);

    UE_LOG(LogTemp, Log, TEXT("ConflictResolver: Resolved %d conflicts, generated %d knockbacks"),
        Conflicts.Num(), OutGeneratedKnockbacks.Num());
}

TArray<FConflictInfo> UConflictResolver::DetectEndPointConflicts(
    const TArray<FGridDisplacementRequest>& Requests)
{
    TArray<FConflictInfo> Conflicts;
    TMap<FIntPoint, TArray<int32>> EndPointMap;

    // 构建终点 -> 请求索引的映射
    for (int32 i = 0; i < Requests.Num(); ++i)
    {
        const FGridDisplacementRequest& Request = Requests[i];
        if (Request.ActualEndGrid != FIntPoint::ZeroValue)
        {
            EndPointMap.FindOrAdd(Request.ActualEndGrid).Add(i);
        }
    }

    // 检测冲突
    for (const auto& Pair : EndPointMap)
    {
        if (Pair.Value.Num() > 1)
        {
            FConflictInfo ConflictInfo;
            ConflictInfo.Type = EConflictType::SameDestination;
            ConflictInfo.ConflictGrid = Pair.Key;
            ConflictInfo.ConflictingRequestIndices = Pair.Value;
            Conflicts.Add(ConflictInfo);

            UE_LOG(LogTemp, Warning, TEXT("Conflict detected at %s: %d requests"),
                *Pair.Key.ToString(), Pair.Value.Num());
        }
    }

    return Conflicts;
}

void UConflictResolver::ResolveSingleConflict(
    TArray<FGridDisplacementRequest>& Requests,
    const FConflictInfo& Conflict)
{
    if (Conflict.ConflictingRequestIndices.Num() < 2) return;

    // 按优先级排序（高到低）
    TArray<int32> SortedIndices = Conflict.ConflictingRequestIndices;
    SortedIndices.Sort([&Requests](int32 A, int32 B) {
        return static_cast<uint8>(Requests[A].Priority) > static_cast<uint8>(Requests[B].Priority);
        });

    // 保留最高优先级的请求，取消其他请求
    int32 WinnerIndex = SortedIndices[0];
    FGridDisplacementRequest& Winner = Requests[WinnerIndex];

    for (int32 i = 1; i < SortedIndices.Num(); ++i)
    {
        int32 LoserIndex = SortedIndices[i];
        FGridDisplacementRequest& Loser = Requests[LoserIndex];

        UE_LOG(LogTemp, Warning, TEXT("  Request %s canceled (Priority %d lost to %d)"),
            *Loser.Requester->GetName(),
            static_cast<uint8>(Loser.Priority),
            static_cast<uint8>(Winner.Priority));

        // 取消请求：重置为起点
        Loser.ActualEndGrid = Loser.StartGrid;
        Loser.Path.Empty();
        Loser.Path.Add(Loser.StartGrid);
        Loser.ExecutionResult = EDisplacementResult::Cancelled;
    }
}

void UConflictResolver::GenerateKnockbackRequests(
    const TArray<FGridDisplacementRequest>& Requests,
    TArray<FGridDisplacementRequest>& OutKnockbacks)
{
    for (const FGridDisplacementRequest& Request : Requests)
    {
        if (Request.ExecutionResult == EDisplacementResult::Cancelled)
            continue;

        if (Request.Type != EDisplacementType::Dash && Request.Type != EDisplacementType::Push)
            continue;

        if (!Request.bCanCollideWithActors || Request.KnockbackDistance <= 0)
            continue;

        // 为每个碰撞生成击退
        for (const FCollisionInfo& Collision : Request.CollisionResults)
        {
            if (!Collision.HitActor) continue;

            FGridDisplacementRequest KnockbackReq;
            KnockbackReq.Requester = Collision.HitActor;
            KnockbackReq.Type = EDisplacementType::Knockback;
            KnockbackReq.Priority = EDisplacementPriority::Forced;
            KnockbackReq.StartGrid = Collision.CollisionGrid;
            KnockbackReq.Direction = Request.Direction;
            KnockbackReq.MaxDistance = Request.KnockbackDistance;

            KnockbackReq.ExecutionDuration = 0.2f;

            OutKnockbacks.Add(KnockbackReq);

            UE_LOG(LogTemp, Log, TEXT("Generated knockback for %s at %s"),
                *Collision.HitActor->GetName(),
                *Collision.CollisionGrid.ToString());
        }
    }
}