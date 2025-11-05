// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseSkill.generated.h"

class AHeroCharacter;
class USkillDataAsset;
UCLASS(Blueprintable, BlueprintType, Abstract)  //Blueprintable, BlueprintType 通常用于可配置的“Buff 效果类”，既希望被蓝图继承（扩展新 Buff），又希望作为变量传入函数
class GRIDTACTICS_API UBaseSkill : public UObject
{
	GENERATED_BODY()
	
public:
    // 初始化技能，由SkillComponent调用
    virtual void Initialize(AHeroCharacter* InOwner, const USkillDataAsset* InSkillData);

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
    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TObjectPtr<AHeroCharacter> OwnerCharacter;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TObjectPtr<const USkillDataAsset> SkillData;
};
