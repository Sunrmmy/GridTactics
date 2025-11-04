// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class AGridTacticsPlayerState;
UCLASS()
class GRIDTACTICS_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnUpdatePlayerState();

	UFUNCTION(BlueprintPure, Category = "UI")
	AGridTacticsPlayerState* GetGridTacticsPlayerState() const;

	virtual void NativeTick(const  FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TWeakObjectPtr<AGridTacticsPlayerState> CachedPlayerState;
};
