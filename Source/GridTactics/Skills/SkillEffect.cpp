// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillEffect.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/AttributesComponent.h"
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

AGridManager* USkillEffect::GetGridManager() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    return Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
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