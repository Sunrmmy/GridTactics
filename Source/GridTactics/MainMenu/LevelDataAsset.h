// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelDataAsset.generated.h"

/**
 * 关卡配置数据资产
 * 用于存储关卡信息（名称、描述、难度、地图等）
 */
UCLASS(BlueprintType)
class GRIDTACTICS_API ULevelDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 关卡显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Info")
    FText LevelName;

    /** 关卡描述 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Info", meta = (MultiLine = true))
    FText LevelDescription;

    /** 关卡难度（1-5） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Info", meta = (ClampMin = "1", ClampMax = "5"))
    int32 Difficulty = 1;

    /** 关卡缩略图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Info")
    TObjectPtr<UTexture2D> ThumbnailImage;

    /** 关卡地图路径（例如："/Game/Maps/Level_01"） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Info")
    TSoftObjectPtr<UWorld> LevelMap;

    /** 是否已解锁 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Info")
    bool bIsUnlocked = true;

    /** 关卡奖励（金币、经验等） */
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Info")
    //int32 RewardGold = 100;
};