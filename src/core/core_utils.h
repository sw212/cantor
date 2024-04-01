#ifndef CORE_UTILS_H
#define CORE_UTILS_H

struct ThreadContext
{
    Arena*   scratch[2];
    i8       name[128];
    i64      name_length;
    b32      main_thread;
};

extern "C" void           ThreadContextSet(ThreadContext* ctx);
extern "C" ThreadContext* ThreadContextGet();

function ThreadContext ThreadContextAlloc();
function void          ThreadContextRelease(ThreadContext* ctx);

function void SetThreadName(Str8 string);
function b32  IsCurrentThreadMain();




#endif