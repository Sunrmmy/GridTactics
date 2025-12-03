// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SkillEffect.generated.h"

class AActor;
class AGridManager;

/**
 * 技能效果基类 - 所有效果的抽象接口
 * 特点：
 * - EditInlineNew：可在 DataAsset 中实例化
 * - Blueprintable：支持蓝图扩展
 * - 网络友好：预留 Authority 检查
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories)
class GRIDTACTICS_API USkillEffect : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 执行效果（核心接口）
     * @param Instigator 施法者
     * @param TargetGrid 目标网格坐标
     * @param AffectedActors 受影响的角色列表（由技能系统计算）
     * @return 是否执行成功
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Skill Effect")
    bool Execute(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors);
    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors);

    /**
     * 验证是否可以执行
     * @return 是否可以执行
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Skill Effect")
    bool CanExecute(AActor* Instigator, FIntPoint TargetGrid) const;
    virtual bool CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const;

    // ========================================
    // 配置属性
    // ========================================

    /** 效果名称（用于调试） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Info")
    FText EffectName = FText::FromString(TEXT("Unnamed Effect"));

    /** 执行延迟（秒），0 = 立即执行 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Timing", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float ExecutionDelay = 0.0f;

    /** 是否仅在服务器执行（网络模式下） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Network")
    bool bServerOnly = true;

protected:
    // ========================================
    // 辅助函数（供子类使用）
    // ========================================

    /** 获取 GridManager（需要传入 Instigator 以获取 World） */
    UFUNCTION(BlueprintPure, Category = "Skill Effect")
    AGridManager* GetGridManager(AActor* WorldContextObject) const;

    /** 检查是否在服务器上执行 */
    UFUNCTION(BlueprintPure, Category = "Skill Effect")
    bool HasAuthority(AActor* Instigator) const;

    /** 获取角色的 AttributesComponent */
    UFUNCTION(BlueprintPure, Category = "Skill Effect")
    class UAttributesComponent* GetAttributesComponent(AActor* Actor) const;
};