// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMapData.h"

void UGridMapData::InitializeGrid()
{
    // 自动调整 GridCells 数组大小
    GridCells.SetNumZeroed(MapWidth * MapHeight);

    // 默认全设为 Walkable
    for (int32 i = 0; i < GridCells.Num(); ++i)
    {
        GridCells[i] = EGridCellType::Walkable;
    }
}
