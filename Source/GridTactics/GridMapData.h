// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridType.h"
#include "GridMapData.generated.h"

/**
 * 
 */
UCLASS()
class GRIDTACTICS_API UGridMapData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 MapWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 MapHeight = 10;

	// 每个格子的实际尺寸（单位：厘米）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	float GridSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TArray<EGridCellType> GridCells;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Grid")
	void InitializeGrid();
};
