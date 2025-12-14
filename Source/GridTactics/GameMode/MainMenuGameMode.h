// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class ULevelDataAsset;
class USkillDataAsset;
class UUserWidget; // 添加这行

/**
 * 主菜单专用 GameMode
 * 管理关卡列表、技能图鉴等数据
 */
UCLASS()
class GRIDTACTICS_API AMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMainMenuGameMode();

    virtual void BeginPlay() override;

    // ========================================
    // 关卡管理
    // ========================================

    /** 所有可用关卡列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Management")
    TArray<TObjectPtr<ULevelDataAsset>> AvailableLevels;

    /** 获取所有关卡 */
    UFUNCTION(BlueprintPure, Category = "Level Management")
    TArray<ULevelDataAsset*> GetAllLevels() const { return AvailableLevels; }

    /** 加载指定关卡 */
    UFUNCTION(BlueprintCallable, Category = "Level Management")
    void LoadLevel(ULevelDataAsset* LevelData);

    // ========================================
    // 技能图鉴
    // ========================================

    /** 所有技能列表（用于图鉴显示） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Encyclopedia")
    TArray<TObjectPtr<USkillDataAsset>> AllSkills;

    /** 获取所有技能 */
    UFUNCTION(BlueprintPure, Category = "Skill Encyclopedia")
    TArray<USkillDataAsset*> GetAllSkills() const { return AllSkills; }

    // ========================================
    // UI 管理
    // ========================================

    /** 主菜单 Widget 类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> MainMenuWidgetClass;

    /** 主菜单音乐 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundBase> MenuMusic;

    /** 按钮点击音效 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundBase> ButtonClickSound;

    /** 播放音效 */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlaySFX(USoundBase* Sound);

    /** 退出游戏 */
    UFUNCTION(BlueprintCallable, Category = "Game Flow")
    void QuitGame();

protected:
    /** 显示主菜单 */
    void ShowMainMenu();

private:
    /** 缓存主菜单 Widget 实例 */
    UPROPERTY()
    TObjectPtr<UUserWidget> MainMenuWidget;
};