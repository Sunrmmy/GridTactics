// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillEffect_Buff.h"
#include "GridTactics/AttributesComponent.h"

USkillEffect_Buff::USkillEffect_Buff()
{
    EffectName = FText::FromString(TEXT("Buff"));
}

bool USkillEffect_Buff::Execute_Implementation(AActor* Instigator, FIntPoint TargetGrid, const TArray<AActor*>& AffectedActors)
{
    int32 BuffedCount = 0;

    for (AActor* Target : AffectedActors)
    {
        if (Target == Instigator && !bApplyToSelf)
        {
            continue;
        }

        UAttributesComponent* TargetAttrs = GetAttributesComponent(Target);
        if (!TargetAttrs)
        {
            continue;
        }

        // 应用所有 Modifier
        for (const FAttributeModifier& Modifier : Modifiers)
        {
            TargetAttrs->AddAttributeModifier(Modifier);
        }

        BuffedCount++;
        UE_LOG(LogTemp, Log, TEXT("SkillEffect_Buff: Applied %d modifiers to %s"), Modifiers.Num(), *Target->GetName());
    }

    return BuffedCount > 0;
}
