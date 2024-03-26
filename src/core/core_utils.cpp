__declspec(thread) ThreadContext *tl_ctx = 0;

extern "C" void
ThreadContextSet(ThreadContext* ctx)
{
    tl_ctx = ctx;
}

extern "C" ThreadContext*
ThreadContextGet()
{
    return tl_ctx;
}

function ThreadContext
ThreadContextAlloc()
{
    ThreadContext ctx = {};

    for (u64 idx = 0; idx < ArrayCount(ctx.scratch); idx++)
    {
        ctx.scratch[idx] = ArenaAlloc(Gigabytes(8));
    }
    
    return ctx;
}

function void
ThreadContextRelease(ThreadContext* ctx)
{
    for (u64 idx = 0; idx < ArrayCount(ctx->scratch); idx++)
    {
        ArenaRelease(ctx->scratch[idx]);
    }
}

function void
SetThreadName(Str8 string)
{
    ThreadContext* ctx = ThreadContextGet();
    ctx->name_length = Min(string.size, sizeof(ctx->name));
    MemoryCopy(ctx->name, string.str, ctx->name_length);
    Sys_SetThreadName(string);
}

function b32
IsCurrentThreadMain()
{
    ThreadContext* ctx = ThreadContextGet();
    return ctx->main_thread;
}
