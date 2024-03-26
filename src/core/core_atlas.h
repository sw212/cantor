#ifndef CORE_ATLAS_H
#define CORE_ATLAS_H

typedef u32 AtlasFlag;
enum
{
    AtlasFlag_Occupied = (1<<0),
};

struct AtlasNode
{
    AtlasNode* parent;
    AtlasNode* children[Corner_Count];
    u64        child_count;

    Vec2_i64   max_space[Corner_Count];
    AtlasFlag  flags;
};

struct Atlas
{
    Vec2_i64   size;
    AtlasNode* root;
};


function Atlas*    AtlasInitialise(Arena* arena, Vec2_i64 dim);
function Rect2_i64 AtlasAllocate(Arena* arena, Atlas* atlas, Vec2_i64 size);
function void      AtlasRelease(Atlas* atlas, Rect2_i64 region);

#endif