// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridType.h"
#include "GridMapData.h"
#include "GridMapManager.generated.h"

UCLASS()
class GRIDTACTICS_API AGridMapManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGridMapManager();

	// 查询某个格子是否可行走（蓝图和 C++ 都能调用）
	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsWalkable(int32 X, int32 Y) const;

	// 获取格子类型
	UFUNCTION(BlueprintPure, Category = "Grid")
	EGridCellType GetCellType(int32 X, int32 Y) const;

	// 重新加载地图（可用于切换关卡）
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void LoadMapData();

protected:
	// 在编辑器中指定要加载的地图数据
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TSoftObjectPtr<UGridMapData> MapDataAsset;		// 使用 TSoftObjectPtr 是为了避免硬引用，支持按需加载

private:
	// 缓存解析后的二维地图Grid2D[Y][X]（提升查询效率）
	TArray<TArray<EGridCellType>> Grid2D;

	// 当前地图尺寸
	int32 MapWidth = 0;
	int32 MapHeight = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
