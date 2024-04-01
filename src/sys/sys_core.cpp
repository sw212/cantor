function b32
Sys_HndMatch(Sys_Hnd a, Sys_Hnd b)
{
    return a.v[0] == b.v[0];
}


// - Stripe

function Sys_ShardTable*
Sys_ShardTableAlloc(Arena* arena, u64 count)
{
    Sys_ShardTable* result = PushArray(arena, Sys_ShardTable, 1);
    result->count   = count;
    result->stripes = PushArray(arena, Sys_Shard, count);

    for (u64 i = 0; i < count; i++)
    {
        result->stripes[i].cv    = Sys_CVAlloc();
        result->stripes[i].mutex = Sys_MutexAlloc();
        result->stripes[i].arena = ArenaAlloc(Gigabytes(1));
    }

    return result;
}

function void
Sys_ShardTableRelease(Sys_ShardTable* table)
{
    for (u64 i = 0; i < table->count; i++)
    {
        Sys_CVRelease(table->stripes[i].cv);
        Sys_MutexRelease(table->stripes[i].mutex);
        ArenaRelease(table->stripes[i].arena);
    }
}