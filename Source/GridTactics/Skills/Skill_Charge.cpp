// Fill out your copyright notice in the Description page of Project Settings.


#include "Skill_Charge.h"
#include "GameFramework/Character.h"
#include "GridTactics/SkillDataAsset.h"
#include "GridTactics/GridMovementComponent.h"
#include "GridTactics/AttributesComponent.h"
#include "GridTactics/HeroCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

void USkill_Charge::Activate_Implementation() {
	if (!CanActivate_Implementation()) return;

	if (AttributesComp) {
		AttributesComp->ConsumeStamina(SkillData->StaminaCost);
		AttributesComp->ConsumeMP(SkillData->MPCost);
	}
	UE_LOG(LogTemp, Warning, TEXT("Charge Skill '%s' Activated!"), *SkillData->SkillName.ToString());

	// 获取移动组件并计算目标位置
	UGridMovementComponent* MovementComp = OwnerCharacter->FindComponentByClass<UGridMovementComponent>();
	if (!MovementComp) return;

	// 如果技能没有设置位移距离，则直接在原地造成伤害
	if (SkillData->MovementDistance <= 0)
	{
		Super::Activate_Implementation(); // 直接调用父类的范围伤害逻辑
		return;
	}

	// 计算冲锋方向 (基于角色当前朝向)
	const FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
	const int32 DeltaX = FMath::RoundToInt(ForwardVector.X);
	const int32 DeltaY = FMath::RoundToInt(ForwardVector.Y);

	// 执行位移
		// 我们直接调用GridMovementComponent的函数来移动角色
		// 注意：这里我们假设TryMoveOneStep可以被连续调用来完成长距离移动
		// 为了实现更平滑的冲锋，未来可以考虑在GridMovementComponent中增加一个DashToGrid的专用函数
		for (int32 i = 0; i < SkillData->MovementDistance; ++i)
		{
			// 尝试向目标方向移动一格
			// 如果中途遇到障碍物或敌人，移动会失败，冲锋会停在障碍物前
			if (!MovementComp->TryMoveOneStep(DeltaX, DeltaY))
			{
				break; // 移动失败，停止冲锋
			}
		}

	// --- 4. 在新位置造成伤害 ---
	// 移动完成后，我们再执行范围伤害逻辑。
	// 这里的代码与BaseSkill::Activate_Implementation中的伤害部分几乎完全一样。
	// 为了代码复用，未来可以将其提取到一个独立的辅助函数中。
	TArray<FIntPoint> WorldGrids;
	if (auto GridTacticsChar = Cast<AHeroCharacter>(OwnerCharacter))
	{
		WorldGrids = GridTacticsChar->GetSkillRangeInWorld(SkillData->RangePattern);
	}
	else
	{
		// 处理AI等其他角色类型
	}

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TSet<AActor*> DamagedActors;

	for (const FIntPoint& Grid : WorldGrids)
	{
		FVector WorldLocation = MovementComp->GridToWorld(Grid.X, Grid.Y);
		TArray<AActor*> OverlappedActors;

		UKismetSystemLibrary::SphereOverlapActors(
			GetWorld(),
			WorldLocation,
			50.0f,
			ObjectTypes,
			nullptr,
			ActorsToIgnore,
			OverlappedActors
		);

		for (AActor* OverlappedActor : OverlappedActors)
		{
			if (!DamagedActors.Contains(OverlappedActor))
			{
				if (UAttributesComponent* TargetAttrs = OverlappedActor->FindComponentByClass<UAttributesComponent>())
				{
					TargetAttrs->ApplyDamage(SkillData->Damage);
					DamagedActors.Add(OverlappedActor);
				}
			}
		}
	}
}
