#pragma once

#include "CoreMinimal.h"
#include "GameTypes.generated.h"

/**
 * 游戏阶段
 */
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Preparation     UMETA(DisplayName = "准备阶段"),
    Combat          UMETA(DisplayName = "战斗阶段"),
    SkillSelection  UMETA(DisplayName = "技能选择"),
    Victory         UMETA(DisplayName = "胜利"),
    Defeat          UMETA(DisplayName = "失败")
};

/**
 * 敌人生成配置
 */
USTRUCT(BlueprintType)
struct FEnemySpawnConfig
{
    GENERATED_BODY()

    /** 敌人类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<ACharacter> EnemyClass;

    /** 生成位置（网格坐标） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint SpawnGrid;

    /** 敌人等级 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 1;
};

/**
 * 波次配置
 */
USTRUCT(BlueprintType)
struct FWaveConfig
{
    GENERATED_BODY()

    /** 波次名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText WaveName;

    /** 敌人列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEnemySpawnConfig> Enemies;

    /** 波次开始延迟（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StartDelay = 2.0f;
};