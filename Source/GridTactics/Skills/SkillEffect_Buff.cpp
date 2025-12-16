// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillEffect_Buff.h"
#include "GridTactics/AttributesComponent.h"
#include "GridTactics/HeroCharacter.h"
#include "GridTactics/EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Character.h"

USkillEffect_Buff::USkillEffect_Buff()
{
    EffectName = FText::FromString(TEXT("Buff"));
}

bool USkillEffect_Buff::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
	if (!HasAuthority(Instigator))
	{
		UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Buff: Not authorized to execute"));
		return false;
	}

	if (Modifiers.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("SkillEffect_Buff: No modifiers configured! Buff '%s' has nothing to apply."), 
			*BuffName.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("========== SkillEffect_Buff: Applying '%s' =========="), 
		*BuffName.ToString());

	int32 BuffedCount = 0;
	int32 FilteredCount = 0;
	int32 RefreshedCount = 0;
	int32 DirectAppliedCount = 0;  // 新增：直接应用的计数器（如护盾）

	for (AActor* Target : AffectedActors)
	{
		// 1. 检查目标有效性
		if (!IsValid(Target))
		{
			UE_LOG(LogTemp, Warning, TEXT("  Skipping invalid/destroyed actor"));
			continue;
		}

		// 2. 目标过滤
		if (!IsValidTarget(Target, Instigator))
		{
			FilteredCount++;
			UE_LOG(LogTemp, Verbose, TEXT("  Filtered out %s (Filter: %d)"), 
				*Target->GetName(), (int32)TargetFilter);
			continue;
		}

		// 3. 获取目标的属性组件
		UAttributesComponent* TargetAttrs = GetAttributesComponent(Target);
		if (!TargetAttrs)
		{
			UE_LOG(LogTemp, Warning, TEXT("  %s has no AttributesComponent, skipping"), 
				*Target->GetName());
			continue;
		}

		// 4. 应用所有 Modifier
		int32 AppliedModifiers = 0;
		for (FAttributeModifier Modifier : Modifiers)
		{
			// 特殊处理：Shield 直接添加护盾值，而不是作为 Modifier
			if (Modifier.AttributeToModify == EAttributeType::Shield)
			{
				// 直接调用 AddShield 方法
				TargetAttrs->AddShield(Modifier.Value);
				DirectAppliedCount++;
				AppliedModifiers++;
				
				UE_LOG(LogTemp, Warning, TEXT("    Directly added %.1f shield to %s (Current: %.1f / %.1f)"), 
					Modifier.Value, 
					*Target->GetName(),
					TargetAttrs->GetShield(),
					TargetAttrs->GetMaxShield());
				
				continue;  // 跳过添加到 Modifier 列表
			}

			// 生成唯一 ID
			Modifier.ID = FGuid::NewGuid();
			
			// 改进：检查堆叠逻辑（其他属性）
			if (!bAllowStacking && HasSameBuff(TargetAttrs, Modifier))
			{
				if (bRefreshDurationOnReapply)
				{
					// 移除旧的同类型 Buff
					TargetAttrs->RemoveModifiersForAttributeAndType(
						Modifier.AttributeToModify, 
						Modifier.Type
					);
					
					// 应用新 Buff（刷新持续时间）
					TargetAttrs->AddAttributeModifier(Modifier);
					AppliedModifiers++;
					RefreshedCount++;

					UE_LOG(LogTemp, Log, TEXT("    Refreshed %s %s on %s (Duration: %.1fs)"), 
						*UEnum::GetValueAsString(Modifier.AttributeToModify),
						Modifier.Type == EModifierType::Additive ? TEXT("Add") : TEXT("Mult"),
						*Target->GetName(),
						Modifier.Duration);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("    %s already has %s %s, skipping"), 
						*Target->GetName(),
						*UEnum::GetValueAsString(Modifier.AttributeToModify),
						Modifier.Type == EModifierType::Additive ? TEXT("Add") : TEXT("Mult"));
					continue;
				}
			}
			else
			{
				// 正常应用 Modifier
				TargetAttrs->AddAttributeModifier(Modifier);
				AppliedModifiers++;

				// 详细日志
				FString ModifierTypeStr = (Modifier.Type == EModifierType::Additive) ? TEXT("Add") : TEXT("Mult");
				FString DurationStr = (Modifier.Duration > 0) 
					? FString::Printf(TEXT("%.1fs"), Modifier.Duration) 
					: TEXT("Permanent");

				UE_LOG(LogTemp, Log, TEXT("    Applied: %s %s %.2f (%s)"), 
					*UEnum::GetValueAsString(Modifier.AttributeToModify),
					*ModifierTypeStr,
					Modifier.Value,
					*DurationStr);
			}
		}

		if (AppliedModifiers > 0)
		{
			BuffedCount++;
			
			// 播放视觉反馈（如果取消注释）
			// PlayFeedback(Target);

			UE_LOG(LogTemp, Warning, TEXT("  Buffed %s with %d modifiers"), 
				*Target->GetName(), AppliedModifiers);
		}
	}

	// 总结日志
	if (BuffedCount > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Buff: Successfully buffed %d/%d targets (filtered: %d, refreshed: %d, direct: %d)"), 
			BuffedCount, AffectedActors.Num(), FilteredCount, RefreshedCount, DirectAppliedCount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Buff: No valid targets buffed (Total: %d, Filtered: %d)"), 
			AffectedActors.Num(), FilteredCount);
	}

	UE_LOG(LogTemp, Warning, TEXT("========== SkillEffect_Buff: Complete =========="));

	return BuffedCount > 0;
}

bool USkillEffect_Buff::IsValidTarget(AActor* Target, AActor* Instigator) const
{
    if (!Target || !Instigator)
    {
        return false;
    }

    switch (TargetFilter)
    {
    case EBuffTargetFilter::Self:
        // 只对施法者自己
        return Target == Instigator;

    case EBuffTargetFilter::Allies:
    {
        // 对友军（包括自己，除非 bApplyToSelf = false）
        if (Target == Instigator)
        {
            return bApplyToSelf;
        }

        // 判断是否为友军（简化实现：同类型视为友军）
        bool bInstigatorIsHero = Cast<AHeroCharacter>(Instigator) != nullptr;
        bool bTargetIsHero = Cast<AHeroCharacter>(Target) != nullptr;

        return bInstigatorIsHero == bTargetIsHero;
    }

    case EBuffTargetFilter::Enemies:
    {
        // 对敌军（不同类型）
        if (Target == Instigator)
        {
            return false; // 敌军过滤不包括自己
        }

        bool bInstigatorIsHero = Cast<AHeroCharacter>(Instigator) != nullptr;
        bool bTargetIsHero = Cast<AHeroCharacter>(Target) != nullptr;

        return bInstigatorIsHero != bTargetIsHero;
    }

    case EBuffTargetFilter::All:
    default:
        // 对所有角色
        return true;
    }
}

bool USkillEffect_Buff::HasSameBuff(UAttributesComponent* TargetAttrs, const FAttributeModifier& Modifier) const
{
	if (!TargetAttrs)
	{
		return false;
	}

	// 检查是否已有相同属性和类型的修改器
	bool bHasSameModifier = TargetAttrs->HasModifierForAttributeAndType(
		Modifier.AttributeToModify, 
		Modifier.Type
	);

	if (bHasSameModifier)
	{
		UE_LOG(LogTemp, Verbose, TEXT("  Target already has %s modifier for attribute %s"), 
			Modifier.Type == EModifierType::Additive ? TEXT("Additive") : TEXT("Multiplicative"),
			*UEnum::GetValueAsString(Modifier.AttributeToModify));
	}

	return bHasSameModifier;
}

//void USkillEffect_Buff::PlayFeedback(AActor* Target)
//{
//    if (!Target)
//    {
//        return;
//    }
//
//    // 播放音效
//    if (BuffApplySound)
//    {
//        UGameplayStatics::PlaySoundAtLocation(
//            Target->GetWorld(),
//            BuffApplySound,
//            Target->GetActorLocation(),
//            1.0f,
//            1.0f
//        );
//
//        UE_LOG(LogTemp, Verbose, TEXT("  Played buff sound for %s"), *Target->GetName());
//    }
//
//    // 播放粒子特效
//    if (BuffApplyVFX)
//    {
//        ACharacter* Character = Cast<ACharacter>(Target);
//        
//        if (Character && VFXAttachSocketName != NAME_None)
//        {
//            // 附着在角色的 Socket 上
//            UGameplayStatics::SpawnEmitterAttached(
//                BuffApplyVFX,
//                Character->GetMesh(),
//                VFXAttachSocketName,
//                FVector::ZeroVector,
//                FRotator::ZeroRotator,
//                EAttachLocation::SnapToTarget,
//                true
//            );
//
//            UE_LOG(LogTemp, Verbose, TEXT("  Spawned buff VFX attached to %s at socket '%s'"), 
//                *Target->GetName(), *VFXAttachSocketName.ToString());
//        }
//        else
//        {
//            // 在目标位置生成
//            UGameplayStatics::SpawnEmitterAtLocation(
//                Target->GetWorld(),
//                BuffApplyVFX,
//                Target->GetActorLocation(),
//                FRotator::ZeroRotator,
//                true
//            );
//
//            UE_LOG(LogTemp, Verbose, TEXT("  Spawned buff VFX at %s location"), *Target->GetName());
//        }
//    }
//}
