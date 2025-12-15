// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_SelectAndUseSkill.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GridTactics/EnemyCharacter.h"
#include "GridTactics/Skills/SkillComponent.h"
#include "GridTactics/Skills/SkillDataAsset.h"
#include "GridTactics/GridMovement/GridManager.h"
#include "GridTactics/AttributesComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_SelectAndUseSkill::UBTTask_SelectAndUseSkill()
{
	NodeName = "Select And Use Skill";
	bNotifyTick = true;
	
	// 设置默认黑板键
	TargetActorKey.SelectedKeyName = FName("TargetPlayer");
	LastSkillTimeKey.SelectedKeyName = FName("LastSkillTime");
}

EBTNodeResult::Type UBTTask_SelectAndUseSkill::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogTemp, Warning, TEXT("========== BTTask_SelectAndUseSkill: ExecuteTask START =========="));

	// 获取任务内存
	FBTSelectAndUseSkillMemory* MyMemory = CastInstanceNodeMemory<FBTSelectAndUseSkillMemory>(NodeMemory);
	MyMemory->bSkillConfirmed = false;
	MyMemory->SkillExecutionTimer = 0.0f;
	MyMemory->ExecutingSkillIndex = -1;

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill: No AIController!"));
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!EnemyChar)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill: No EnemyCharacter!"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_SelectAndUseSkill: Enemy = %s"), *EnemyChar->GetName());

	USkillComponent* SkillComp = EnemyChar->GetSkillComponent();
	if (!SkillComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill: No SkillComponent!"));
		return EBTNodeResult::Failed;
	}

	// 检查技能数量
	int32 SkillCount = SkillComp->GetSkillCount();
	UE_LOG(LogTemp, Log, TEXT("BTTask_SelectAndUseSkill: Enemy has %d skills"), SkillCount);

	if (SkillCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill: Enemy has NO skills configured!"));
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill: No Blackboard!"));
		return EBTNodeResult::Failed;
	}

	// 检查技能使用间隔
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float LastSkillTime = BlackboardComp->GetValueAsFloat(LastSkillTimeKey.SelectedKeyName);
	float TimeSinceLastSkill = CurrentTime - LastSkillTime;

	if (TimeSinceLastSkill < SkillUsageCooldown)
	{
		UE_LOG(LogTemp, Log, TEXT("BTTask_SelectAndUseSkill: Skill on cooldown (%.1fs / %.1fs), must move first!"), 
			TimeSinceLastSkill, SkillUsageCooldown);
		return EBTNodeResult::Failed;
	}

	// 获取目标玩家
	AActor* TargetPlayer = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill: No TargetPlayer in Blackboard!"));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogTemp, Log, TEXT("BTTask_SelectAndUseSkill: Target = %s"), *TargetPlayer->GetName());

	// 按优先级检查技能
	bool bFoundUsableSkill = false;
	for (int32 SkillIndex : SkillPriority)
	{
		// 检查索引有效性
		if (SkillIndex >= SkillCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill: SkillIndex %d out of range (total: %d), skipping"), 
				SkillIndex, SkillCount);
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("BTTask_SelectAndUseSkill: Checking skill %d..."), SkillIndex);

		if (IsSkillUsable(SkillComp, SkillIndex, TargetPlayer, EnemyChar))
		{
			bFoundUsableSkill = true;
			UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill: Using skill %d!"), SkillIndex);

			// 开始瞄准
			SkillComp->TryStartAiming(SkillIndex);

			// 存储到任务内存
			MyMemory->ExecutingSkillIndex = SkillIndex;
			MyMemory->SkillExecutionTimer = 0.0f;

			UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill: Skill execution started, returning InProgress"));
			return EBTNodeResult::InProgress;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("BTTask_SelectAndUseSkill: Skill %d not usable"), SkillIndex);
		}
	}

	// 没有可用技能
	UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill: NO usable skills found! Returning Failed."));
	UE_LOG(LogTemp, Warning, TEXT("========== BTTask_SelectAndUseSkill: ExecuteTask END (Failed) =========="));
	return EBTNodeResult::Failed;
}

void UBTTask_SelectAndUseSkill::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 获取任务内存
	FBTSelectAndUseSkillMemory* MyMemory = CastInstanceNodeMemory<FBTSelectAndUseSkillMemory>(NodeMemory);
	
	MyMemory->SkillExecutionTimer += DeltaSeconds;

	// 延迟 0.3 秒后确认技能（只执行一次）
	if (!MyMemory->bSkillConfirmed && MyMemory->SkillExecutionTimer >= 0.3f)
	{
		AAIController* AIController = OwnerComp.GetAIOwner();
		if (!AIController)
		{
			UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill (Tick): No AIController!"));
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(AIController->GetPawn());
		if (!EnemyChar)
		{
			UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill (Tick): No EnemyCharacter!"));
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		USkillComponent* SkillComp = EnemyChar->GetSkillComponent();
		if (SkillComp)
		{
			// 确认技能（只执行一次）
			SkillComp->TryConfirmSkill();
			MyMemory->bSkillConfirmed = true;
			
			UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill (Tick): Skill %d confirmed ONCE at %.2fs"), 
				MyMemory->ExecutingSkillIndex, MyMemory->SkillExecutionTimer);
		}
	}

	// 等待技能的 TimeCost 完成
	if (MyMemory->bSkillConfirmed)
	{
		AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(OwnerComp.GetAIOwner()->GetPawn());
		if (!EnemyChar)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		USkillComponent* SkillComp = EnemyChar->GetSkillComponent();
		const USkillDataAsset* SkillData = SkillComp ? SkillComp->GetSkillData(MyMemory->ExecutingSkillIndex) : nullptr;
		
		if (SkillData)
		{
			float TotalWaitTime = 0.3f + SkillData->TimeCost + 0.5f;
			if (MyMemory->SkillExecutionTimer >= TotalWaitTime)
			{
				// 技能执行完成，记录时间到黑板
				UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
				if (BlackboardComp)
				{
					float CurrentTime = GetWorld()->GetTimeSeconds();
					BlackboardComp->SetValueAsFloat(LastSkillTimeKey.SelectedKeyName, CurrentTime);
					
					UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill (Tick): Skill execution completed! Recorded time: %.2f"), 
						CurrentTime);
				}
				
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BTTask_SelectAndUseSkill (Tick): No SkillData, finishing immediately"));
			
			// 即使没有 SkillData 也记录时间
			UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
			if (BlackboardComp)
			{
				BlackboardComp->SetValueAsFloat(LastSkillTimeKey.SelectedKeyName, GetWorld()->GetTimeSeconds());
			}
			
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

bool UBTTask_SelectAndUseSkill::IsSkillUsable(USkillComponent* SkillComp, int32 SkillIndex, AActor* Target, AActor* Self) const
{
	if (!SkillComp || !Target || !Self)
	{
		UE_LOG(LogTemp, Error, TEXT("BTTask_SelectAndUseSkill::IsSkillUsable: Invalid parameters!"));
		return false;
	}

	// 检查技能是否存在
	const USkillDataAsset* SkillData = SkillComp->GetSkillData(SkillIndex);
	if (!SkillData)
	{
		UE_LOG(LogTemp, Warning, TEXT("  IsSkillUsable[%d]: No SkillData!"), SkillIndex);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: Checking skill '%s'"), 
		SkillIndex, *SkillData->SkillName.ToString());

	// 检查冷却
	float CooldownRemaining = SkillComp->GetCooldownRemaining(SkillIndex);
	if (CooldownRemaining > 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: On cooldown (%.1fs remaining)"), 
			SkillIndex, CooldownRemaining);
		return false;
	}

	// 检查资源
	UAttributesComponent* AttributesComp = Self->FindComponentByClass<UAttributesComponent>();
	if (!AttributesComp)
	{
		UE_LOG(LogTemp, Error, TEXT("  IsSkillUsable[%d]: No AttributesComponent!"), SkillIndex);
		return false;
	}

	float CurrentMP = AttributesComp->GetMP();
	float CurrentStamina = AttributesComp->GetStamina();

	if (CurrentMP < SkillData->MPCost)
	{
		UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: Not enough MP (need %.1f, have %.1f)"), 
			SkillIndex, SkillData->MPCost, CurrentMP);
		return false;
	}

	if (CurrentStamina < SkillData->StaminaCost)
	{
		UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: Not enough Stamina (need %.1f, have %.1f)"), 
			SkillIndex, SkillData->StaminaCost, CurrentStamina);
		return false;
	}

	// 检查目标是否在范围内
	AGridManager* GridMgr = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(Self->GetWorld(), AGridManager::StaticClass())
	);
	if (!GridMgr)
	{
		UE_LOG(LogTemp, Error, TEXT("  IsSkillUsable[%d]: No GridManager!"), SkillIndex);
		return false;
	}

	FIntPoint SelfGrid = GridMgr->GetActorCurrentGrid(Self);
	FIntPoint TargetGrid = GridMgr->GetActorCurrentGrid(Target);

	UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: SelfGrid=%s, TargetGrid=%s"), 
		SkillIndex, *SelfGrid.ToString(), *TargetGrid.ToString());

	// 计算相对位置
	FIntPoint RelativePos = TargetGrid - SelfGrid;

	// 获取角色朝向，将相对位置旋转到局部坐标系
	FRotator SelfRotation = Self->GetActorRotation();
	FVector RelativeVec(RelativePos.X, RelativePos.Y, 0);
	FVector LocalVec = SelfRotation.UnrotateVector(RelativeVec);
	FIntPoint LocalPos(FMath::RoundToInt(LocalVec.X), FMath::RoundToInt(LocalVec.Y));

	UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: RelativePos=%s, Rotation=%.1f, LocalPos=%s"), 
		SkillIndex, *RelativePos.ToString(), SelfRotation.Yaw, *LocalPos.ToString());

	// 检查 RangePattern
	if (SkillData->RangePattern.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("  IsSkillUsable[%d]: RangePattern is EMPTY! Skill cannot be used."), 
			SkillIndex);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: Checking against %d range points..."), 
		SkillIndex, SkillData->RangePattern.Num());

	// 检查是否在 RangePattern 中
	for (const FIntPoint& RangePoint : SkillData->RangePattern)
	{
		if (LocalPos == RangePoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("  IsSkillUsable[%d]: Target IS in range! (matched %s)"), 
				SkillIndex, *RangePoint.ToString());
			return true;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("  IsSkillUsable[%d]: Target NOT in range"), SkillIndex);
	return false;
}

FIntPoint UBTTask_SelectAndUseSkill::CalculateDirectionToTarget(AActor* Self, AActor* Target) const
{
	FVector DirectionVec = (Target->GetActorLocation() - Self->GetActorLocation()).GetSafeNormal();
	
	// 转换为四向
	if (FMath::Abs(DirectionVec.X) > FMath::Abs(DirectionVec.Y))
	{
		return FIntPoint((DirectionVec.X > 0) ? 1 : -1, 0);
	}
	else
	{
		return FIntPoint(0, (DirectionVec.Y > 0) ? 1 : -1);
	}
}

