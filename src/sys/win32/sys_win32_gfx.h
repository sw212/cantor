#ifndef SYS_WIN32_GFX_H
#define SYS_WIN32_GFX_H

typedef UINT Sys_Win32_GetDpiForWindowType(HWND hwnd);
typedef BOOL Sys_Win32_SetProcessDpiAwarenessContext(void* value);

struct Sys_Win32_Window
{
    Sys_Win32_Window* next;
    Sys_Win32_Window* prev;
    HWND hWnd;
    HDC  hdc;

    Arena* paint_arena;
};

struct Sys_Win32_GfxState
{
    Arena* arena;

    HWND global_hWnd;
    HDC  global_hdc;

    Sys_CursorType cursor;
    f32 refresh_rate;

    Arena*  window_arena;
    SRWLOCK window_srw;
    Sys_Win32_Window* first_window;
    Sys_Win32_Window* last_window;
    Sys_Win32_Window* free_window;
};


// - Window Utils
function Sys_Hnd           Sys_Win32_GetHandle(Sys_Win32_Window* window);
function Sys_Win32_Window* Sys_Win32_GetWindow(Sys_Hnd handle);
function Sys_KeyModifier   Sys_Win32_GetKeyModifiers();

// - Window Proc
function LRESULT Sys_Win32_WindowProc(HWND hWnd, UINT message, WPARAM w_param, LPARAM l_param);

#endif