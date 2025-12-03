// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillEffect_Dash.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/GridMovementComponent.h"
#include "GridTactics/AttributesComponent.h"
#include "Kismet/GameplayStatics.h"

USkillEffect_Dash::USkillEffect_Dash()
{
    EffectName = FText::FromString(TEXT("Dash"));
}

bool USkillEffect_Dash::CanExecute_Implementation(AActor* Instigator, FIntPoint TargetGrid) const
{
    // 调试 1：检查 GridManager
    AGridManager* GridMgr = GetGridManager(Instigator);
    if (!GridMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect_Dash::CanExecute - GridManager not found!"));
        return false;
    }

    // 调试 2：检查 Instigator
    if (!Instigator)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect_Dash::CanExecute - Instigator is null!"));
        return false;
    }

    // 调试 3：检查 GridMovementComponent
    UGridMovementComponent* MovementComp = Instigator->FindComponentByClass<UGridMovementComponent>();
    if (!MovementComp)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect_Dash::CanExecute - No GridMovementComponent on %s"), *Instigator->GetName());
        return false;
    }

    // 调试 4：输出成功信息
    UE_LOG(LogTemp, Log, TEXT("SkillEffect_Dash::CanExecute - Passed! Instigator: %s, TargetGrid: %s"),
        *Instigator->GetName(),
        *TargetGrid.ToString());


    //AGridManager* GridMgr = GetGridManager();
    //if (!GridMgr || !Instigator)
    //{
    //    return false;
    //}

    //// 只检查 GridManager 是否存在和 Instigator 是否有 GridMovementComponent
    //UGridMovementComponent* MovementComp = Instigator->FindComponentByClass<UGridMovementComponent>();
    //if (!MovementComp)
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Dash: No GridMovementComponent on %s"), *Instigator->GetName());
    //    return false;
    //}

    // 不验证目标格子，由 GridManager 的 PathPlanner 处理
    return true;
}

bool USkillEffect_Dash::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    if (!HasAuthority(Instigator))
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Dash: Not authorized"));
        return false;
    }

    AGridManager* GridMgr = GetGridManager(Instigator);
    if (!GridMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("SkillEffect_Dash: GridManager not found"));
        return false;
    }

    // 获取施法者当前位置
    FIntPoint StartGrid = GridMgr->GetActorCurrentGrid(Instigator);

    // 获取实际冲刺方向
    FIntPoint ActualDirection = GetActualDashDirection(Instigator);

    UE_LOG(LogTemp, Log, TEXT("SkillEffect_Dash: %s dashing from %s in direction %s for %d grids"),
        *Instigator->GetName(),
        *StartGrid.ToString(),
        *ActualDirection.ToString(),
        DashDistance);

    // 提交前记录冲刺路径上的敌人（用于碰撞伤害）
    TArray<AActor*> CollidedEnemies;
    
    if (bDamageCollidedEnemies && CollisionDamage > 0.0f)
    {
        // 预先检测路径上的敌人
        FIntPoint CheckPos = StartGrid;
        for (int32 i = 0; i < DashDistance; ++i)
        {
            CheckPos = CheckPos + ActualDirection;
            AActor* ActorAtGrid = GridMgr->GetActorAtGrid(CheckPos);
            if (ActorAtGrid && ActorAtGrid != Instigator)
            {
                CollidedEnemies.AddUnique(ActorAtGrid);
                
                // 如果碰到敌人且 bStopOnCollision = true，停止检测
                if (bStopOnCollision)
                {
                    break;
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("SkillEffect_Dash: Found %d enemies in path for collision damage"), CollidedEnemies.Num());
    }

    // 提交冲刺请求
    GridMgr->RequestDash(
        Instigator,
        ActualDirection,
        DashDistance,
        KnockbackDistance > 0,  // 如果 KnockbackDistance > 0 则启用击退
        KnockbackDistance
    );

    // 处理位移
    GridMgr->ProcessDisplacements();

    // 对碰撞的敌人造成伤害
    if (bDamageCollidedEnemies && CollisionDamage > 0.0f)
    {
        int32 DamagedCount = 0;
        for (AActor* Enemy : CollidedEnemies)
        {
            if (UAttributesComponent* Attrs = GetAttributesComponent(Enemy))
            {
                Attrs->ApplyDamage(CollisionDamage);
                UE_LOG(LogTemp, Log, TEXT("SkillEffect_Dash: Collision damage %.1f to %s"), 
                    CollisionDamage, *Enemy->GetName());
                DamagedCount++;
            }
        }

        if (DamagedCount > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("SkillEffect_Dash: Total damaged %d enemies"), DamagedCount);
        }
    }

    return true;
}

FIntPoint USkillEffect_Dash::GetActualDashDirection(AActor* Instigator) const
{
    if (!Instigator)
    {
        return DashDirection;
    }

    // 获取施法者的朝向
    FRotator ActorRotation = Instigator->GetActorRotation();

    // 将配置的相对方向转换为世界坐标方向
    FVector LocalDir(DashDirection.X, DashDirection.Y, 0);
    FVector WorldDir = ActorRotation.RotateVector(LocalDir);

    // 转换回网格坐标
    FIntPoint ActualDirection(
        FMath::RoundToInt(WorldDir.X),
        FMath::RoundToInt(WorldDir.Y)
    );

    return ActualDirection;
}

