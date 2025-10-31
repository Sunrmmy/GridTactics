// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMapManager.h"

// Sets default values
AGridMapManager::AGridMapManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AGridMapManager::BeginPlay()
{
	Super::BeginPlay();
	LoadMapData();
}

// Called every frame
void AGridMapManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGridMapManager::LoadMapData() {
	if (MapDataAsset.IsNull()) {
		UE_LOG(LogTemp, Warning, TEXT("GridMapManager: MapDataAsset is not assigned!"));
		return;
	}
	UGridMapData* LoadedMap = MapDataAsset.LoadSynchronous();
	if (!LoadedMap) {
		UE_LOG(LogTemp, Error, TEXT("GridMapManager: Failed to load MapDataAsset!"));
		return;
	}

	// 缓存地图尺寸
	MapWidth = LoadedMap->MapWidth;
	MapHeight = LoadedMap->MapHeight;

	// 构建Grid2D: 外层数组 = 行（Y），内层数组 = 列（X）
	Grid2D.Empty(MapHeight);
	Grid2D.SetNum(MapHeight);
    for (int32 Y = 0; Y < MapHeight; ++Y)
    {
        Grid2D[Y].Empty(MapWidth);
        Grid2D[Y].SetNum(MapWidth);

        for (int32 X = 0; X < MapWidth; ++X)
        {
            const int32 FlatIndex = Y * MapWidth + X;
            if (FlatIndex < LoadedMap->GridCells.Num())
            {
                Grid2D[Y][X] = LoadedMap->GridCells[FlatIndex];
            }
            else
            {
                // 数据不足时默认为障碍（安全兜底）
                Grid2D[Y][X] = EGridCellType::Blocked;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GridMapManager: Loaded map %dx%d"), MapWidth, MapHeight);
}

// 获取指定格子的类型
EGridCellType AGridMapManager::GetCellType(int32 X, int32 Y) const
{
    // 边界检查：确保 X, Y 在 [0, Width) x [0, Height) 范围内
    if (X < 0 || X >= MapWidth || Y < 0 || Y >= MapHeight)
    {
        return EGridCellType::Blocked; // 越界视为不可通行
    }

    return Grid2D[Y][X]; // 注意：Y 是行，X 是列
}

bool AGridMapManager::IsWalkable(int32 X, int32 Y) const
{
    return GetCellType(X, Y) == EGridCellType::Walkable;
}