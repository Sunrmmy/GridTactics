// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillComponent.h"
#include "SkillDataAsset.h"
#include "SkillEffect.h"
#include "GridTactics/GridMovement/GridManager.h"
#include "BaseSkill.h"
#include "GridTactics/HeroCharacter.h"
#include "GridTactics/EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"

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

	// 改为使用基类 ACharacter
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("SkillComponent: Owner is not a Character! Component on: %s"), 
			*GetOwner()->GetName());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("SkillComponent: BeginPlay on %s (Owner: %s)"), 
		*GetName(), *OwnerCharacter->GetName());

	// 检查 EquippedSkillsData
	if (EquippedSkillsData.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SkillComponent: EquippedSkillsData is EMPTY! No skills to initialize."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("SkillComponent: Found %d skills in EquippedSkillsData"), 
			EquippedSkillsData.Num());
	}

	// 基于数据资产，实例化技能逻辑对象
	for (int32 i = 0; i < EquippedSkillsData.Num(); ++i)
	{
		const USkillDataAsset* DataAsset = EquippedSkillsData[i];
		
		if (!DataAsset)
		{
			UE_LOG(LogTemp, Warning, TEXT("SkillComponent: EquippedSkillsData[%d] is NULL!"), i);
			continue;
		}

		if (!DataAsset->SkillLogicClass)
		{
			UE_LOG(LogTemp, Error, TEXT("SkillComponent: Skill '%s' has no SkillLogicClass!"), 
				*DataAsset->SkillName.ToString());
			continue;
		}

		FSkillEntry NewEntry;
		NewEntry.SkillData = DataAsset;
		NewEntry.SkillInstance = NewObject<UBaseSkill>(this, DataAsset->SkillLogicClass);
		NewEntry.SkillInstance->Initialize(OwnerCharacter, DataAsset);
		SkillSlots.Add(NewEntry);

		UE_LOG(LogTemp, Log, TEXT("SkillComponent: Initialized skill[%d]: '%s' (Class: %s)"), 
			i, 
			*DataAsset->SkillName.ToString(),
			*DataAsset->SkillLogicClass->GetName());
	}

	UE_LOG(LogTemp, Warning, TEXT("SkillComponent: BeginPlay complete. Total skills in SkillSlots: %d"), 
		SkillSlots.Num());
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
		// 检查是否是玩家控制的角色（HeroCharacter）
		if (AHeroCharacter* HeroChar = Cast<AHeroCharacter>(OwnerCharacter))
		{
			// 从鼠标位置获取目标格子
			APlayerController* PC = Cast<APlayerController>(HeroChar->GetController());
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
		// ✅ 对于 AI 控制的角色（EnemyCharacter），目标格子应该在技能执行时由 AI 设置
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
        
        // 只有 HeroCharacter 才有范围指示器
        if (AHeroCharacter* HeroChar = Cast<AHeroCharacter>(OwnerCharacter))
        {
            HeroChar->HideRangeIndicators();
        }
        
        UE_LOG(LogTemp, Log, TEXT("SkillComponent: Canceled Aiming mode"));
    }
}


void USkillComponent::TryConfirmSkill()
{
    if (CurrentState != ESkillState::Aiming || AimingSkillIndex == -1)
    {
        return;
    }

    const USkillDataAsset* SkillData = GetSkillData(AimingSkillIndex);
    if (!SkillData)
    {
        return;
    }

    // 验证技能是否可以激活（检查资源/冷却）
    if (!SkillSlots.IsValidIndex(AimingSkillIndex))
    {
        return;
    }

    FSkillEntry& SkillEntry = SkillSlots[AimingSkillIndex];
    if (!SkillEntry.SkillInstance || !SkillEntry.SkillInstance->CanActivate())
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillComponent: Skill cannot be activated"));
        CancelAiming();
        return;
    }

    // 进入 Casting 状态
    CurrentState = ESkillState::Casting;
    CurrentCastingSkillIndex = AimingSkillIndex;
    AimingSkillIndex = -1;

    // 只有 HeroCharacter 才有范围指示器
    if (AHeroCharacter* HeroChar = Cast<AHeroCharacter>(OwnerCharacter))
    {
        HeroChar->HideRangeIndicators();
    }

    // 立即执行技能（每个 Effect 自己控制延迟）
    SkillEntry.SkillInstance->Activate();

    // 设置冷却
    SkillEntry.CooldownRemaining = SkillData->Cooldown;

    // 计算最大 Effect 延迟，作为 Casting 状态持续时间
    float MaxEffectDelay = 0.0f;
    for (USkillEffect* Effect : SkillData->SkillEffects)
    {
        if (Effect)
        {
            MaxEffectDelay = FMath::Max(MaxEffectDelay, Effect->ExecutionDelay);
        }
    }

    // 加上 TimeCost
    float TotalCastingTime = MaxEffectDelay + SkillData->TimeCost;

    if (TotalCastingTime > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            TimeCostTimerHandle,
            this,
            &USkillComponent::FinishCasting,
            TotalCastingTime,
            false
        );

        UE_LOG(LogTemp, Log, TEXT("SkillComponent: Casting state will last %.2fs (MaxDelay: %.2f + TimeCost: %.2f)"),
            TotalCastingTime, MaxEffectDelay, SkillData->TimeCost);
    }
    else
    {
        FinishCasting();
    }
}

void USkillComponent::OnCastDelayFinished()
{
    if (!SkillSlots.IsValidIndex(CurrentCastingSkillIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SkillComponent: Invalid casting skill index"));
        FinishCasting();
        return;
    }

    FSkillEntry& SkillEntry = SkillSlots[CurrentCastingSkillIndex];
    const USkillDataAsset* SkillData = SkillEntry.SkillData;

    if (!SkillData)
    {
        FinishCasting();
        return;
    }

    // 执行技能（调用 Activate）
    if (SkillEntry.SkillInstance)
    {
        SkillEntry.SkillInstance->Activate();

        // 设置冷却
        SkillEntry.CooldownRemaining = SkillData->Cooldown;

        UE_LOG(LogTemp, Log, TEXT("SkillComponent: Skill %d executed after cast delay"),
            CurrentCastingSkillIndex);
    }

    // 如果有 TimeCost，延迟返回 Idle
    if (SkillData->TimeCost > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            TimeCostTimerHandle,
            this,
            &USkillComponent::FinishCasting,
            SkillData->TimeCost,
            false
        );

        UE_LOG(LogTemp, Log, TEXT("SkillComponent: Waiting %.2fs before finishing cast"),
            SkillData->TimeCost);
    }
    else
    {
        // 无 TimeCost，立即返回 Idle
        FinishCasting();
    }
}


void USkillComponent::FinishCasting()
{
    CurrentState = ESkillState::Idle;
    CurrentCastingSkillIndex = -1;

    // 清理所有计时器
    GetWorld()->GetTimerManager().ClearTimer(CastDelayTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(TimeCostTimerHandle);

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

bool USkillComponent::AddSkill(USkillDataAsset* NewSkillData)
{
    if (!NewSkillData || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillComponent::AddSkill - Invalid skill data or owner"));
        return false;
    }

    // 检查是否已满（最多5个技能）
    if (SkillSlots.Num() >= 5)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillComponent::AddSkill - Skill slots are full!"));
        return false;
    }

    // 创建技能实例
    FSkillEntry NewEntry;
    NewEntry.SkillData = NewSkillData;
    
    if (NewSkillData->SkillLogicClass)
    {
        NewEntry.SkillInstance = NewObject<UBaseSkill>(this, NewSkillData->SkillLogicClass);
        if (NewEntry.SkillInstance)
        {
            NewEntry.SkillInstance->Initialize(OwnerCharacter, NewSkillData);
        }
    }
    
    NewEntry.CooldownRemaining = 0.0f;
    
    int32 NewSlotIndex = SkillSlots.Num();
    SkillSlots.Add(NewEntry);

    UE_LOG(LogTemp, Log, TEXT("SkillComponent::AddSkill - Added skill: %s (Slot %d)"), 
        *NewSkillData->SkillName.ToString(), 
        NewSlotIndex);
    
    // 广播技能添加事件
    OnSkillAdded.Broadcast(NewSlotIndex);
    
    return true;
}

bool USkillComponent::ReplaceSkill(int32 SlotIndex, USkillDataAsset* NewSkillData)
{
    if (!NewSkillData || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillComponent::ReplaceSkill - Invalid skill data or owner"));
        return false;
    }

    if (!SkillSlots.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillComponent::ReplaceSkill - Invalid slot index: %d"), SlotIndex);
        return false;
    }

    // 检查是否替换成相同的技能
    if (SkillSlots[SlotIndex].SkillData == NewSkillData)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillComponent::ReplaceSkill - Same skill, no replacement needed"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("SkillComponent::ReplaceSkill - Replacing skill at slot %d: %s -> %s"),
        SlotIndex,
        SkillSlots[SlotIndex].SkillData ? *SkillSlots[SlotIndex].SkillData->SkillName.ToString() : TEXT("None"),
        *NewSkillData->SkillName.ToString());

    // 创建新技能实例
    FSkillEntry NewEntry;
    NewEntry.SkillData = NewSkillData;
    
    if (NewSkillData->SkillLogicClass)
    {
        NewEntry.SkillInstance = NewObject<UBaseSkill>(this, NewSkillData->SkillLogicClass);
        if (NewEntry.SkillInstance)
        {
            NewEntry.SkillInstance->Initialize(OwnerCharacter, NewSkillData);
        }
    }
    
    NewEntry.CooldownRemaining = 0.0f;
    
    // 替换技能槽
    SkillSlots[SlotIndex] = NewEntry;

    UE_LOG(LogTemp, Log, TEXT("SkillComponent::ReplaceSkill - Skill replaced successfully"));

    // 广播技能替换事件
    OnSkillReplaced.Broadcast(SlotIndex, NewSkillData);
    
    return true;
}