// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillEffect.h"
#include "GridTactics/GridMovement/GridManager.h"
#include "GridTactics/AttributesComponent.h"
#include "BaseSkill.h"
#include "Kismet/GameplayStatics.h"

bool USkillEffect::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    // 基类默认实现什么都不做
    UE_LOG(LogTemp, Warning, TEXT("SkillEffect::Execute called on base class! Override this in subclass."));
    return false;
}

bool USkillEffect::CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const
{
    // 默认：总是可以执行
    return true;
}

AGridManager* USkillEffect::GetGridManager(AActor* WorldContextObject) const
{
    if (!WorldContextObject)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect::GetGridManager - WorldContextObject is null!"));
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect::GetGridManager - World is null!"));
        return nullptr;
    }

    AGridManager* GridMgr = Cast<AGridManager>(
        UGameplayStatics::GetActorOfClass(World, AGridManager::StaticClass())
    );

    if (!GridMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect::GetGridManager - No GridManager found in the world!"));
    }

    return GridMgr;
}

bool USkillEffect::HasAuthority(AActor* Instigator) const
{
    if (!Instigator)
    {
        return false;
    }

    // 单机模式下总是返回 true
    // 联机模式下检查是否在服务器
    return Instigator->HasAuthority();
}

UAttributesComponent* USkillEffect::GetAttributesComponent(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

    return Actor->FindComponentByClass<UAttributesComponent>();
}

// 重新检测范围内的目标
TArray<AActor*> USkillEffect::RecheckAffectedActors(AActor* Instigator, FIntPoint TargetGrid) const
{
    TArray<AActor*> AffectedActors;

    // 检查是否有有效的 OwningSkill 引用
    UBaseSkill* Skill = OwningSkill.Get();
    if (!Skill)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect::RecheckAffectedActors - OwningSkill is invalid! Effect: %s"), 
            *EffectName.ToString());
        return AffectedActors;
    }

    // 使用技能的 GetAffectedActors 方法重新检测
    AffectedActors = Skill->GetAffectedActors(TargetGrid);

    UE_LOG(LogTemp, Warning, TEXT("SkillEffect::RecheckAffectedActors - Effect '%s' rechecked range, found %d actors"), 
        *EffectName.ToString(),
        AffectedActors.Num());

    return AffectedActors;
}