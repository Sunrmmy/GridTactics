//// Fill out your copyright notice in the Description page of Project Settings.
//
//#include "Skill_Charge.h"
//#include "GridTactics/GridManager.h"
//#include "GridTactics/GridDisplacementRequest.h"
//#include "GridTactics/SkillDataAsset.h"
//#include "GridTactics/GridMovementComponent.h"
//#include "GridTactics/AttributesComponent.h"
//#include "GridTactics/HeroCharacter.h"
//#include "Kismet/KismetSystemLibrary.h"
//#include "Kismet/GameplayStatics.h"
//#include "GameFramework/PlayerController.h"
//
//USkill_Charge::USkill_Charge()
//{
//    // 全部从 SkillDataAsset 获取
//}
//
//bool USkill_Charge::CanActivate_Implementation()
//{
//    // 调用父类的基础检查
//    if (!Super::CanActivate_Implementation())
//    {
//        return false;
//    }
//
//    // 检查是否正在移动
//    if (UGridMovementComponent* MovementComp = OwnerCharacter->FindComponentByClass<UGridMovementComponent>())
//    {
//        if (MovementComp->IsMoving() || MovementComp->IsExecutingDisplacement())
//        {
//            UE_LOG(LogTemp, Warning, TEXT("Skill_Charge: Cannot activate while moving!"));
//            return false;
//        }
//    }
//
//    // 检查是否有有效的冲刺方向
//    FIntPoint ChargeDir = GetChargeDirection();
//    if (ChargeDir == FIntPoint::ZeroValue)
//    {
//        UE_LOG(LogTemp, Warning, TEXT("Skill_Charge: Invalid charge direction!"));
//        return false;
//    }
//
//    // 检查 SkillDataAsset 是否有效
//    if (!SkillData)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Skill_Charge: SkillData is null!"));
//        return false;
//    }
//
//    return true;
//}
//
//void USkill_Charge::Activate_Implementation()
//{
//    if (!CanActivate_Implementation())
//    {
//        UE_LOG(LogTemp, Warning, TEXT("Skill_Charge: CanActivate failed!"));
//        return;
//    }
//
//    // 获取 GridManager
//    AGridManager* GridMgr = GetGridManager();
//    if (!GridMgr)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Skill_Charge: GridManager not found!"));
//        return;
//    }
//
//    // 检查 SkillDataAsset
//    if (!SkillData)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Skill_Charge: SkillData is null!"));
//        return;
//    }
//
//    // 调用父类的基础逻辑（消耗资源、触发冷却）
//    Super::Activate_Implementation();
//
//    // 计算冲刺方向
//    FIntPoint ChargeDir = GetChargeDirection();
//    if (ChargeDir == FIntPoint::ZeroValue)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Skill_Charge: Failed to determine charge direction!"));
//        return;
//    }
//
//    // 从 SkillDataAsset 获取技能参数
//    const int32 DashDistance = SkillData->DashDistance;
//    const int32 KnockbackDistance = SkillData->KnockbackDistance;
//    const bool bStopOnCollision = SkillData->bStopOnCollision;
//    const float ChargeDuration = SkillData->ChargeDuration;
//
//    UE_LOG(LogTemp, Log, TEXT("✓ Skill_Charge: Charging in direction %s for %d grids (Knockback: %d)"),
//        *ChargeDir.ToString(), DashDistance, KnockbackDistance);
//
//    // 简化后的冲刺请求
//    GridMgr->RequestDash(
//        OwnerCharacter,
//        ChargeDir,
//        DashDistance,
//        true,
//        KnockbackDistance
//    );
//
//    GridMgr->ProcessDisplacements();
//
//    // 播放冲刺音效/特效
//    // UGameplayStatics::PlaySoundAtLocation(GetWorld(), ChargeSound, OwnerCharacter->GetActorLocation());
//}
//
//// ========================================
//// 辅助函数实现
//// ========================================
//
//AGridManager* USkill_Charge::GetGridManager() const
//{
//    if (!OwnerCharacter || !OwnerCharacter->GetWorld())
//    {
//        return nullptr;
//    }
//
//    return Cast<AGridManager>(
//        UGameplayStatics::GetActorOfClass(OwnerCharacter->GetWorld(), AGridManager::StaticClass())
//    );
//}
//
//FIntPoint USkill_Charge::GetChargeDirection() const
//{
//    if (!OwnerCharacter)
//    {
//        return FIntPoint::ZeroValue;
//    }
//
//    // 优先使用鼠标方向（如果有玩家控制器）
//    if (OwnerCharacter->IsPlayerControlled())
//    {
//        FIntPoint MouseDir = GetDirectionFromMouse();
//        if (MouseDir != FIntPoint::ZeroValue)
//        {
//            return MouseDir;
//        }
//    }
//
//    // 回退方案：使用角色当前朝向
//    return GetDirectionFromRotation();
//}
//
//FIntPoint USkill_Charge::GetDirectionFromRotation() const
//{
//    if (!OwnerCharacter)
//    {
//        return FIntPoint::ZeroValue;
//    }
//
//    // 获取角色当前朝向
//    FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
//    FVector ForwardVector = CurrentRotation.Vector();
//
//    // 归一化为四方向
//    int32 DirX = 0;
//    int32 DirY = 0;
//
//    if (FMath::Abs(ForwardVector.X) > FMath::Abs(ForwardVector.Y))
//    {
//        // X 轴为主导
//        DirX = (ForwardVector.X > 0) ? 1 : -1;
//    }
//    else if (FMath::Abs(ForwardVector.Y) > 0.01f)
//    {
//        // Y 轴为主导
//        DirY = (ForwardVector.Y > 0) ? 1 : -1;
//    }
//
//    return FIntPoint(DirX, DirY);
//}
//
//FIntPoint USkill_Charge::GetDirectionFromMouse() const
//{
//    if (!OwnerCharacter)
//    {
//        return FIntPoint::ZeroValue;
//    }
//
//    // 获取玩家控制器
//    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
//    if (!PC)
//    {
//        return FIntPoint::ZeroValue;
//    }
//
//    // 获取鼠标在世界中的位置
//    FVector WorldLocation, WorldDirection;
//    PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
//
//    // 计算鼠标指向的地面位置
//    FVector PlaneOrigin = OwnerCharacter->GetActorLocation();
//    FVector MouseGroundPos = FMath::LinePlaneIntersection(
//        WorldLocation,
//        WorldLocation + WorldDirection * 10000.f,
//        PlaneOrigin,
//        FVector::UpVector
//    );
//
//    // 计算从角色到鼠标位置的方向向量
//    FVector DirToMouse = (MouseGroundPos - OwnerCharacter->GetActorLocation()).GetSafeNormal();
//
//    // 离散化为四个主方向之一
//    int32 DirX = 0;
//    int32 DirY = 0;
//
//    if (FMath::Abs(DirToMouse.X) > FMath::Abs(DirToMouse.Y))
//    {
//        // X 轴为主导方向
//        DirX = (DirToMouse.X > 0) ? 1 : -1;
//        DirY = 0;
//    }
//    else if (FMath::Abs(DirToMouse.Y) > 0.01f)
//    {
//        // Y 轴为主导方向
//        DirX = 0;
//        DirY = (DirToMouse.Y > 0) ? 1 : -1;
//    }
//    else
//    {
//        // 方向向量太小，无法确定方向
//        return FIntPoint::ZeroValue;
//    }
//
//    return FIntPoint(DirX, DirY);
//}