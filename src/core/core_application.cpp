function void
Main(void (*entry)(), u64 argument_count, char **arguments)
{
    ThreadContext ctx = ThreadContextAlloc();
    {
        ctx.main_thread = 1;
        ThreadContextSet(&ctx);
    }

    // TODO: Handle args

    SetThreadName(StringLiteral("Main Thread"));  

    entry();

    ThreadContextRelease(&ctx);
}