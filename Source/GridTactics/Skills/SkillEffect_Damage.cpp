#include "SkillEffect_Damage.h"
#include "GridTactics/AttributesComponent.h"
#include "GridTactics/GridManager.h"

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

    int32 DamagedCount = 0;

    for (AActor* Target : AffectedActors)
    {
        // 跳过施法者自己
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

    return DamagedCount > 0;
}