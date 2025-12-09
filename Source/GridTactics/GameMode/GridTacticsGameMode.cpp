// Fill out your copyright notice in the Description page of Project Settings.


#include "GridTacticsGameMode.h"
#include "GridTactics/GridTacticsPlayerController.h"
#include "GridTactics/HeroCharacter.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/SkillComponent.h"
#include "GridTactics/SkillDataAsset.h"
#include "GridTactics/AttributesComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

AGridTacticsGameMode::AGridTacticsGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;  // 禁用自动生成
    // 设置默认玩家控制器
    PlayerControllerClass = AGridTacticsPlayerController::StaticClass();

    // 设置默认玩家角色类为 C++（蓝图中可以覆盖）
    PlayerCharacterClass = AHeroCharacter::StaticClass();
}

void AGridTacticsGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 初始化游戏阶段
    SetGamePhase(EGamePhase::Preparation);

    // 显示准备界面
    ShowPreparationUI();

    UE_LOG(LogTemp, Log, TEXT("GameMode: Entered Preparation Phase"));
}

void AGridTacticsGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 监听玩家死亡（可选：在 AttributesComponent 中直接调用）
}

// ========================================
// 游戏流程控制
// ========================================

void AGridTacticsGameMode::SetGamePhase(EGamePhase NewPhase)
{
    if (CurrentPhase == NewPhase)
    {
        return;
    }

    CurrentPhase = NewPhase;
    OnGamePhaseChanged.Broadcast(NewPhase);

    UE_LOG(LogTemp, Log, TEXT("GameMode: Phase changed to %d"), static_cast<int32>(NewPhase));
}

void AGridTacticsGameMode::OnPlayerReady()
{
    if (CurrentPhase != EGamePhase::Preparation)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: OnPlayerReady called in wrong phase"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("GameMode: Player is ready, starting combat"));

    // 生成玩家
    SpawnPlayer();

    // 切换到战斗阶段
    SetGamePhase(EGamePhase::Combat);

    // 开始第一波
    CurrentWaveIndex = 0;
    StartNextWave();
}

void AGridTacticsGameMode::SpawnPlayer()
{
    // 获取 GridManager
    AGridManager* GridMgr = Cast<AGridManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
    );

    if (!GridMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: GridManager not found"));
        return;
    }

    // 计算生成位置
    FVector SpawnLocation = GridMgr->GridToWorld(PlayerSpawnGrid);
    SpawnLocation.Z = 50.0f;  // 略微抬高避免穿模

    FRotator SpawnRotation = FRotator::ZeroRotator;

    // 生成玩家角色
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 修改：使用配置的蓝图类
    if (!PlayerCharacterClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: PlayerCharacterClass is NULL!"));
        return;
    }

    PlayerCharacter = GetWorld()->SpawnActor<AHeroCharacter>(
        PlayerCharacterClass,  // 使用蓝图类
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (PlayerCharacter)
    {
        // 添加调试日志
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Spawned %s from class %s"), 
            *PlayerCharacter->GetName(),
            *PlayerCharacterClass->GetName());

        // 在生成后立即添加技能
        if (USkillComponent* SkillComp = PlayerCharacter->FindComponentByClass<USkillComponent>())
        {
            for (USkillDataAsset* Skill : PendingSkills)
            {
                if (Skill)
                {
                    SkillComp->AddSkill(Skill);
                    UE_LOG(LogTemp, Log, TEXT("GameMode: Added pending skill: %s"), 
                        *Skill->SkillName.ToString());
                }
            }

            // 清空临时技能列表
            PendingSkills.Empty();
        }

        // 延迟 Possess
        FTimerHandle PossessTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            PossessTimerHandle,
            [this]()
            {
                if (PlayerCharacter && PlayerCharacter->IsValidLowLevel())
                {
                    APlayerController* PC = GetWorld()->GetFirstPlayerController();
                    if (PC)
                    {
                        PC->Possess(PlayerCharacter);
                        UE_LOG(LogTemp, Warning, TEXT("GameMode: Possessed %s"), *PlayerCharacter->GetName());

                        // 设置视图目标
                        FTimerHandle ViewTimerHandle;
                        GetWorld()->GetTimerManager().SetTimer(
                            ViewTimerHandle,
                            [this, PC]()
                            {
                                if (PlayerCharacter)
                                {
                                    PC->SetViewTarget(PlayerCharacter);
                                    UE_LOG(LogTemp, Warning, TEXT("GameMode: SetViewTarget to %s"), 
                                        *PlayerCharacter->GetName());
                                }
                            },
                            0.2f,
                            false
                        );

                        // 恢复输入模式
                        FInputModeGameAndUI InputMode;
                        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                        InputMode.SetHideCursorDuringCapture(false);
                        PC->SetInputMode(InputMode);
                        PC->bShowMouseCursor = true;
                    }
                }
            },
            0.5f,
            false
        );

        // 绑定死亡事件
        if (UAttributesComponent* Attrs = PlayerCharacter->FindComponentByClass<UAttributesComponent>())
        {
            Attrs->OnCharacterDied.AddDynamic(this, &AGridTacticsGameMode::OnPlayerDied);
        }

        // 调试：检查 IMC_Hero
        if (PlayerCharacter->IMC_Hero)
        {
            UE_LOG(LogTemp, Warning, TEXT("GameMode: IMC_Hero = %s"), *PlayerCharacter->IMC_Hero->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("GameMode: IMC_Hero is NULL! Did you set it in BP_HeroCharacter?"));
        }

        UE_LOG(LogTemp, Log, TEXT("GameMode: Player spawned at grid (%d, %d)"),
            PlayerSpawnGrid.X, PlayerSpawnGrid.Y);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to spawn player"));
    }
}

void AGridTacticsGameMode::StartNextWave()
{
    if (!WaveConfigs.IsValidIndex(CurrentWaveIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: Invalid wave index %d"), CurrentWaveIndex);
        return;
    }

    const FWaveConfig& WaveConfig = WaveConfigs[CurrentWaveIndex];

    UE_LOG(LogTemp, Log, TEXT("GameMode: Starting wave %d/%d - %s"),
        CurrentWaveIndex + 1,
        WaveConfigs.Num(),
        *WaveConfig.WaveName.ToString());

    // 广播波次变更
    OnWaveChanged.Broadcast(CurrentWaveIndex + 1, WaveConfigs.Num());

    // 延迟生成敌人
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        [this, WaveConfig]()
        {
            SpawnEnemies(WaveConfig);
        },
        WaveConfig.StartDelay,
        false
    );
}

void AGridTacticsGameMode::SpawnEnemies(const FWaveConfig& WaveConfig)
{
    AGridManager* GridMgr = Cast<AGridManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
    );

    if (!GridMgr)
    {
        return;
    }

    CurrentWaveEnemies.Empty();

    for (const FEnemySpawnConfig& EnemyConfig : WaveConfig.Enemies)
    {
        if (!EnemyConfig.EnemyClass)
        {
            continue;
        }

        // 计算生成位置
        FVector SpawnLocation = GridMgr->GridToWorld(EnemyConfig.SpawnGrid);
        SpawnLocation.Z = 100.0f;

        FRotator SpawnRotation = FRotator::ZeroRotator;

        // 生成敌人
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ACharacter* Enemy = GetWorld()->SpawnActor<ACharacter>(
            EnemyConfig.EnemyClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (Enemy)
        {
            // 绑定敌人死亡事件
            if (UAttributesComponent* Attrs = Enemy->FindComponentByClass<UAttributesComponent>())
            {
                Attrs->OnCharacterDied.AddDynamic(this, &AGridTacticsGameMode::OnEnemyDied);
            }

            CurrentWaveEnemies.Add(Enemy);

            UE_LOG(LogTemp, Log, TEXT("GameMode: Spawned enemy at grid (%d, %d)"),
                EnemyConfig.SpawnGrid.X, EnemyConfig.SpawnGrid.Y);
        }
    }

    RemainingEnemies = CurrentWaveEnemies.Num();

    UE_LOG(LogTemp, Log, TEXT("GameMode: Wave %d started with %d enemies"),
        CurrentWaveIndex + 1, RemainingEnemies);

}

// 修改：玩家死亡回调
void AGridTacticsGameMode::OnPlayerDied(AActor* Player)
{
    SetGamePhase(EGamePhase::Defeat);

    UE_LOG(LogTemp, Error, TEXT("GameMode: DEFEAT! Player %s died"), 
        Player ? *Player->GetName() : TEXT("Unknown"));

    // TODO: 显示失败界面
}

// 新增：敌人死亡回调
void AGridTacticsGameMode::OnEnemyDied(AActor* Enemy)
{
    ACharacter* EnemyChar = Cast<ACharacter>(Enemy);
    if (!EnemyChar)
    {
        return;
    }

    NotifyEnemyDefeated(EnemyChar);
}

// 保持不变
void AGridTacticsGameMode::NotifyEnemyDefeated(ACharacter* Enemy)
{
    if (CurrentPhase != EGamePhase::Combat)
    {
        return;
    }

    // 从列表中移除
    CurrentWaveEnemies.Remove(Enemy);
    RemainingEnemies = CurrentWaveEnemies.Num();

    UE_LOG(LogTemp, Log, TEXT("GameMode: Enemy defeated, %d remaining"), RemainingEnemies);

    // 广播事件
    OnEnemyDefeated.Broadcast(Enemy);

    // 显示技能选择
    if (!bWaitingForSkillSelection)
    {
        bWaitingForSkillSelection = true;
        ShowSkillSelection();
    }

    // 检查波次是否完成
    if (RemainingEnemies == 0)
    {
        OnWaveComplete();
    }
}

void AGridTacticsGameMode::OnWaveComplete()
{
    UE_LOG(LogTemp, Log, TEXT("GameMode: Wave %d complete"), CurrentWaveIndex + 1);

    // 检查是否还有下一波
    if (CurrentWaveIndex + 1 < WaveConfigs.Num())
    {
        // 开始下一波
        CurrentWaveIndex++;
        StartNextWave();
    }
    else
    {
        // 所有波次完成，胜利
        CheckVictoryCondition();
    }
}

void AGridTacticsGameMode::ShowSkillSelection()
{
    // 暂停游戏
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    PC->SetPause(true);
    PC->bShowMouseCursor = true;

    // 随机选择3个技能
    CurrentSkillOptions.Empty();
    TArray<int32> UsedIndices;

    for (int32 i = 0; i < 3 && SkillPool.Num() > 0; ++i)
    {
        int32 RandomIndex;
        do
        {
            RandomIndex = FMath::RandRange(0, SkillPool.Num() - 1);
        } while (UsedIndices.Contains(RandomIndex) && UsedIndices.Num() < SkillPool.Num());

        UsedIndices.Add(RandomIndex);
        CurrentSkillOptions.Add(SkillPool[RandomIndex]);
    }

    // 加载技能选择界面
    UClass* WidgetClass = LoadClass<UUserWidget>(
        nullptr,
        TEXT("/Game/UI/WBP_SkillSelection.WBP_SkillSelection_C")
    );

    if (WidgetClass)
    {
        UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (Widget)
        {
            Widget->AddToViewport(999);  // 高优先级
            UE_LOG(LogTemp, Log, TEXT("GameMode: Skill selection UI displayed"));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GameMode: Showing skill selection (%d options)"), CurrentSkillOptions.Num());
}

void AGridTacticsGameMode::OnPlayerSelectSkill(USkillDataAsset* SelectedSkill)
{
    if (!SelectedSkill || !PlayerCharacter)
    {
        return;
    }

    // 添加技能到玩家
    if (USkillComponent* SkillComp = PlayerCharacter->FindComponentByClass<USkillComponent>())
    {
        if (SkillComp->AddSkill(SelectedSkill))
        {
            UE_LOG(LogTemp, Log, TEXT("GameMode: Player selected skill: %s"), 
                *SelectedSkill->SkillName.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GameMode: Failed to add skill (slots full?)"));
            // TODO: 显示替换界面
        }
    }

    // 完成技能选择
    OnSkillSelectionComplete();
}

void AGridTacticsGameMode::OnSkillSelectionComplete()
{
    bWaitingForSkillSelection = false;

    // 恢复游戏
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetPause(false);
        PC->bShowMouseCursor = true;  // 保持鼠标可见

        // 恢复输入模式
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
    }

    UE_LOG(LogTemp, Log, TEXT("GameMode: Skill selection complete"));
}

void AGridTacticsGameMode::CheckVictoryCondition()
{
    SetGamePhase(EGamePhase::Victory);

    UE_LOG(LogTemp, Log, TEXT("GameMode: VICTORY!"));

    // TODO: 显示胜利界面
}

void AGridTacticsGameMode::ShowPreparationUI()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: PlayerController not found"));
        return;
    }

    // 显示鼠标
    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;

    // 使用 LoadClass 动态加载蓝图类
    UClass* WidgetClass = LoadClass<UUserWidget>(
        nullptr,
        TEXT("/Game/UI/WBP_Preparation.WBP_Preparation_C")
    );

    if (WidgetClass)
    {
        UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (Widget)
        {
            Widget->AddToViewport();
            UE_LOG(LogTemp, Log, TEXT("GameMode: Preparation UI displayed"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to create Preparation Widget"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: WBP_Preparation blueprint not found at /Game/UI/WBP_Preparation"));
    }
}

APawn* AGridTacticsGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
    // 在准备阶段完全阻止自动生成
    if (CurrentPhase == EGamePhase::Preparation)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: SpawnDefaultPawnAtTransform blocked - in Preparation phase"));
        return nullptr;
    }

    // 其他阶段返回手动生成的角色
    if (PlayerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: SpawnDefaultPawnAtTransform - returning existing PlayerCharacter"));
        return PlayerCharacter;
    }

    return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

APawn* AGridTacticsGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    if (CurrentPhase == EGamePhase::Preparation)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: SpawnDefaultPawnFor blocked - in Preparation phase"));
        return nullptr;
    }

    if (PlayerCharacter)
    {
        return PlayerCharacter;
    }

    return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

void AGridTacticsGameMode::RestartPlayer(AController* NewPlayer)
{
    if (CurrentPhase == EGamePhase::Preparation)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: RestartPlayer blocked - in Preparation phase"));
        return;
    }

    Super::RestartPlayer(NewPlayer);
}
