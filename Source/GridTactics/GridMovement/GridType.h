#pragma once

UENUM(BlueprintType)
enum class EGridCellType : uint8
{
    Walkable,
    Blocked,
    Water,      // 未来扩展
    Lava        // 未来扩展
};