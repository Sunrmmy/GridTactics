// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuGameMode.h"
#include "GridTactics/MainMenu/LevelDataAsset.h"
#include "GridTactics/SkillDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AMenuGameMode::AMenuGameMode()
{
    // 禁用默认 Pawn 生成
    DefaultPawnClass = nullptr;
}

void AMenuGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 播放主菜单音乐
    if (MenuMusic)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), MenuMusic, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
    }

    // 显示鼠标
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

    UE_LOG(LogTemp, Log, TEXT("MenuGameMode: Main menu initialized with %d levels and %d skills"),
        AvailableLevels.Num(), AllSkills.Num());
}

void AMenuGameMode::LoadLevel(ULevelDataAsset* LevelData)
{
    if (!LevelData)
    {
        UE_LOG(LogTemp, Error, TEXT("MenuGameMode: Invalid level data"));
        return;
    }

    if (!LevelData->bIsUnlocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("MenuGameMode: Level %s is locked"), *LevelData->LevelName.ToString());
        return;
    }

    if (!LevelData->LevelMap.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("MenuGameMode: Level map path is invalid for %s"),
            *LevelData->LevelName.ToString());
        return;
    }

    // 加载关卡
    FString LevelPath = LevelData->LevelMap.GetLongPackageName();
    UE_LOG(LogTemp, Log, TEXT("MenuGameMode: Loading level: %s"), *LevelPath);

    UGameplayStatics::OpenLevel(GetWorld(), FName(*LevelPath));
}

TArray<USkillDataAsset*> AMenuGameMode::GetSkillsByType(ESkillTargetType TargetType) const
{
    TArray<USkillDataAsset*> FilteredSkills;

    for (USkillDataAsset* Skill : AllSkills)
    {
        if (Skill && Skill->TargetType == TargetType)
        {
            FilteredSkills.Add(Skill);
        }
    }

    return FilteredSkills;
}

void AMenuGameMode::PlaySFX(USoundBase* Sound)
{
    if (Sound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), Sound, 1.0f);
    }
}

void AMenuGameMode::QuitGame()
{
    UE_LOG(LogTemp, Log, TEXT("MenuGameMode: Quitting game"));

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
    }
}
