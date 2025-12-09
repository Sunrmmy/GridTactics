// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameTypes.h"
#include "GridTacticsGameMode.generated.h"

class AHeroCharacter;
class USkillDataAsset;

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

    // 新增：玩家选择技能
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void OnPlayerSelectSkill(USkillDataAsset* SelectedSkill);

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
};
