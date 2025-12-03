// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillEffect.h"
#include "SkillEffect_Teleport.generated.h"

/**
 * 传送模式
 */
UENUM(BlueprintType)
enum class ETeleportMode : uint8
{
    Instant     UMETA(DisplayName = "Instant (闪现)"),      // 瞬间传送，无移动过程
    Smooth      UMETA(DisplayName = "Smooth (平滑移动)")    // 平滑移动到目标位置
};

/**
 * 传送效果 - 将施法者传送到目标位置
 */
UCLASS(Blueprintable, EditInlineNew, meta = (DisplayName = "Teleport Effect"))
class GRIDTACTICS_API USkillEffect_Teleport : public USkillEffect
{
	GENERATED_BODY()

public:
    USkillEffect_Teleport();

    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors) override;
    virtual bool CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const override;

    // ========================================
    // 配置属性
    // ========================================

    /** 传送模式 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport|Mode")
    ETeleportMode TeleportMode = ETeleportMode::Instant;

    /** 传送持续时间（仅在 Smooth 模式生效） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport|Mode", meta = (EditCondition = "TeleportMode == ETeleportMode::Smooth", ClampMin = "0.1", ClampMax = "2.0"))
    float TeleportDuration = 0.5f;

    /** 起始高度偏移（cm） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport|Height", meta = (ClampMin = "0.0", ClampMax = "2000.0"))
    float StartHeightOffset = 0.0f;

    /** 目标高度偏移（cm） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport|Height", meta = (ClampMin = "-500.0", ClampMax = "500.0"))
    float EndHeightOffset = 0.0f;

    /** 是否启用抛物线跳跃（仅 Smooth 模式） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport|Height", meta = (EditCondition = "TeleportMode == ETeleportMode::Smooth"))
    bool bUseParabolicArc = false;

    /** 抛物线顶点高度（cm） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport|Height", meta = (EditCondition = "bUseParabolicArc && TeleportMode == ETeleportMode::Smooth", ClampMin = "0.0", ClampMax = "1000.0"))
    float ArcPeakHeight = 200.0f;

protected:
    // ========================================
    // 内部辅助函数声明
    /**
     * 执行自定义传送移动（带高度控制）
     * @param Instigator 施法者
     * @param TargetGrid 目标网格
     * @param GridMgr GridManager 引用
     */
    void ExecuteCustomTeleportMovement(AActor* Instigator, FIntPoint TargetGrid, class AGridManager* GridMgr);
};
