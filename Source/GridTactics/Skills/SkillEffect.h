// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SkillEffect.generated.h"

class AActor;
class AGridManager;
class UBaseSkill;  // ✅ 前向声明

/**
 * 技能效果基类 - 所有效果的抽象接口
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
     * @param AffectedActors 受影响的角色列表（如果 bRecheckRangeOnExecution=true 则为空）
     * @return 是否执行成功
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Skill Effect")
    bool Execute(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors);
    virtual bool Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors);

    /**
     * 验证是否可以执行
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

    // ✅ 新增：是否在执行时重新检测范围
    /** 
     * 是否在效果执行时重新检测范围内的目标（适用于延迟伤害）
     * - true: 执行时实时检测范围（精确但有性能开销）
     * - false: 使用技能激活时的目标列表（快速但可能不精确）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Timing", 
        meta = (EditCondition = "ExecutionDelay > 0.0"))
    bool bRecheckRangeOnExecution = false;

    // ✅ 新增：技能引用（由 BaseSkill 设置，用于重新检测范围）
    UPROPERTY()
    TWeakObjectPtr<UBaseSkill> OwningSkill;

protected:
    // ========================================
    // 辅助函数
    // ========================================

    /** 获取 GridManager */
    UFUNCTION(BlueprintPure, Category = "Skill Effect")
    AGridManager* GetGridManager(AActor* WorldContextObject) const;

    /** 检查是否在服务器上执行 */
    UFUNCTION(BlueprintPure, Category = "Skill Effect")
    bool HasAuthority(AActor* Instigator) const;

    /** 获取角色的 AttributesComponent */
    UFUNCTION(BlueprintPure, Category = "Skill Effect")
    class UAttributesComponent* GetAttributesComponent(AActor* Actor) const;

    // 新增：重新检测范围内的目标
    /**
     * 重新检测当前在范围内的目标（供延迟效果使用）
     * @param Instigator 施法者
     * @param TargetGrid 目标格子
     * @return 当前在范围内的目标列表
     */
    UFUNCTION(BlueprintCallable, Category = "Skill Effect")
    TArray<AActor*> RecheckAffectedActors(AActor* Instigator, FIntPoint TargetGrid) const;
};