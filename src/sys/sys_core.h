#ifndef SYS_CORE_H
#define SYS_CORE_H

struct Sys_Hnd
{
    u64 v[1];
};

typedef u32 Sys_Permission;
enum
{
    Sys_Permission_R      = (1<<0),
    Sys_Permission_W      = (1<<1),
    Sys_Permission_X      = (1<<2),
    Sys_Permission_Shared = (1<<4),
    Sys_Permission_Create = (1<<3),
};

typedef void Sys_ThreadFunction(void* params);

struct Sys_ProcCode
{
    i8 launch_err;
    i8 running;
    i8 kill_err;
    i8 read_err;
    i8 killed;
    u32 exitcode;
};

struct Sys_Shard
{
    Arena* arena;
    Sys_Hnd cv;
    Sys_Hnd mutex;
};

struct Sys_ShardTable
{
    u64 count;
    Sys_Shard* stripes;
};


function b32 Sys_HndMatch(Sys_Hnd a, Sys_Hnd b);

function u64 Sys_GetPageSize();

function Sys_ShardTable* Sys_ShardTableAlloc(Arena* arena, u64 count);
function void Sys_ShardTableRelease(Sys_ShardTable* table);

function void* Sys_ReserveMemory(u64 size);
function void  Sys_CommitMemory(void* ptr, u64 size);
function void  Sys_ReleaseMemory(void* ptr, u64 size);
function void  Sys_DecommitMemory(void* ptr, u64 size);
function void  Sys_ProtectMemory(void* ptr, u64 size, Sys_Permission flags);

function void Sys_SetThreadName(Str8 string);

function u64 Sys_TimeUS(); // Micro seconds

function Sys_Hnd Sys_MutexAlloc();
function void    Sys_MutexRelease(Sys_Hnd mutex);
function void    Sys_MutexBegin(Sys_Hnd mutex);
function void    Sys_MutexEnd(Sys_Hnd mutex);
#define Sys_Mutex(m) ScopedContextBlock(Sys_MutexBegin(m), Sys_MutexEnd(m))

function Sys_Hnd    Sys_CVAlloc();
function void       Sys_CVRelease(Sys_Hnd handle);
function void       Sys_CVSignal(Sys_Hnd handle);
function void       Sys_CVSignalAll(Sys_Hnd handle);

function void Sys_Initialise();

#endif
