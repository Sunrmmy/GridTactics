// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DisplacementTypes.h"
#include "GridDisplacementRequest.generated.h"

// 位移类型
UENUM(BlueprintType)
enum class EDisplacementType : uint8
{
    Dash        UMETA(DisplayName = "Dash"),           // 冲刺 (指向性位移, 可撞击)
    Knockback   UMETA(DisplayName = "Knockback"),      // 击退 (被动强制位移)
    Teleport    UMETA(DisplayName = "Teleport"),       // 传送 (精确位移到指定格子)
    Push        UMETA(DisplayName = "Push")            // 推动 (类似击退但优先级可能不同)
};

// 位移优先级 (数字越大越优先)
UENUM(BlueprintType)
enum class EDisplacementPriority : uint8
{
    Passive = 0     UMETA(DisplayName = "Passive"),    // 被动位移 (如被击退)
    Active = 10     UMETA(DisplayName = "Active"),     // 主动位移 (如冲刺、传送)
    Forced = 20     UMETA(DisplayName = "Forced")      // 强制位移 (如被技能直接控制)
};

 // 描述一次位移请求的完整信息
USTRUCT(BlueprintType)
struct GRIDTACTICS_API FGridDisplacementRequest
{
    GENERATED_BODY()

    // 发起位移的角色
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    TObjectPtr<AActor> Requester = nullptr;

    // 位移类型
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    EDisplacementType Type = EDisplacementType::Dash;

    // 位移优先级
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    EDisplacementPriority Priority = EDisplacementPriority::Active;

    // 起始网格
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    FIntPoint StartGrid = FIntPoint::ZeroValue;

    // 目标网格 (对于冲刺,这是方向的终点;对于传送,这是精确终点)
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    FIntPoint TargetGrid = FIntPoint::ZeroValue;

    // 位移方向 (单位向量, 仅用于冲刺和击退)
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    FIntPoint Direction = FIntPoint::ZeroValue;

    // 最大位移距离 (格子数)
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    int32 MaxDistance = 1;

    // 是否可以撞击其他角色
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    bool bCanCollideWithActors = false;

    // 撞击后对被撞者的击退距离
    UPROPERTY(BlueprintReadWrite, Category = "Displacement")
    int32 KnockbackDistance = 0;

    // --- 扩展属性 ---
    // 是否在碰撞后停止（false=穿透）
    UPROPERTY(BlueprintReadWrite, Category = "Displacement|Advanced")
    bool bStopOnCollision = true;

    // 是否启用链式击退
    UPROPERTY(BlueprintReadWrite, Category = "Displacement|Advanced")
    bool bEnableChainKnockback = false;

    // 链式击退衰减系数（0.0-1.0）
    UPROPERTY(BlueprintReadWrite, Category = "Displacement|Advanced")
    float ChainKnockbackDecay = 0.5f;

    // 执行持续时间（秒）
    UPROPERTY(BlueprintReadWrite, Category = "Displacement|Advanced")
    float ExecutionDuration = 0.3f;

    // --- 运行时数据 ---
    // 实际计算出的路径 (在规划阶段填充)
    UPROPERTY(BlueprintReadOnly, Category = "Displacement|Runtime")
    TArray<FIntPoint> Path;

    // 实际终点 (在规划阶段计算,考虑了障碍物和碰撞)
    UPROPERTY(BlueprintReadOnly, Category = "Displacement|Runtime")
    FIntPoint ActualEndGrid = FIntPoint::ZeroValue;

    // 在路径上撞击到的角色 (在规划阶段填充)
    UPROPERTY(BlueprintReadOnly, Category = "Displacement|Runtime")
    TObjectPtr<AActor> CollidedActor = nullptr;

    // 多个碰撞信息
    UPROPERTY(BlueprintReadOnly, Category = "Displacement|Runtime")
    TArray<FCollisionInfo> CollisionResults;

    // 路径验证结果
    UPROPERTY(BlueprintReadOnly, Category = "Displacement|Runtime")
    FPathValidationResult ValidationResult;

    // 执行结果
    UPROPERTY(BlueprintReadOnly, Category = "Displacement|Runtime")
    EDisplacementResult ExecutionResult = EDisplacementResult::Success;

    // 唯一ID,用于追踪和调试
    UPROPERTY(BlueprintReadOnly, Category = "Displacement|Runtime")
    FGuid RequestID;

    // 默认构造函数
    FGridDisplacementRequest()
    {
        RequestID = FGuid::NewGuid();
    }
    // 带参数的构造函数,方便快速创建请求
    FGridDisplacementRequest(AActor* InRequester, EDisplacementType InType)
        : Requester(InRequester)
        , Type(InType)
    {
        RequestID = FGuid::NewGuid();
    }

    // 检查是否有效
    bool IsValid() const
    {
        return Requester != nullptr && Requester->IsValidLowLevel();
    }

    // 获取移动距离
    int32 GetMoveDistance() const
    {
        return FMath::Abs(ActualEndGrid.X - StartGrid.X) + FMath::Abs(ActualEndGrid.Y - StartGrid.Y);
    }

    // 是否有碰撞
    bool HasCollisions() const
    {
        return CollisionResults.Num() > 0;
    }
};