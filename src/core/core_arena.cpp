function Arena*
ArenaAlloc(u64 size)
{
    //
    // size is reserve size, initally commit only a single page
    //

    u64 alloc_size  = ((size + Megabytes(64) - 1) / Megabytes(64)) * Megabytes(64);
    u64 init_commit = ALLOCATION_GRANULARITY;

    void* block = Sys_ReserveMemory(alloc_size);

    Sys_CommitMemory(block, init_commit);

    Arena* arena = (Arena*)block;
    arena->pos        = sizeof(Arena);
    arena->committed = init_commit;
    arena->alignment  = 8;
    arena->size       = alloc_size;

    return arena;
}

function u64
ArenaPos(Arena* arena)
{
    return arena->pos;
}

function void*
ArenaPushNZ(Arena* arena, u64 size)
{
    void* result = 0;

    if ((arena->pos + size) <= arena->size)
    {
        u8* base = (u8*)arena;

        u64 pos = arena->pos;
        u64 alignment = arena->alignment;
        u64 aligned_pos = ((pos + alignment - 1) / alignment) * alignment;

        result     = base + aligned_pos;
        arena->pos = aligned_pos + size;

        if (arena->committed < arena->pos)
        {
            u64 commit = arena->pos - arena->committed;
            commit = ((commit + ALLOCATION_GRANULARITY - 1) / ALLOCATION_GRANULARITY) * ALLOCATION_GRANULARITY;

            Sys_CommitMemory(base + arena->committed, commit);
            arena->committed += commit;
        }
    }

    return result;
}

function void*
ArenaPush(Arena* arena, u64 size)
{
    void* result = ArenaPushNZ(arena, size);
    MemoryZero(result, size);
    
    return result;
}

function void
ArenaPopTo(Arena* arena, u64 pos)
{
    u64 next_pos = Max(sizeof(Arena), pos);
    arena->pos = next_pos;

    u64 aligned_pos = ((arena->pos + ALLOCATION_GRANULARITY - 1) / ALLOCATION_GRANULARITY) * ALLOCATION_GRANULARITY;

    if ((aligned_pos + DEALLOCATION_GRANULARITY) <= arena->committed)
    {
        u8* base = (u8*)arena;
        u64 decommit = arena->committed - aligned_pos;
        Sys_DecommitMemory(base + aligned_pos, decommit);
        arena->committed -= decommit;
    }
}

function void
ArenaPop(Arena* arena, u64 size)
{
    u64 pop_size = arena->pos - Min(arena->pos, size);
    u64 next_pos = Max(pop_size, sizeof(Arena));
    ArenaPopTo(arena, next_pos);
}

function void
ArenaClear(Arena *arena)
{
    ArenaPopTo(arena, sizeof(Arena));
}

function void
ArenaRelease(Arena* arena)
{
    Sys_ReleaseMemory(arena, arena->size);
}


function Scratch
ScratchBegin(Arena** conflicts, u64 conflict_count)
{
    Scratch result = {};
    ThreadContext* ctx = ThreadContextGet();

    for(u64 i = 0; i < ArrayCount(ctx->scratch); i++)
    {
        b32 conflicting = 0;

        for(Arena** conflict = conflicts; conflict < conflicts+conflict_count; conflict += 1)
        {
            if(*conflict == ctx->scratch[i])
            {
                conflicting = 1;
                break;
            }
        }

        if(!conflicting)
        {
            result.arena = ctx->scratch[i];
            result.pos   = result.arena->pos;
            break;
        }
    }

    return result;
}

function void
ScratchEnd(Scratch scratch)
{
    ArenaPopTo(scratch.arena, scratch.pos);
}