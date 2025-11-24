#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GridDisplacementRequest.h"
#include "PathPlanner.generated.h"

/**
 * 路径规划器：负责计算各种位移类型的路径
 * 单一职责：只做路径计算，不处理冲突
 */
UCLASS()
class GRIDTACTICS_API UPathPlanner : public UObject
{
    GENERATED_BODY()

public:
    // 规划冲刺路径
    UFUNCTION(BlueprintCallable, Category = "PathPlanner")
    static FPathValidationResult PlanDashPath(
        class AGridManager* GridManager,
        FIntPoint StartGrid,
        FIntPoint Direction,
        int32 MaxDistance,
        bool bCanCollide,
        bool bStopOnCollision,
        int32 KnockbackDistance,
        AActor* IgnoreActor = nullptr
    );

    // 规划击退路径
    UFUNCTION(BlueprintCallable, Category = "PathPlanner")
    static FPathValidationResult PlanKnockbackPath(
        AGridManager* GridManager,
        FIntPoint StartGrid,
        FIntPoint Direction,
        int32 Distance,
        AActor* IgnoreActor = nullptr
    );

    // 规划传送路径
    UFUNCTION(BlueprintCallable, Category = "PathPlanner")
    static FPathValidationResult PlanTeleportPath(
        AGridManager* GridManager,
        FIntPoint StartGrid,
        FIntPoint TargetGrid,
        AActor* IgnoreActor = nullptr
    );

    // 验证路径是否仍然有效（用于重新验证）
    UFUNCTION(BlueprintCallable, Category = "PathPlanner")
    static bool ValidatePath(
        AGridManager* GridManager,
        const TArray<FIntPoint>& Path,
        AActor* IgnoreActor = nullptr
    );

private:
    // 检查单个格子是否可通行
    static bool IsGridPassable(
        AGridManager* GridManager,
        FIntPoint Grid,
        AActor* IgnoreActor
    );

    // 检查格子上的角色
    static AActor* GetActorAtGrid(
        AGridManager* GridManager,
        FIntPoint Grid,
        AActor* IgnoreActor
    );

    // 验证击退路径
    static bool ValidateKnockbackPath(
        AGridManager* GridManager,
        FIntPoint StartGrid,
        FIntPoint Direction,
        int32 Distance,
        AActor* IgnoreActor
    );
};