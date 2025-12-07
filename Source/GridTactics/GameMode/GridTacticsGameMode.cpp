// Fill out your copyright notice in the Description page of Project Settings.


#include "GridTacticsGameMode.h"
#include "GridTactics/HeroCharacter.h"
#include "GridTactics/GridManager.h"
#include "GridTactics/SkillComponent.h"
#include "GridTactics/SkillDataAsset.h"
#include "GridTactics/AttributesComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

AGridTacticsGameMode::AGridTacticsGameMode()
{
    PrimaryActorTick.bCanEverTick = true;

    // 设置默认玩家控制器
    // PlayerControllerClass = AGridTacticsPlayerController::StaticClass();
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
    SpawnLocation.Z = 100.0f;  // 略微抬高避免穿模

    FRotator SpawnRotation = FRotator::ZeroRotator;

    // 生成玩家角色
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    PlayerCharacter = GetWorld()->SpawnActor<AHeroCharacter>(
        AHeroCharacter::StaticClass(),
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (PlayerCharacter)
    {
        // 设置玩家控制器
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            PC->Possess(PlayerCharacter);
        }

        // 绑定死亡事件
        if (UAttributesComponent* Attrs = PlayerCharacter->FindComponentByClass<UAttributesComponent>())
        {
            Attrs->OnCharacterDied.AddDynamic(this, &AGridTacticsGameMode::OnPlayerDied);
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
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetPause(true);
    }

    // 随机选择3个技能
    TArray<USkillDataAsset*> AvailableSkills;
    for (int32 i = 0; i < 3 && SkillPool.Num() > 0; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, SkillPool.Num() - 1);
        AvailableSkills.Add(SkillPool[RandomIndex]);
    }

    // TODO: 显示技能选择 UI，传入 AvailableSkills

    UE_LOG(LogTemp, Log, TEXT("GameMode: Showing skill selection (%d options)"), AvailableSkills.Num());
}

void AGridTacticsGameMode::OnSkillSelectionComplete()
{
    bWaitingForSkillSelection = false;

    // 恢复游戏
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetPause(false);
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
