// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillComponent.h"
#include "SkillDataAsset.h"
#include "GridManager.h"
#include "BaseSkill.h"
#include "HeroCharacter.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
USkillComponent::USkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AHeroCharacter>(GetOwner());
	if (!OwnerCharacter) return;

    // 基于数据资产，实例化技能逻辑对象
    for (const USkillDataAsset* DataAsset : EquippedSkillsData)
    {
        if (DataAsset && DataAsset->SkillLogicClass)
        {
            FSkillEntry NewEntry;
            NewEntry.SkillData = DataAsset;
            NewEntry.SkillInstance = NewObject<UBaseSkill>(this, DataAsset->SkillLogicClass);
            NewEntry.SkillInstance->Initialize(OwnerCharacter, DataAsset);
            SkillSlots.Add(NewEntry);
        }
    }
	
}

// Called every frame
void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 更新所有技能的冷却时间
    for (FSkillEntry& SkillEntry : SkillSlots)
    {
        if (SkillEntry.CooldownRemaining > 0.0f)
        {
            SkillEntry.CooldownRemaining = FMath::Max(0.0f, SkillEntry.CooldownRemaining - DeltaTime);
        }
    }

    // 新增：在 Aiming 状态下更新目标格子
    if (CurrentState == ESkillState::Aiming && OwnerCharacter)
    {
        // 从鼠标位置获取目标格子
        APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
        if (PC)
        {
            FVector WorldLocation, WorldDirection;
            PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);

            // 计算鼠标指向的地面位置
            FVector MouseGroundPos = FMath::LinePlaneIntersection(
                WorldLocation,
                WorldLocation + WorldDirection * 10000.f,
                FVector::ZeroVector,
                FVector::UpVector
            );

            // 转换为网格坐标
            if (AGridManager* GridMgr = Cast<AGridManager>(
                UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())))
            {
                AimingTargetGrid = GridMgr->WorldToGrid(MouseGroundPos);
            }
        }
    }
}


void USkillComponent::TryStartAiming(int32 SkillIndex)
{
    if (CurrentState == ESkillState::Idle)
    {
        CurrentState = ESkillState::Aiming;
        AimingSkillIndex = SkillIndex;
        UE_LOG(LogTemp, Log, TEXT("SkillComponent: Entering Aiming mode for skill %d"), SkillIndex);
    }
    else if (CurrentState == ESkillState::Aiming && AimingSkillIndex == SkillIndex)
    {
        CancelAiming();
    }
    else if (CurrentState == ESkillState::Aiming && AimingSkillIndex != SkillIndex)
    {
        AimingSkillIndex = SkillIndex;
        UE_LOG(LogTemp, Log, TEXT("SkillComponent: Switched to Aiming mode for skill %d"), SkillIndex);
    }
}
void USkillComponent::CancelAiming()
{
    if (CurrentState == ESkillState::Aiming)
    {
        CurrentState = ESkillState::Idle;
        AimingSkillIndex = -1;
        if (OwnerCharacter)
        {
            OwnerCharacter->HideRangeIndicators();
        }
        UE_LOG(LogTemp, Log, TEXT("SkillComponent: Canceled Aiming mode"));
    }
}


void USkillComponent::TryConfirmSkill()
{
    if (CurrentState != ESkillState::Aiming || AimingSkillIndex == -1) return;

    if (TryActivateSkill(AimingSkillIndex))
    {
        CurrentState = ESkillState::Casting;
        if (OwnerCharacter)
        {
            OwnerCharacter->HideRangeIndicators();
        }

        const USkillDataAsset* SkillData = GetSkillData(AimingSkillIndex);
        if (SkillData)
        {
            if (SkillData->TimeCost > 0.0f)
            {
                GetWorld()->GetTimerManager().SetTimer(CastingTimerHandle, this, &USkillComponent::FinishCasting, SkillData->TimeCost, false);
            }
            else
            {
                FinishCasting();
            }
        }
    }
    else
    {
        // 激活失败，返回Idle
        CurrentState = ESkillState::Idle;
        AimingSkillIndex = -1;
        if (OwnerCharacter)
        {
            OwnerCharacter->HideRangeIndicators();
        }
    }
}

void USkillComponent::FinishCasting()
{
    CurrentState = ESkillState::Idle;
    AimingSkillIndex = -1;
    GetWorld()->GetTimerManager().ClearTimer(CastingTimerHandle);
    UE_LOG(LogTemp, Log, TEXT("SkillComponent: Finished Casting, returning to Idle."));
}

bool USkillComponent::TryActivateSkill(int32 SkillIndex)
{
    if (!SkillSlots.IsValidIndex(SkillIndex)) return false;

    FSkillEntry& SkillEntry = SkillSlots[SkillIndex];

    // 检查冷却和能否激活
    if (SkillEntry.SkillInstance && SkillEntry.SkillInstance->CanActivate())
    {
        SkillEntry.SkillInstance->Activate();
        // 设置冷却时间
        SkillEntry.CooldownRemaining = SkillEntry.SkillData->Cooldown;
        return true;    // 技能激活成功
    }
    return false;
}

float USkillComponent::GetCooldownRemaining(int32 SkillIndex) const     // 将技能cd剩余时间传给BaseSkill的CanActivate函数来判断技能是否可用
{
    if (SkillSlots.IsValidIndex(SkillIndex))
    {
        return SkillSlots[SkillIndex].CooldownRemaining;
    }
    return 0.0f;
}

const USkillDataAsset* USkillComponent::GetSkillData(int32 SkillIndex) const
{
    // 检查索引是否在有效范围内
    if (SkillSlots.IsValidIndex(SkillIndex))
    {
        // 返回对应技能插槽中的数据资产指针
        return SkillSlots[SkillIndex].SkillData;
    }

    // 如果索引无效，返回空指针
    return nullptr;
}

int32 USkillComponent::GetSkillIndex(const UBaseSkill* SkillInstance) const
{
    for (int32 i = 0; i < SkillSlots.Num(); ++i)
    {
        if (SkillSlots[i].SkillInstance == SkillInstance)
        {
            return i;
        }
    }
    return INDEX_NONE;  //找不到则返回-1
}