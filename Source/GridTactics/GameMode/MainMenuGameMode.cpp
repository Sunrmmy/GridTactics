// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuGameMode.h"
#include "GridTactics/MainMenu/LevelDataAsset.h"
#include "GridTactics/Skills/SkillDataAsset.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AMainMenuGameMode::AMainMenuGameMode()
{
    // 禁用默认 Pawn 生成
    DefaultPawnClass = nullptr;
}

void AMainMenuGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 播放主菜单音乐
    if (MenuMusic)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), MenuMusic, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
    }

    // 显示鼠标并设置输入模式
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
    }

    // 显示主菜单
    ShowMainMenu();

    UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Initialized with %d levels"), AvailableLevels.Num());
}

void AMainMenuGameMode::ShowMainMenu()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: PlayerController not found"));
        return;
    }

    if (!MainMenuWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: MainMenuWidgetClass is not set! Please assign it in BP_MainMenuGameMode"));
        return;
    }

    // 创建主菜单 Widget
    MainMenuWidget = CreateWidget<UUserWidget>(PC, MainMenuWidgetClass);
    if (MainMenuWidget)
    {
        MainMenuWidget->AddToViewport(0); // Z Order = 0
        UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Main menu displayed"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: Failed to create main menu widget"));
    }
}

void AMainMenuGameMode::LoadLevel(ULevelDataAsset* LevelData)
{
    if (!LevelData)
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: Invalid level data"));
        return;
    }

    if (!LevelData->bIsUnlocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("MainMenuGameMode: Level %s is locked"), 
            *LevelData->LevelName.ToString());
        // TODO: 显示"关卡未解锁"提示
        return;
    }

    // 检查 TSoftObjectPtr 是否为空（IsNull 而不是 IsValid）
    if (LevelData->LevelMap.IsNull())
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: Level map is null for %s"),
            *LevelData->LevelName.ToString());
        return;
    }

    // 获取资产路径（ToSoftObjectPath 而不是 GetLongPackageName）
    FSoftObjectPath LevelPath = LevelData->LevelMap.ToSoftObjectPath();
    FString LevelPathString = LevelPath.GetLongPackageName();

    // 检查路径是否有效
    if (LevelPathString.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuGameMode: Level map path is empty for %s"),
            *LevelData->LevelName.ToString());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Loading level: %s"), *LevelPathString);

    // 使用 FName 加载关卡
    UGameplayStatics::OpenLevel(GetWorld(), FName(*LevelPathString));
}

void AMainMenuGameMode::PlaySFX(USoundBase* Sound)
{
    if (Sound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), Sound, 1.0f);
    }
}

void AMainMenuGameMode::QuitGame()
{
    UE_LOG(LogTemp, Log, TEXT("MainMenuGameMode: Quitting game"));

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
    }
}
