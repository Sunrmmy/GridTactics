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
#include "Components/AudioComponent.h"

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

    // 播放准备阶段音乐
    if (PreparationMusic)
    {
        PlayMusic(PreparationMusic, 2.0f);
    }

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

    // 切换到战斗音乐
    if (CombatMusic)
    {
        PlayMusic(CombatMusic, 1.5f);
    }

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

    // 播放波次开始音效
    if (WaveStartSound)
    {
        PlaySFX(WaveStartSound);
    }

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

    // 停止背景音乐并播放失败音效
    StopMusic(1.0f);

    if (DefeatSound)
    {
        // 延迟播放，让音乐先淡出一点
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle,
            [this]()
            {
                PlaySFX(DefeatSound);
            },
            0.5f,
            false
        );
    }
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

    // 播放敌人击败音效
    if (EnemyDefeatSound)
    {
        PlaySFX(EnemyDefeatSound);
    }

    // 广播事件
    OnEnemyDefeated.Broadcast(Enemy);

    // 延迟显示技能选择
    if (!bWaitingForSkillSelection)
    {
        bWaitingForSkillSelection = true;

        // 使用 Timer 延迟显示
        FTimerHandle DelayTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            DelayTimerHandle,
            this,
            &AGridTacticsGameMode::ShowSkillSelection,
            1.5f,  // 延迟
            false
        );
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

    // 只在技能列表为空时才随机选择（避免重复刷新）
    if (CurrentSkillOptions.Num() == 0)
    {
        // 随机选择3个技能
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

        UE_LOG(LogTemp, Log, TEXT("GameMode: Generated %d skill options"), CurrentSkillOptions.Num());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("GameMode: Reusing existing %d skill options"), CurrentSkillOptions.Num());
    }

    // 加载技能选择界面
    UClass* WidgetClass = LoadClass<UUserWidget>(
        nullptr,
        TEXT("/Game/UI/WBP_SelectSkill.WBP_SelectSkill_C")
    );

    if (WidgetClass)
    {
        UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (Widget)
        {
            Widget->AddToViewport(999);  // 高优先级
            UE_LOG(LogTemp, Log, TEXT("GameMode: WBP_SelectSkill displayed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to load WBP_SelectSkill"));
    }
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
            // 技能槽未满，直接添加成功
            UE_LOG(LogTemp, Log, TEXT("GameMode: Player selected skill: %s"), 
                *SelectedSkill->SkillName.ToString());

            // 清空技能选项列表（下次敌人死亡时重新生成）
            CurrentSkillOptions.Empty();

            // 完成技能选择
            OnSkillSelectionComplete();
        }
        else
        {
            // 技能槽已满，显示替换界面
            UE_LOG(LogTemp, Warning, TEXT("GameMode: Skill slots full, showing replacement UI"));
            
            // 保存待添加的技能
            PendingNewSkill = SelectedSkill;
            
            // 显示替换界面（不调用 OnSkillSelectionComplete，保持暂停）
            ShowSkillReplacement(SelectedSkill);
        }
    }
}

// 显示技能替换界面
void AGridTacticsGameMode::ShowSkillReplacement(USkillDataAsset* NewSkill)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    // 确保游戏保持暂停状态
    if (!PC->IsPaused())
    {
        PC->SetPause(true);
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Re-pausing game for replacement UI"));
    }
    
    PC->bShowMouseCursor = true;

    // 加载 WBP_ReplaceSkill
    UClass* WidgetClass = LoadClass<UUserWidget>(
        nullptr,
        TEXT("/Game/UI/WBP_ReplaceSkill.WBP_ReplaceSkill_C")
    );

    if (WidgetClass)
    {
        UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (Widget)
        {
            Widget->AddToViewport(1000);  // 更高优先级
            UE_LOG(LogTemp, Log, TEXT("GameMode: WBP_ReplaceSkill displayed for skill: %s"),
                *NewSkill->SkillName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to load WBP_ReplaceSkill"));
    }
}

// 玩家确认替换技能
void AGridTacticsGameMode::OnPlayerConfirmReplace(int32 SlotIndex)
{
    if (!PendingNewSkill || !PlayerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: No pending skill to replace"));
        return;
    }

    if (USkillComponent* SkillComp = PlayerCharacter->FindComponentByClass<USkillComponent>())
    {
        if (SkillComp->ReplaceSkill(SlotIndex, PendingNewSkill))
        {
            UE_LOG(LogTemp, Log, TEXT("GameMode: Replaced skill at slot %d with %s"),
                SlotIndex, *PendingNewSkill->SkillName.ToString());

            // 清空临时技能
            PendingNewSkill = nullptr;

            // 清空技能选项列表
            CurrentSkillOptions.Empty();

            // 完成技能选择
            OnSkillSelectionComplete();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to replace skill at slot %d"), SlotIndex);
        }
    }
}

// 玩家取消替换
void AGridTacticsGameMode::OnPlayerCancelReplace()
{
    UE_LOG(LogTemp, Log, TEXT("GameMode: Player cancelled skill replacement"));

    // 清空临时技能
    PendingNewSkill = nullptr;

    // 修复：不重新生成技能选项，直接显示之前的选择界面
    // CurrentSkillOptions 保持不变

    // 重新显示技能选择界面（使用已有的技能选项）
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    // 确保暂停状态
    PC->SetPause(true);
    PC->bShowMouseCursor = true;

    // 加载 WBP_SelectSkill
    UClass* WidgetClass = LoadClass<UUserWidget>(
        nullptr,
        TEXT("/Game/UI/WBP_SelectSkill.WBP_SelectSkill_C")
    );

    if (WidgetClass)
    {
        UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (Widget)
        {
            Widget->AddToViewport(999);
            UE_LOG(LogTemp, Log, TEXT("GameMode: Re-showing WBP_SelectSkill with existing options"));
        }
    }
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

    UE_LOG(LogTemp, Log, TEXT("GameMode: Skill selection complete, game unpaused"));
}

void AGridTacticsGameMode::CheckVictoryCondition()
{
    SetGamePhase(EGamePhase::Victory);

    UE_LOG(LogTemp, Log, TEXT("GameMode: VICTORY!"));

    // 停止背景音乐并播放胜利音效
    StopMusic(1.0f);

    if (VictorySound)
    {
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle,
            [this]()
            {
                PlaySFX(VictorySound);
            },
            0.5f,
            false
        );
    }
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






// ========================================
// 音频管理函数
// ========================================

void AGridTacticsGameMode::PlayMusic(USoundBase* Music, float FadeInTime)
{
    if (!Music)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: Attempted to play null music"));
        return;
    }

    // 如果已经在播放相同的音乐，跳过
    if (CurrentMusic == Music && MusicAudioComponent && MusicAudioComponent->IsPlaying())
    {
        return;
    }

    // 停止当前音乐
    if (MusicAudioComponent && MusicAudioComponent->IsPlaying())
    {
        MusicAudioComponent->Stop();
    }

    // 清理旧的 Timer
    GetWorld()->GetTimerManager().ClearTimer(MusicFadeTimerHandle);

    // 创建或复用 AudioComponent
    if (!MusicAudioComponent)
    {
        MusicAudioComponent = NewObject<UAudioComponent>(this);
        MusicAudioComponent->bAllowSpatialization = false;  // 2D 音乐
        MusicAudioComponent->bIsUISound = true;  //不受暂停影响
        MusicAudioComponent->RegisterComponent();
    }

    CurrentMusic = Music;
    MusicAudioComponent->SetSound(Music);
    MusicAudioComponent->SetVolumeMultiplier(0.0f);  // 从 0 开始淡入
    MusicAudioComponent->Play();

    // 设置淡入参数
    if (FadeInTime > 0.0f)
    {
        bIsFadingIn = true;
        MusicFadeProgress = 0.0f;
        MusicFadeStartVolume = 0.0f;
        MusicFadeTargetVolume = MusicVolume;
        MusicFadeDuration = FadeInTime;

        GetWorld()->GetTimerManager().SetTimer(
            MusicFadeTimerHandle,
            this,
            &AGridTacticsGameMode::UpdateMusicFade,
            0.05f,  // 每 50ms 更新一次
            true
        );
    }
    else
    {
        MusicAudioComponent->SetVolumeMultiplier(MusicVolume);
    }

    UE_LOG(LogTemp, Log, TEXT("GameMode: Playing music with %.2fs fade-in"), FadeInTime);
}

void AGridTacticsGameMode::StopMusic(float FadeOutTime)
{
    if (!MusicAudioComponent || !MusicAudioComponent->IsPlaying())
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(MusicFadeTimerHandle);

    if (FadeOutTime > 0.0f)
    {
        bIsFadingIn = false;
        MusicFadeProgress = 0.0f;
        MusicFadeStartVolume = MusicAudioComponent->VolumeMultiplier;
        MusicFadeTargetVolume = 0.0f;
        MusicFadeDuration = FadeOutTime;

        GetWorld()->GetTimerManager().SetTimer(
            MusicFadeTimerHandle,
            this,
            &AGridTacticsGameMode::UpdateMusicFade,
            0.05f,
            true
        );
    }
    else
    {
        MusicAudioComponent->Stop();
    }

    UE_LOG(LogTemp, Log, TEXT("GameMode: Stopping music with %.2fs fade-out"), FadeOutTime);
}

void AGridTacticsGameMode::UpdateMusicFade()
{
    if (!MusicAudioComponent)
    {
        GetWorld()->GetTimerManager().ClearTimer(MusicFadeTimerHandle);
        return;
    }

    MusicFadeProgress += 0.05f;
    float Alpha = FMath::Clamp(MusicFadeProgress / MusicFadeDuration, 0.0f, 1.0f);
    float NewVolume = FMath::Lerp(MusicFadeStartVolume, MusicFadeTargetVolume, Alpha);

    MusicAudioComponent->SetVolumeMultiplier(NewVolume);

    if (Alpha >= 1.0f)
    {
        OnMusicFadeComplete();
    }
}

void AGridTacticsGameMode::OnMusicFadeComplete()
{
    GetWorld()->GetTimerManager().ClearTimer(MusicFadeTimerHandle);

    // 如果是淡出，停止播放
    if (!bIsFadingIn && MusicAudioComponent)
    {
        MusicAudioComponent->Stop();
    }
}

void AGridTacticsGameMode::PlaySFX(USoundBase* Sound)
{
    if (!Sound)
    {
        return;
    }

    // 使用 bIsUISound = true，使音效不受暂停影响
    UGameplayStatics::PlaySound2D(
        GetWorld(),
        Sound,
        SFXVolume,
        1.0f,  // Pitch
        0.0f,  // StartTime
        nullptr,
        nullptr,
        true  // bIsUISound
    );

    UE_LOG(LogTemp, Log, TEXT("GameMode: Played SFX"));
}

void AGridTacticsGameMode::SetMusicVolume(float Volume)
{
    MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);

    if (MusicAudioComponent && MusicAudioComponent->IsPlaying())
    {
        MusicAudioComponent->SetVolumeMultiplier(MusicVolume);
    }
}

void AGridTacticsGameMode::SetSFXVolume(float Volume)
{
    SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
}