// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillEffect_Teleport.h"
#include "GridTactics/GridManager.h"

USkillEffect_Teleport::USkillEffect_Teleport()
{
    EffectName = FText::FromString(TEXT("Teleport"));
}

bool USkillEffect_Teleport::CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const
{
    //AGridManager* GridMgr = GetGridManager();
    //if (!GridMgr)
    //{
    //    return false;
    //}

    //// 检查目标格子是否有效且可行走
    //if (!GridMgr->IsGridValid(TargetGrid) || !GridMgr->IsGridWalkable(TargetGrid))
    //{
    //    return false;
    //}

    //// 检查目标格子是否被占用
    //AActor* OccupyingActor = GridMgr->GetActorAtGrid(TargetGrid);
    //if (OccupyingActor && OccupyingActor != Instigator)
    //{
    //    return false;
    //}

    // 调试 1：检查 GridManager
    AGridManager* GridMgr = GetGridManager(Instigator);
    if (!GridMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect_Teleport::CanExecute - GridManager not found!"));
        return false;
    }

    // 调试 2：检查 TargetGrid 有效性
    bool bIsValid = GridMgr->IsGridValid(TargetGrid);
    if (!bIsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Teleport::CanExecute - TargetGrid %s is INVALID"), *TargetGrid.ToString());
        return false;
    }

    // 调试 3：检查可行走性
    bool bIsWalkable = GridMgr->IsGridWalkable(TargetGrid);
    if (!bIsWalkable)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Teleport::CanExecute - TargetGrid %s is NOT WALKABLE"), *TargetGrid.ToString());
        return false;
    }

    // 调试 4：检查占用情况
    AActor* OccupyingActor = GridMgr->GetActorAtGrid(TargetGrid);
    if (OccupyingActor && OccupyingActor != Instigator)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Teleport::CanExecute - TargetGrid %s is OCCUPIED by %s"),
            *TargetGrid.ToString(),
            *OccupyingActor->GetName());
        return false;
    }

    // 调试 5：输出成功信息
    UE_LOG(LogTemp, Log, TEXT("SkillEffect_Teleport::CanExecute - Passed! Instigator: %s, TargetGrid: %s"),
        *Instigator->GetName(),
        *TargetGrid.ToString());

    return true;
}

bool USkillEffect_Teleport::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    if (!HasAuthority(Instigator))
    {
        return false;
    }

    AGridManager* GridMgr = GetGridManager(Instigator);
    if (!GridMgr)
    {
        return false;
    }

    // 提交传送请求
    GridMgr->RequestTeleport(Instigator, TargetGrid);
    GridMgr->ProcessDisplacements();

    UE_LOG(LogTemp, Log, TEXT("SkillEffect_Teleport: %s teleported to %s"), *Instigator->GetName(), *TargetGrid.ToString());

    return true;
}
