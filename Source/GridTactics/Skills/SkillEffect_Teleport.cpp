// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillEffect_Teleport.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/GridMovementComponent.h"
#include "GameFramework/Character.h"

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

    // 根据传送模式选择不同的执行方式
    if (TeleportMode == ETeleportMode::Instant)
    {
        // ========================================
        // 瞬移模式：直接设置位置
        // ========================================

        FVector TargetWorldPos = GridMgr->GridToWorld(TargetGrid);
        FVector CurrentPos = Instigator->GetActorLocation();

        // 应用起始高度（仅用于特效生成位置，瞬移后立即落到目标位置）
        FVector FinalPos = FVector(
            TargetWorldPos.X,
            TargetWorldPos.Y,
            CurrentPos.Z + EndHeightOffset  // 目标高度
        );

        // 直接设置位置
        Instigator->SetActorLocation(FinalPos);

        // 对齐旋转到四向
        if (UGridMovementComponent* MovementComp = Instigator->FindComponentByClass<UGridMovementComponent>())
        {
            FRotator CurrentRotation = Instigator->GetActorRotation();
            FRotator SnappedRotation = UGridMovementComponent::SnapRotationToFourDirections(CurrentRotation);
            Instigator->SetActorRotation(SnappedRotation);
            MovementComp->SetTargetRotation(SnappedRotation);
        }

        UE_LOG(LogTemp, Log, TEXT("SkillEffect_Teleport: %s instantly teleported to %s"), *Instigator->GetName(), *TargetGrid.ToString());
    }
    else if (TeleportMode == ETeleportMode::Smooth)
    {
        // ========================================
        // 平滑模式直接调用自定义移动
        // ========================================

        UGridMovementComponent* MovementComp = Instigator->FindComponentByClass<UGridMovementComponent>();
        if (!MovementComp)
        {
            UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Teleport: No GridMovementComponent on %s"), *Instigator->GetName());
            return false;
        }

        // 获取起始和目标位置
        FVector StartPos = Instigator->GetActorLocation();
        FVector TargetWorldPos = GridMgr->GridToWorld(TargetGrid);

        // 生成路径
        TArray<FIntPoint> Path;
        FIntPoint StartGrid = GridMgr->WorldToGrid(StartPos);
        
        if (bUseParabolicArc && TeleportDuration > 0.0f)
        {
            // 抛物线路径：添加中间点
            Path.Add(StartGrid);

            FIntPoint MidGrid = FIntPoint(
                (StartGrid.X + TargetGrid.X) / 2,
                (StartGrid.Y + TargetGrid.Y) / 2
            );
            Path.Add(MidGrid);

            Path.Add(TargetGrid);
        }
        else
        {
            // 简单两点路径
            Path.Add(StartGrid);
            Path.Add(TargetGrid);
        }

        // 执行自定义位移
        MovementComp->ExecuteDisplacementPathWithHeight(
            Path,
            TeleportDuration,
            StartHeightOffset,
            EndHeightOffset,
            bUseParabolicArc ? ArcPeakHeight : 0.0f
        );

        UE_LOG(LogTemp, Log, TEXT("SkillEffect_Teleport: %s smoothly teleporting to %s over %.2fs"),
            *Instigator->GetName(),
            *TargetGrid.ToString(),
            TeleportDuration);
    }

    return true;
}

void USkillEffect_Teleport::ExecuteCustomTeleportMovement(AActor* Instigator, FIntPoint TargetGrid, AGridManager* GridMgr)
{
    if (!Instigator)
    {
        return;
    }

    UGridMovementComponent* MovementComp = Instigator->FindComponentByClass<UGridMovementComponent>();
    if (!MovementComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Teleport: No GridMovementComponent on %s"), *Instigator->GetName());
        return;
    }

    // 获取起始和目标位置
    FVector StartPos = Instigator->GetActorLocation();
    FVector TargetWorldPos = GridMgr->GridToWorld(TargetGrid);

    // 应用高度偏移
    FVector AdjustedStartPos = StartPos + FVector(0, 0, StartHeightOffset);
    FVector AdjustedEndPos = FVector(TargetWorldPos.X, TargetWorldPos.Y, StartPos.Z + EndHeightOffset);

    // 生成路径（如果启用抛物线，添加中间点）
    TArray<FIntPoint> Path;
    
    if (bUseParabolicArc && TeleportDuration > 0.0f)
    {
        // 生成抛物线路径（添加中间点）
        FIntPoint StartGrid = GridMgr->WorldToGrid(StartPos);
        Path.Add(StartGrid);

        // 在中点添加高度
        FIntPoint MidGrid = FIntPoint(
            (StartGrid.X + TargetGrid.X) / 2,
            (StartGrid.Y + TargetGrid.Y) / 2
        );
        Path.Add(MidGrid);  // 中间点会在 ExecuteDisplacementPath 中被赋予抛物线高度

        Path.Add(TargetGrid);
    }
    else
    {
        // 简单的两点路径
        FIntPoint StartGrid = GridMgr->WorldToGrid(StartPos);
        Path.Add(StartGrid);
        Path.Add(TargetGrid);
    }

    // 执行自定义位移（需要扩展 GridMovementComponent）
    MovementComp->ExecuteDisplacementPathWithHeight(
        Path,
        TeleportDuration,
        StartHeightOffset,
        EndHeightOffset,
        bUseParabolicArc ? ArcPeakHeight : 0.0f
    );
}
