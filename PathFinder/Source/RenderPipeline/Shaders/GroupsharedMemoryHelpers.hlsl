#ifndef _GroupsharedMemoryHelpers__
#define _GroupsharedMemoryHelpers__

struct GSBoxLoadStoreCoords
{
    int2 LoadCoord0;
    int2 LoadCoord1;
    int2 LoadCoord2;
    int2 LoadCoord3;

    int2 StoreCoord0;
    int2 StoreCoord1;
    int2 StoreCoord2;
    int2 StoreCoord3;

    bool IsLoadStore1Required;
    bool IsLoadStore2Required;
    bool IsLoadStore3Required;
};

GSBoxLoadStoreCoords GetGSBoxLoadStoreCoords(int2 dispatchIndex, int2 threadIndex, int2 textureSize, int groupSize, int radius)
{
    GSBoxLoadStoreCoords coords;
    coords.IsLoadStore1Required = false;
    coords.IsLoadStore2Required = false;
    coords.IsLoadStore3Required = false;

    dispatchIndex = clamp(dispatchIndex, 0, textureSize - 1);

    // Account for additional pixels (Radius) around the group
    int2 arrayIndex = threadIndex + radius;

    coords.LoadCoord0 = dispatchIndex;
    coords.StoreCoord0 = arrayIndex;

    // A relative coordinate for diagonal sampling, if required
    int2 diagonalOffset = 0;

    // Sample horizontally, left side
    if (threadIndex.x < radius)
    {
        int2 loadIndex = int2(max(dispatchIndex.x - radius, 0), dispatchIndex.y);
        int2 storeIndex = int2(arrayIndex.x - radius, arrayIndex.y);

        diagonalOffset.x = -radius;

        coords.LoadCoord1 = loadIndex;
        coords.StoreCoord1 = storeIndex;
        coords.IsLoadStore1Required = true;
    }

    // Sample vertically, top side
    if (threadIndex.y < radius)
    {
        int2 loadIndex = int2(dispatchIndex.x, max(dispatchIndex.y - radius, 0));
        int2 storeIndex = int2(arrayIndex.x, arrayIndex.y - radius);

        diagonalOffset.y = -radius;

        coords.LoadCoord2 = loadIndex;
        coords.StoreCoord2 = storeIndex;
        coords.IsLoadStore2Required = true;
    }

    // Sample horizontally, right side
    if (threadIndex.x >= (groupSize - radius))
    {
        int2 loadIndex = int2(min(dispatchIndex.x + radius, textureSize.x - 1), dispatchIndex.y);
        int2 storeIndex = int2(arrayIndex.x + radius, arrayIndex.y);

        diagonalOffset.x = radius;

        coords.LoadCoord1 = loadIndex;
        coords.StoreCoord1 = storeIndex;
        coords.IsLoadStore1Required = true;
    }

    // Sample vertically, bottom side
    if (threadIndex.y >= (groupSize - radius))
    {
        int2 loadIndex = int2(dispatchIndex.x, min(dispatchIndex.y + radius, textureSize.y - 1));
        int2 storeIndex = int2(arrayIndex.x, arrayIndex.y + radius);

        diagonalOffset.y = radius;

        coords.LoadCoord2 = loadIndex;
        coords.StoreCoord2 = storeIndex;
        coords.IsLoadStore2Required = true;
    }

    // Sample diagonally
    if (all(diagonalOffset != 0))
    {
        int2 loadIndex = clamp(int2(dispatchIndex + diagonalOffset), 0, textureSize - 1);
        int2 storeIndex = int2(arrayIndex + diagonalOffset);

        coords.LoadCoord3 = loadIndex;
        coords.StoreCoord3 = storeIndex;
        coords.IsLoadStore3Required = true;
    }

    return coords;
}


#endif