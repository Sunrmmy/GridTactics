// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GridDisplacementRequest.h"
#include "ConflictResolver.generated.h"

// 冲突类型
UENUM(BlueprintType)
enum class EConflictType : uint8
{
    None,
    SameDestination,     // 多个请求同一终点
    PathCrossing,        // 路径交叉
    ChainReaction       // 链式反应（A撞B，B撞C）
};

// 冲突信息
USTRUCT(BlueprintType)
struct GRIDTACTICS_API FConflictInfo
{
    GENERATED_BODY()

    UPROPERTY()
    EConflictType Type = EConflictType::None;

    UPROPERTY()
    TArray<int32> ConflictingRequestIndices;

    UPROPERTY()
    FIntPoint ConflictGrid = FIntPoint::ZeroValue;
};

/**
 * 冲突解决器：处理位移请求之间的冲突
 */
UCLASS()
class GRIDTACTICS_API UConflictResolver : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 检测并解决所有冲突
     * @param Requests 所有待处理的请求（会被修改）
     * @param OutGeneratedKnockbacks 生成的击退请求
     */
    UFUNCTION(BlueprintCallable, Category = "ConflictResolver")
    static void ResolveAllConflicts(
        UPARAM(ref) TArray<FGridDisplacementRequest>& Requests,
        TArray<FGridDisplacementRequest>& OutGeneratedKnockbacks
    );

private:
    // 检测终点冲突
    static TArray<FConflictInfo> DetectEndPointConflicts(
        const TArray<FGridDisplacementRequest>& Requests
    );

    // 解决单个终点冲突
    static void ResolveSingleConflict(
        TArray<FGridDisplacementRequest>& Requests,
        const FConflictInfo& Conflict
    );

    // 生成击退请求
    static void GenerateKnockbackRequests(
        const TArray<FGridDisplacementRequest>& Requests,
        TArray<FGridDisplacementRequest>& OutKnockbacks
    );

    // 处理链式击退
    static void ProcessChainKnockbacks(
        TArray<FGridDisplacementRequest>& Requests,
        TArray<FGridDisplacementRequest>& KnockbackRequests,
        class AGridManager* GridManager
    );
};
