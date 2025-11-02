// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMapManager.h"
#include "DrawDebugHelpers.h"
#include "Editor.h"

// Sets default values
AGridMapManager::AGridMapManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGridMapManager::BeginPlay()
{
	Super::BeginPlay();
	LoadMapData();
#if WITH_EDITOR
	if (bShowDebugGrid && GetWorld()&& GetWorld()->WorldType == EWorldType::Game)
	{
		DrawDebugGrid();
	}
#endif
}

// Called every frame
void AGridMapManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
//#if WITH_EDITOR
//	if (bShowDebugGrid && GetWorld()/* && GetWorld()->WorldType == EWorldType::Editor*/)
//	{
//		DrawDebugGrid();
//	}
//#endif
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
	CachedGridSize = LoadedMap->GridSize;

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









/////////////////////////////////////////////////////

#if WITH_EDITOR
void AGridMapManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 检查是否修改了 MapDataAsset
	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AGridMapManager, MapDataAsset))
	{
		// 重新加载数据并绘制
		LoadMapData();
		DrawDebugGrid();
	}
}

void AGridMapManager::DrawDebugGrid()
{
	UE_LOG(LogTemp, Warning, TEXT("DrawDebugGrid called! Map: %dx%d"), MapWidth, MapHeight);
	if (!GetWorld()) return;

	// 获取当前加载的地图数据（从缓存 Grid2D）
	if (MapWidth <= 0 || MapHeight <= 0) return;

	float HalfGridSize = CachedGridSize * 0.5f;
	const FVector BaseLocation = GetActorLocation();

	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			EGridCellType CellType = Grid2D[Y][X];
			FColor CellColor = (CellType == EGridCellType::Walkable) ? FColor::Green : FColor::Red;

			// 计算每个格子中心世界坐标
			FVector CellCenter = BaseLocation
				+ FVector(X * CachedGridSize, Y * CachedGridSize, 10.0f); // 抬高一点避免 Z-fighting

			// 绘制网格线（边框）
			DrawDebugBox(
				GetWorld(),
				CellCenter,
				FVector(HalfGridSize, HalfGridSize, 5.0f),
				FColor::Black,
				false,
				-1.0f,
				0,
				1.0f
			);
		}
	}
}
#endif