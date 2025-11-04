// Fill out your copyright notice in the Description page of Project Settings.


#include "HUDWidget.h"
#include "GameFramework/PlayerState.h"
#include "GridTacticsPlayerState.h"
#include "Kismet/GameplayStatics.h"

AGridTacticsPlayerState* UHUDWidget::GetGridTacticsPlayerState() const
{
    return CachedPlayerState.Get();
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!CachedPlayerState.IsValid())
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        if (PlayerController)
        {
            CachedPlayerState = PlayerController->GetPlayerState<AGridTacticsPlayerState>();
        }
    }

    if (CachedPlayerState.IsValid())
    {
        //调用蓝图事件来更新UI
        OnUpdatePlayerState();
    }
}

