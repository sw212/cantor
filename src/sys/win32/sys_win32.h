#ifndef SYS_WIN32_H
#define SYS_WIN32_H

#pragma push_macro("function")
#pragma warning(push, 0)
#undef function
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <timeapi.h>
#include <tlhelp32.h>
#include <Shlobj.h>
#pragma warning(pop)
#pragma pop_macro("function")

struct Sys_Win32_Proc
{
    Sys_Win32_Proc*      next;
    HANDLE               parent_hnd;
    PROCESS_INFORMATION  procinfo;
    Sys_ProcCode         proccode;
};

struct Sys_Win32_Thread
{
    Sys_Win32_Thread*   next;
    HANDLE              hnd;
    DWORD               tid;

    Sys_ThreadFunction* fn;
    void*               params;
};

struct Sys_Win32_CritSection
{
    Sys_Win32_CritSection* next;
    CRITICAL_SECTION       base;
};

struct Sys_Win32_SRW
{
    Sys_Win32_SRW* next;
    SRWLOCK        lock;
};

struct Sys_Win32_CV
{
    Sys_Win32_CV*      next;
    CONDITION_VARIABLE cv;
};

struct Sys_Win32_State
{
    Arena* arena;

    SYSTEM_INFO system_info;
    LARGE_INTEGER perf_freq;
    b32  timer_high_res;
    
    Str8 exe_path;
    Str8 data_path;

    // for allocating SRW locks
    SRWLOCK SRW_lock;
    Arena*  SRW_arena;
    Sys_Win32_SRW* free_SRW;

    SRWLOCK thread_SRW;
    Arena*  thread_arena;
    Sys_Win32_Thread* free_thread;

    SRWLOCK proc_SRW;
    Arena*  proc_arena;
    Sys_Win32_Proc* free_proc;

    SRWLOCK crit_section_SRW;
    Arena*  crit_section_arena;
    Sys_Win32_CritSection* free_crit_section;

    SRWLOCK cv_SRW;
    Arena*  cv_arena;
    Sys_Win32_CV* free_cv;
};


function Sys_Win32_Proc* Sys_Win32_ProcAlloc();
function void Sys_Win32_ProcRelease(Sys_Win32_Proc* process);

function Sys_Win32_Thread* Sys_Win32_ThreadAlloc();
function void Sys_Win32_ThreadRelease(Sys_Win32_Thread* thread);

function Sys_Win32_CritSection* Sys_Win32_CritSectionAlloc();
function void Sys_Win32_CritSectionRelease(Sys_Win32_CritSection* critical_section);

function Sys_Win32_SRW* Sys_Win32_SRWAlloc();
function void Sys_Win32_SRWRelease(Sys_Win32_SRW* srw);

function Sys_Win32_CV* Sys_Win32_CVAlloc();
function void Sys_Win32_CVRelease(Sys_Win32_CV* cv);

#endif