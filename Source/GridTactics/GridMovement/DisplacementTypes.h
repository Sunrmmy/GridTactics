#pragma once

#include "CoreMinimal.h"
#include "DisplacementTypes.generated.h"

// 位移结果状态
UENUM(BlueprintType)
enum class EDisplacementResult : uint8
{
    Success,            // 成功
    Blocked,            // 被阻挡
    Cancelled,          // 被取消
    InvalidTarget,      // 无效目标
    OutOfBounds        // 超出边界
};

// 碰撞信息
USTRUCT(BlueprintType)
struct GRIDTACTICS_API FCollisionInfo
{
    GENERATED_BODY()

    // 被撞击的对象
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> HitActor = nullptr;

    // 碰撞点
    UPROPERTY(BlueprintReadOnly)
    FIntPoint CollisionGrid = FIntPoint::ZeroValue;

    // 碰撞步数（第几步发生碰撞）
    UPROPERTY(BlueprintReadOnly)
    int32 CollisionStep = 0;

    // 是否继续移动（穿透模式）
    UPROPERTY(BlueprintReadOnly)
    bool bContinueAfterCollision = false;
};

// 击退失败原因
UENUM(BlueprintType)
enum class EKnockbackBlockReason : uint8
{
    None,
    OutOfBounds,
    StaticObstacle,
    AnotherActor,
    InvalidPath
};

// 路径验证结果
USTRUCT(BlueprintType)
struct GRIDTACTICS_API FPathValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bIsValid = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FIntPoint> ValidPath;

    UPROPERTY(BlueprintReadOnly)
    FIntPoint BlockedAtGrid = FIntPoint::ZeroValue;

    UPROPERTY(BlueprintReadOnly)
    EKnockbackBlockReason BlockReason = EKnockbackBlockReason::None;

    UPROPERTY(BlueprintReadOnly)
    TArray<FCollisionInfo> Collisions;
};