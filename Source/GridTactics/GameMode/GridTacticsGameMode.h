// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameTypes.h"
#include "GridTacticsGameMode.generated.h"

class AHeroCharacter;
class USkillDataAsset;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveChanged, int32, CurrentWave, int32, TotalWaves);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDefeated, ACharacter*, Enemy);

UCLASS()
class GRIDTACTICS_API AGridTacticsGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
    AGridTacticsGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ========================================
    // 游戏状态管理
    // ========================================

    /** 当前游戏阶段 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    EGamePhase CurrentPhase = EGamePhase::Preparation;

    /** 当前波次索引（0-based） */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentWaveIndex = 0;

    /** 当前波次剩余敌人数量 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 RemainingEnemies = 0;

    /** 玩家角色引用 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    TObjectPtr<AHeroCharacter> PlayerCharacter;

    // ========================================
    // 关卡配置
    // ========================================

    /** 玩家生成位置（网格坐标） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
    FIntPoint PlayerSpawnGrid = FIntPoint(5, 5);

    /** 波次配置列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
    TArray<FWaveConfig> WaveConfigs;

    /** 可供选择的技能池 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
    TArray<TObjectPtr<USkillDataAsset>> SkillPool;

    /** 初始可选技能数量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
    int32 InitialSkillCount = 2;

    // ========================================
    // 事件委托
    // ========================================

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGamePhaseChanged OnGamePhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWaveChanged OnWaveChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEnemyDefeated OnEnemyDefeated;

    // ========================================
    // 音频配置
    // ========================================

    /** 准备阶段背景音乐 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Music")
    TObjectPtr<USoundBase> PreparationMusic;

    /** 战斗阶段背景音乐 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Music")
    TObjectPtr<USoundBase> CombatMusic;

    /** 胜利音效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
    TObjectPtr<USoundBase> VictorySound;

    /** 失败音效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
    TObjectPtr<USoundBase> DefeatSound;

    /** 波次开始音效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
    TObjectPtr<USoundBase> WaveStartSound;

    /** 敌人击败音效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
    TObjectPtr<USoundBase> EnemyDefeatSound;

    /** 背景音乐音量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MusicVolume = 0.5f;

    /** 音效音量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SFXVolume = 1.0f;

    /** 是否在暂停时静音音频 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Settings")
    bool bMuteOnPause = false;

    // ========================================
    // 音频控制接口
    // ========================================

    /** 播放背景音乐 */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayMusic(USoundBase* Music, float FadeInTime = 1.0f);

    /** 停止背景音乐 */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void StopMusic(float FadeOutTime = 1.0f);

    /** 播放音效 */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlaySFX(USoundBase* Sound);

    /** 设置音乐音量 */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetMusicVolume(float Volume);

    /** 设置音效音量 */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetSFXVolume(float Volume);

    // ========================================
    // 公开接口
    // ========================================

    /** 临时存储准备阶段选择的技能 */
    UPROPERTY(BlueprintReadWrite, Category = "Preparation")
    TArray<TObjectPtr<USkillDataAsset>> PendingSkills;

    /** 玩家准备完毕 */
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void OnPlayerReady();

    /** 玩家选择技能完成 */
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void OnSkillSelectionComplete();

    /** 修改：敌人死亡回调（从 AttributesComponent 触发） */
    UFUNCTION()
    void OnEnemyDied(AActor* Enemy);

    /** 修改：玩家死亡回调 */
    UFUNCTION()
    void OnPlayerDied(AActor* Player);

    // 玩家角色蓝图类
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
    TSubclassOf<AHeroCharacter> PlayerCharacterClass;

    // 新增：当前可选技能列表
    UPROPERTY(BlueprintReadWrite, Category = "Skill Selection")
    TArray<TObjectPtr<USkillDataAsset>> CurrentSkillOptions;

    /** 玩家选择技能 */
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void OnPlayerSelectSkill(USkillDataAsset* SelectedSkill);

    /** 新增：玩家选择替换技能 */
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void OnPlayerConfirmReplace(int32 SlotIndex);

    /** 新增：玩家取消替换 */
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void OnPlayerCancelReplace();

protected:
    /** 切换游戏阶段 */
    void SetGamePhase(EGamePhase NewPhase);

    /** 生成玩家 */
    void SpawnPlayer();

    /** 开始新波次 */
    void StartNextWave();

    /** 生成敌人 */
    void SpawnEnemies(const FWaveConfig& WaveConfig);

    /** 通知敌人被击败（内部处理） */
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void NotifyEnemyDefeated(ACharacter* Enemy);

    /** 显示技能选择界面 */
    void ShowSkillSelection();

    /** 新增：显示技能替换界面 */
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void ShowSkillReplacement(USkillDataAsset* NewSkill);

    /** 检查胜利条件 */
    void CheckVictoryCondition();

    /** 新增：波次完成 */
    void OnWaveComplete();

    // 新增：显示准备界面
    void ShowPreparationUI();

    // 覆盖默认生成逻辑
    virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
    virtual void RestartPlayer(AController* NewPlayer) override;

private:
    /** 当前波次生成的敌人列表 */
    UPROPERTY()
    TArray<TObjectPtr<ACharacter>> CurrentWaveEnemies;

    /** 等待技能选择的标志 */
    bool bWaitingForSkillSelection = false;

    /** 新增：临时存储待添加的技能（技能槽满时使用） */
    UPROPERTY()
    TObjectPtr<USkillDataAsset> PendingNewSkill;

    /** 背景音乐 Audio Component */
    UPROPERTY()
    TObjectPtr<UAudioComponent> MusicAudioComponent;

    /** 当前播放的音乐 */
    UPROPERTY()
    TObjectPtr<USoundBase> CurrentMusic;

    /** 音乐淡入淡出的 Timer */
    FTimerHandle MusicFadeTimerHandle;
    float MusicFadeProgress = 0.0f;
    float MusicFadeStartVolume = 0.0f;
    float MusicFadeTargetVolume = 0.0f;
    float MusicFadeDuration = 0.0f;
    bool bIsFadingIn = false;

    /** 内部音频管理函数 */
    void UpdateMusicFade();
    void OnMusicFadeComplete();
};
