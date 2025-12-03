// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseSkill.generated.h"

class ACharacter;
class USkillDataAsset;
class USkillComponent;
class UAttributesComponent;
class UGridMovementComponent;

UCLASS(Blueprintable, BlueprintType, Abstract)
class GRIDTACTICS_API UBaseSkill : public UObject
{
    GENERATED_BODY()

public:
    // 初始化技能，由 SkillComponent 调用
    virtual void Initialize(ACharacter* InOwner, const USkillDataAsset* InSkillData);

    // 检查技能是否可以被激活
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")  // 允许蓝图子类选择性地覆盖（Override）
    bool CanActivate();
    // 作为 BlueprintNativeEvent的C++默认实现
    virtual bool CanActivate_Implementation();

    // 激活技能
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
    void Activate();
    virtual void Activate_Implementation();

protected:
    // ========================================
    // Effect 系统核心函数
    // ========================================

    /**
     * 执行技能的所有 Effect
     * @return 是否至少有一个 Effect 成功执行
     */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool ExecuteSkillEffects();

    /**
     * 获取目标网格坐标（根据 TargetType）
     * @return 目标格子坐标
     */
    UFUNCTION(BlueprintPure, Category = "Skill")
    virtual FIntPoint GetTargetGrid() const;

    /**
     * 获取受影响的角色列表
     * @param TargetGrid 目标格子
     * @return 受影响的角色数组
     */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual TArray<AActor*> GetAffectedActors(FIntPoint TargetGrid) const;

    /**
     * 获取 GridMovementComponent（缓存优化）
     */
    UFUNCTION(BlueprintPure, Category = "Skill")
    UGridMovementComponent* GetGridMovementComponent() const;

    // ========================================
    // 组件引用
    // ========================================

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TObjectPtr<ACharacter> OwnerCharacter;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TObjectPtr<const USkillDataAsset> SkillData;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TObjectPtr<USkillComponent> OwningComponent;

    // 添加一个缓存的属性组件指针，避免重复查找
    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TObjectPtr<UAttributesComponent> AttributesComp;
};
