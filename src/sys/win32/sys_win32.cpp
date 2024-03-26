#pragma comment(lib, "user32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "shell32")

// - Globals
HINSTANCE        sys_win32_hinstance = 0;
Sys_Win32_State* sys_win32_state = 0;


function Sys_Win32_Proc*
Sys_Win32_ProcAlloc()
{
    Sys_Win32_Proc* result = 0;

    AcquireSRWLockExclusive(&sys_win32_state->proc_SRW);
    {
        result = sys_win32_state->free_proc;

        if (result)
        {
            StackPop(sys_win32_state->free_proc);
        }
        else
        {
            result = PushArrayNZ(sys_win32_state->proc_arena, Sys_Win32_Proc, 1);
        }

        MemoryZero(result, sizeof(*result));
    }
    ReleaseSRWLockExclusive(&sys_win32_state->proc_SRW);

    return result;
}

function void
Sys_Win32_ProcRelease(Sys_Win32_Proc* process)
{
    AcquireSRWLockExclusive(&sys_win32_state->proc_SRW);
    {
        StackPush(sys_win32_state->free_proc, process);
    }
    ReleaseSRWLockExclusive(&sys_win32_state->proc_SRW);
}

function Sys_Win32_Thread*
Sys_Win32_ThreadAlloc()
{
    Sys_Win32_Thread* result = 0;

    AcquireSRWLockExclusive(&sys_win32_state->thread_SRW);
    {
        result = sys_win32_state->free_thread;

        if (result)
        {
            StackPop(sys_win32_state->free_thread);
        }
        else
        {
            result = PushArrayNZ(sys_win32_state->thread_arena, Sys_Win32_Thread, 1);
        }

        MemoryZero(result, sizeof(*result));
    }
    ReleaseSRWLockExclusive(&sys_win32_state->thread_SRW);

    return result;
}

function void
Sys_Win32_ThreadRelease(Sys_Win32_Thread* thread)
{
    AcquireSRWLockExclusive(&sys_win32_state->thread_SRW);
    {
        StackPush(sys_win32_state->free_thread, thread);
    }
    ReleaseSRWLockExclusive(&sys_win32_state->thread_SRW);
}

function Sys_Win32_CritSection*
Sys_Win32_CritSectionAlloc()
{
    Sys_Win32_CritSection* result = 0;

    AcquireSRWLockExclusive(&sys_win32_state->crit_section_SRW);
    {
        result = sys_win32_state->free_crit_section;

        if (result)
        {
            StackPop(sys_win32_state->free_crit_section);
        }
        else
        {
            result = PushArrayNZ(sys_win32_state->crit_section_arena, Sys_Win32_CritSection, 1);
        }

        MemoryZero(result, sizeof(*result));
    }
    ReleaseSRWLockExclusive(&sys_win32_state->crit_section_SRW);

    return result;
}
function void
Sys_Win32_CritSectionRelease(Sys_Win32_CritSection* critical_section)
{
    AcquireSRWLockExclusive(&sys_win32_state->crit_section_SRW);
    {
        StackPush(sys_win32_state->free_crit_section, critical_section);
    }
    ReleaseSRWLockExclusive(&sys_win32_state->crit_section_SRW);
}

function Sys_Win32_SRW*
Sys_Win32_SRWAlloc()
{
    Sys_Win32_SRW* result = 0;

    AcquireSRWLockExclusive(&sys_win32_state->SRW_lock);
    {
        result = sys_win32_state->free_SRW;

        if (result)
        {
            StackPop(sys_win32_state->free_SRW);
        }
        else
        {
            result = PushArrayNZ(sys_win32_state->SRW_arena, Sys_Win32_SRW, 1);
        }

        MemoryZero(result, sizeof(*result));
    }
    ReleaseSRWLockExclusive(&sys_win32_state->SRW_lock);

    return result;
}

function void
Sys_Win32_SRWRelease(Sys_Win32_SRW* srw)
{
    AcquireSRWLockExclusive(&sys_win32_state->SRW_lock);
    {
        StackPush(sys_win32_state->free_SRW, srw);
    }
    ReleaseSRWLockExclusive(&sys_win32_state->SRW_lock);
}

function Sys_Win32_CV*
Sys_Win32_CVAlloc()
{
    Sys_Win32_CV* result = 0;

    AcquireSRWLockExclusive(&sys_win32_state->cv_SRW);
    {
        result = sys_win32_state->free_cv;

        if (result)
        {
            StackPop(sys_win32_state->free_cv);
        }
        else
        {
            result = PushArrayNZ(sys_win32_state->cv_arena, Sys_Win32_CV, 1);
        }

        MemoryZero(result, sizeof(*result));
    }
    ReleaseSRWLockExclusive(&sys_win32_state->cv_SRW);

    return result;
}

function void
Sys_Win32_CVRelease(Sys_Win32_CV* condition_variable)
{
    AcquireSRWLockExclusive(&sys_win32_state->cv_SRW);
    {
        StackPush(sys_win32_state->free_cv, condition_variable);
    }
    ReleaseSRWLockExclusive(&sys_win32_state->cv_SRW);
}


// - Info

function u64
Sys_GetPageSize()
{
    SYSTEM_INFO info = {};
    GetSystemInfo(&info);

    return info.dwPageSize;
}


// - Memory

function void*
Sys_ReserveMemory(u64 size)
{
    u64 reserved_size = ((size + Gigabytes(1) - 1) / Gigabytes(1)) * Gigabytes(1);
    void* result = VirtualAlloc(0, reserved_size, MEM_RESERVE, PAGE_NOACCESS);

    return result;
}

function void
Sys_CommitMemory(void *ptr, u64 size)
{
    u64   page_size = Sys_GetPageSize();   
    u64 commit_size = ((size + page_size - 1) / page_size) * page_size;
    VirtualAlloc(ptr, commit_size, MEM_COMMIT, PAGE_READWRITE);
}

function void
Sys_ReleaseMemory(void* ptr, u64 size)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

function void
Sys_DecommitMemory(void *ptr, u64 size)
{
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

function void
Sys_Protect(void* ptr, u64 size, Sys_Permission flags)
{
    u64 page_size_rounded = ((size + Sys_GetPageSize() - 1) / Sys_GetPageSize()) * Sys_GetPageSize();

    DWORD permissions = 0;
    {
        switch(flags)
        {
            case PAGE_EXECUTE:           { permissions = Sys_Permission_X;                                   } break;
            case PAGE_EXECUTE_READ:      { permissions = Sys_Permission_X|Sys_Permission_R;                  } break;
            case PAGE_EXECUTE_READWRITE: { permissions = Sys_Permission_X|Sys_Permission_R|Sys_Permission_W; } break;
            case PAGE_EXECUTE_WRITECOPY: { permissions = Sys_Permission_X|Sys_Permission_W;                  } break;
            case PAGE_READONLY:          { permissions = Sys_Permission_R;                                   } break;
            case PAGE_READWRITE:         { permissions = Sys_Permission_R|Sys_Permission_W;                  } break;

            default:
            {
                permissions = PAGE_NOACCESS;
            } break;
        }
    }

    // protect
    DWORD old_flags = 0;
    VirtualProtect(ptr, page_size_rounded, permissions, &old_flags);
}



// - Threads

function void
Sys_SetThreadName(Str8 string)
{
    Scratch scratch = ScratchBegin(0, 0);

    Str16 name = String16(scratch.arena, string);
    HRESULT hr = SetThreadDescription(GetCurrentThread(), (WCHAR*)name.str);

    ScratchEnd(scratch);
}


// - Mutexes

function Sys_Hnd
Sys_MutexAlloc()
{
    Sys_Win32_CritSection* critical_section = Sys_Win32_CritSectionAlloc();
    InitializeCriticalSection(&critical_section->base);

    Sys_Hnd result = {u64(critical_section)};
    return result;
}

function void
Sys_MutexRelease(Sys_Hnd mutex)
{
    Sys_Win32_CritSection* critical_section = (Sys_Win32_CritSection*)mutex.v[0];
    DeleteCriticalSection(&critical_section->base);
    Sys_Win32_CritSectionRelease(critical_section);
}

function void
Sys_MutexBegin(Sys_Hnd mutex)
{
    Sys_Win32_CritSection* critical_section = (Sys_Win32_CritSection*)mutex.v[0];
    EnterCriticalSection(&critical_section->base);
}

function void
Sys_MutexEnd(Sys_Hnd mutex)
{
    Sys_Win32_CritSection* critical_section = (Sys_Win32_CritSection*)mutex.v[0];
    LeaveCriticalSection(&critical_section->base);
}


// - CV

function Sys_Hnd
Sys_CVAlloc()
{
    Sys_Win32_CV* cv = Sys_Win32_CVAlloc();
    Sys_Hnd result = {u64(cv)};

    return result;
}

function void
Sys_CVRelease(Sys_Hnd handle)
{
    Sys_Win32_CV* cv = (Sys_Win32_CV*)handle.v;
    Sys_Win32_CVRelease(cv);
}

function void
Sys_CVSignal(Sys_Hnd handle)
{
    Sys_Win32_CV* cv = (Sys_Win32_CV*)handle.v[0];
    WakeConditionVariable(&cv->cv);
}

function void
Sys_CVSignalAll(Sys_Hnd handle)
{
    Sys_Win32_CV* cv = (Sys_Win32_CV*)handle.v[0];
    WakeAllConditionVariable(&cv->cv);
}


// - Threads


// - Time

function u64
Sys_TimeUS()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    
    f64      seconds = f64(time.QuadPart) / f64(sys_win32_state->perf_freq.QuadPart);
    u64 microseconds = u64(seconds * Million(1));

    return microseconds;
}


// - Init

function void
Sys_Initialise()
{
    if (IsCurrentThreadMain() && (!sys_win32_state))
    {
        Arena* arena = ArenaAlloc(Gigabytes(1));
        sys_win32_state = PushArray(arena, Sys_Win32_State, 1);
        sys_win32_state->arena = arena;

        GetSystemInfo(&sys_win32_state->system_info);
        QueryPerformanceFrequency(&sys_win32_state->perf_freq);

        // Locks & Arenas
        {
            InitializeSRWLock(&sys_win32_state->proc_SRW);
            InitializeSRWLock(&sys_win32_state->thread_SRW);
            InitializeSRWLock(&sys_win32_state->crit_section_SRW);
            InitializeSRWLock(&sys_win32_state->SRW_lock);
            InitializeSRWLock(&sys_win32_state->cv_SRW);
            sys_win32_state->proc_arena         = ArenaAlloc(Kilobytes(256));
            sys_win32_state->thread_arena       = ArenaAlloc(Kilobytes(256));
            sys_win32_state->crit_section_arena = ArenaAlloc(Kilobytes(256));
            sys_win32_state->SRW_arena          = ArenaAlloc(Kilobytes(256));
            sys_win32_state->cv_arena           = ArenaAlloc(Kilobytes(256)); 
        }

        // Path Info
        Scratch scratch = ScratchBegin(0, 0);
        {
            Str8 exe_path = {};
            {
                u64     size = Kilobytes(32);
                u16*  buffer = PushArray(arena, u16, size);
                DWORD length = GetModuleFileNameW(0, (WCHAR*)buffer, (DWORD)size);
                exe_path = String8(scratch.arena, String16(buffer, length));
                exe_path = StrTrimToLastSlash(exe_path);
            }

            Str8 data_path = {};
            {
                u64    size = Kilobytes(32);
                u16* buffer = PushArray(arena, u16, size);
                if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR*)buffer)))
                {
                    data_path = String8(scratch.arena, String16(buffer));
                }
            }

            sys_win32_state->exe_path   = StrPushCopy(arena, exe_path);
            sys_win32_state->data_path = StrPushCopy(arena, data_path);
        }
        ScratchEnd(scratch);
    
        sys_win32_state->timer_high_res = (timeBeginPeriod(1) == TIMERR_NOERROR);
    }
}
