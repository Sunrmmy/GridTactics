// Fill out your copyright notice in the Description page of Project Settings.


#include "Skill_Charge.h"
#include "GridTactics/SkillDataAsset.h"
#include "GridTactics/GridMovementComponent.h"
#include "GridTactics/AttributesComponent.h"
#include "GridTactics/HeroCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

void USkill_Charge::Activate_Implementation()
{
	if (!OwnerCharacter || !SkillData) return;

	UGridMovementComponent* GridComp = OwnerCharacter->FindComponentByClass<UGridMovementComponent>();
	if (!GridComp)
	{
		return;
	}

	// 确定冲锋方向 (基于角色当前Actor Forward Vector)
	FVector Forward = OwnerCharacter->GetActorForwardVector();

	int32 DirX = 0;
	int32 DirY = 0;

	// 简单的向量转网格方向逻辑
	if (FMath::Abs(Forward.X) > FMath::Abs(Forward.Y))
	{
		DirX = (Forward.X > 0) ? 1 : -1;
	}
	else
	{
		DirY = (Forward.Y > 0) ? 1 : -1;
	}
	FIntPoint ChargeDirection(DirX, DirY); // 冲锋方向单位向量

	// 获取起始位置和冲锋距离
	int32 StartX, StartY;
	GridComp->GetCurrentGrid(StartX, StartY);
	FIntPoint CurrentGrid(StartX, StartY);

	int32 MaxDistance = SkillData->MovementDistance; // 从SkillDataAsset中读取配置的移动距离
	if (MaxDistance < 0) MaxDistance = 0; // 默认值保护

	FIntPoint TargetGrid = CurrentGrid;

	// 路径遍历检测，预先计算路径，处理碰撞逻辑
	for (int32 i = 1; i <= MaxDistance; ++i)
	{
		FIntPoint CheckGrid = CurrentGrid + ChargeDirection * i;

		// 检查Block网格
		if (!GridComp->IsGridWalkable(CheckGrid.X, CheckGrid.Y))
		{
			// 遇到墙壁，停止检测，最终位置保持在上一格
			UE_LOG(LogTemp, Log, TEXT("Charge hit wall at step %d"), i);
			break;
		}

		// 检测该格子是否有敌人 (需要GridMovementComponent实现GetActorAtGrid)
		AActor* HitActor = GridComp->GetActorAtGrid(CheckGrid.X, CheckGrid.Y);

		if (HitActor && HitActor != OwnerCharacter)
		{
			// 造成伤害
			if (UAttributesComponent* TargetAttrs = HitActor->FindComponentByClass<UAttributesComponent>())
			{
				TargetAttrs->ApplyDamage(SkillData->Damage);
			}

			// 击退敌人 (向冲锋方向击退)
			if (UGridMovementComponent* TargetGridComp = HitActor->FindComponentByClass<UGridMovementComponent>())
			{
				// 调用击退接口，参数为击退方向
				TargetGridComp->ReceiveKnockback(ChargeDirection, 4);
			}

			// 如果该格子可行走（即使有敌人，因为敌人被推走了），我们视为可通过
			TargetGrid = CheckGrid;
			UE_LOG(LogTemp, Log, TEXT("Charge hit actor: %s at (%d, %d)"), *HitActor->GetName(), CheckGrid.X, CheckGrid.Y);
		}
		TargetGrid = CheckGrid;
	}

	// 执行角色自身的强制位移
	if (TargetGrid != CurrentGrid)
	{
		// 计算位移所需时间，距离越远时间越长，保持冲锋速度感
		float Duration = 0.15f * FMath::Sqrt((float)MaxDistance);
		GridComp->ExecuteForcedMove(TargetGrid, Duration);
	}
}
