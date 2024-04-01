#ifndef CORE_ARENA_H
#define CORE_ARENA_H

#if !defined(ALLOCATION_GRANULARITY)
#define ALLOCATION_GRANULARITY Kilobytes(4)
#endif

#if !defined(DEALLOCATION_GRANULARITY)
#define DEALLOCATION_GRANULARITY Megabytes(64)
#endif


struct Arena
{
    u64    pos;
    u64    committed;
    u64    size;
    u64    alignment;
    Arena* ptr;
    u64   _pad[3];
};

struct Scratch
{
    Arena* arena;
    u64    pos;
};


function Arena* ArenaAlloc(u64 size);
function u64    ArenaPos(Arena* arena);
function void*  ArenaPushNZ(Arena* arena, u64 size);
function void*  ArenaPush(Arena* arena, u64 size);
function void   ArenaPopTo(Arena* arena, u64 pos);
function void   ArenaPop(Arena* arena, u64 size);
function void   ArenaClear(Arena* arena);
function void   ArenaRelease(Arena* arena);

#define PushArrayNZ(arena, type, count) (type *)ArenaPushNZ((arena), sizeof(type)*(count))
#define PushArray(arena, type, count)   (type *)ArenaPush((arena), sizeof(type)*(count))


function Scratch ScratchBegin(Arena** conflicts, u64 conflict_count);
function void    ScratchEnd(Scratch scratch);

#endif