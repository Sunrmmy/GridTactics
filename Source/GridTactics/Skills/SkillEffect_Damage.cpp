#include "SkillEffect_Damage.h"
#include "GridTactics/AttributesComponent.h"
#include "GridTactics/GridMovement/GridManager.h"

USkillEffect_Damage::USkillEffect_Damage()
{
    EffectName = FText::FromString(TEXT("Damage"));
    
    // 默认启用实时范围检测（针对延迟伤害）
    bRecheckRangeOnExecution = true;
}

bool USkillEffect_Damage::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    if (!HasAuthority(Instigator))
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Damage: Not authorized to execute"));
        return false;
    }

    // 如果启用了实时检测，则重新获取范围内的目标
    TArray<AActor*> TargetsToHit;
    
    if (bRecheckRangeOnExecution && ExecutionDelay > 0.0f)
    {
        // 重新检测当前在范围内的目标
        TargetsToHit = RecheckAffectedActors(Instigator, TargetGrid);
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Damage: Rechecked range, found %d targets (was %d at cast time)"), 
            TargetsToHit.Num(), AffectedActors.Num());
    }
    else
    {
        // 使用技能激活时的目标列表
        TargetsToHit = AffectedActors;
        UE_LOG(LogTemp, Log, TEXT("SkillEffect_Damage: Using original targets (%d actors)"), AffectedActors.Num());
    }

    int32 DamagedCount = 0;

    for (AActor* Target : TargetsToHit)
    {
        // 添加有效性检查
        if (!IsValid(Target))
        {
            UE_LOG(LogTemp, Warning, TEXT("  Skipping invalid/destroyed actor"));
            continue;
        }

        // 跳过施法者自己（除非允许自伤）
        if (Target == Instigator && !bDamageSelf)
        {
            continue;
        }

        // 获取目标的属性组件
        UAttributesComponent* TargetAttrs = GetAttributesComponent(Target);
        if (!TargetAttrs)
        {
            continue;
        }

        // 计算实际伤害（可选：距离衰减）
        float FinalDamage = BaseDamage;

        //if (DistanceFalloff > 0.0f)
        //{
        //    // 根据 TargetGrid 和目标位置计算距离衰减
        //}

        // 应用伤害
        TargetAttrs->ApplyDamage(FinalDamage);
        DamagedCount++;

        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Damage: Dealt %.1f damage to %s (Delay: %.2fs, Recheck: %s)"), 
            FinalDamage, 
            *Target->GetName(),
            ExecutionDelay,
            bRecheckRangeOnExecution ? TEXT("YES") : TEXT("NO"));
    }

    if (TargetsToHit.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Damage: No targets in range at execution time"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("SkillEffect_Damage: Successfully damaged %d/%d targets"), 
            DamagedCount, TargetsToHit.Num());
    }

    return true;
}