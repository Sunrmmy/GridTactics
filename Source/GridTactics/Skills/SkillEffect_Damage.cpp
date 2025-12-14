#include "SkillEffect_Damage.h"
#include "GridTactics/AttributesComponent.h"
#include "GridTactics/GridMovement/GridManager.h"

USkillEffect_Damage::USkillEffect_Damage()
{
    EffectName = FText::FromString(TEXT("Damage"));
}

bool USkillEffect_Damage::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    if (!HasAuthority(Instigator))
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Damage: Not authorized to execute"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("SkillEffect_Damage: Processing %d affected actors"), AffectedActors.Num());

    int32 DamagedCount = 0;

    for (AActor* Target : AffectedActors)
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

        UE_LOG(LogTemp, Log, TEXT("SkillEffect_Damage: Dealt %.1f damage to %s"), FinalDamage, *Target->GetName());
    }

    // 修复：即使没有造成伤害，也返回 true（技能本身执行成功）
    // 只有在技术性错误时才返回 false
    if (AffectedActors.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillEffect_Damage: No targets in range"));
    }

    return true;  // 技能执行成功，即使没有命中目标
}