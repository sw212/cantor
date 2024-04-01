// - Globals

LPCWSTR             Sys_Win32_WindowClassName = L"WindowClassName";
Sys_Win32_GfxState* sys_win32_gfx_state = 0;

per_thread         Arena* sys_win32_tl_events_arena;
per_thread Sys_EventList* sys_win32_tl_events_list;


// - Window Utils

function Sys_Hnd
Sys_Win32_GetHandle(Sys_Win32_Window* window)
{
    Sys_Hnd result = {};
    result.v[0] = (u64)window;

    return result;
}

function Sys_Win32_Window*
Sys_Win32_GetWindow(Sys_Hnd handle)
{
    Sys_Win32_Window* window = (Sys_Win32_Window*)handle.v[0];
    return window;
}

function Sys_KeyModifier
Sys_Win32_GetKeyModifiers()
{
    Sys_KeyModifier result = {};

    if (GetKeyState(VK_CONTROL) & 0x8000) { result |= Sys_KeyModifier_Ctrl;  }
    if (GetKeyState(VK_SHIFT)   & 0x8000) { result |= Sys_KeyModifier_Shift; }
    if (GetKeyState(VK_MENU)    & 0x8000) { result |= Sys_KeyModifier_Alt;   }

    return result;
}

function Vec2_f32
Sys_GetMouse(Sys_Hnd handle)
{
    Vec2_f32 result = {-100.f, -100.f};
    Sys_Win32_Window* window = Sys_Win32_GetWindow(handle);

    if (window)
    {
        POINT point = {};

        // position of the mouse cursor, in screen coordinates
        if (GetCursorPos(&point))
        {
            // maps screen coordinates -> client-area coordinates
            if (ScreenToClient(window->hWnd, &point))
            {
                result.x = f32(point.x);
                result.y = f32(point.y);
            }
        }
    }

    return result;
}

function Rect2_f32
Sys_GetClientRect(Sys_Hnd handle)
{
    Rect2_f32 result = {};
    Sys_Win32_Window* window = Sys_Win32_GetWindow(handle);

    if (window)
    {
        RECT rect = {};
        // client coordinates i.e. relative to tl corner of window's client area
        if (GetClientRect(window->hWnd, &rect))
        {
            result.min = {f32(rect.left), f32(rect.top)};
            result.max = {f32(rect.right), f32(rect.bottom)};
        }
    }

    return result;
}

function b32
Sys_WindowFocused(Sys_Hnd handle)
{
    b32 result = 0;
    Sys_Win32_Window* window = Sys_Win32_GetWindow(handle);

    if (window)
    {
        result = GetForegroundWindow() == window->hWnd;
    }

    return result;
}


// - Window Events

function Sys_KeyModifier
Sys_GetModifiers()
{
    Sys_KeyModifier result = Sys_Win32_GetKeyModifiers();
    return result;
}

function Sys_EventList
Sys_GetEvents(Arena* arena)
{
    Sys_EventList result = {};

    sys_win32_tl_events_arena = arena;
    sys_win32_tl_events_list = &result;

    MSG message = {};
    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    sys_win32_tl_events_arena = 0;
    sys_win32_tl_events_list  = 0;

    return result;
}

function void
Sys_ConsumeEvent(Sys_EventList* events, Sys_Event* event)
{
    DLLRemove(events->first, events->last, event);
    events->count--;
    event->type = Sys_Event_NOP;
}

function b32
Sys_ConsumeKeyPress(Sys_EventList *events, Sys_Hnd window, Sys_Key key, Sys_KeyModifier mods)
{
    b32 result = 0;
    for (Sys_Event* event = events->first; event; event=event->next)
    {
        if (event->type == Sys_Event_Press &&
            Sys_HndMatch(window, event->window) &&
            event->key == key &&
            ((event->modifiers & mods) || (!event->modifiers && !mods)))
        {
            Sys_ConsumeEvent(events, event);
            result = 1;
            break;
        }
    }

    return result;
}

function b32
Sys_ConsumeKeyRelease(Sys_EventList *events, Sys_Hnd window, Sys_Key key, Sys_KeyModifier mods)
{
    b32 result = 0;
    for (Sys_Event* event = events->first; event; event=event->next)
    {
        if (event->type == Sys_Event_Release &&
            Sys_HndMatch(window, event->window) &&
            event->key == key &&
            ((event->modifiers & mods) || (!event->modifiers && !mods)))
        {
            Sys_ConsumeEvent(events, event);
            result = 1;
            break;
        }
    }

    return result;
}


// - Window Procedures

function Sys_Hnd
Sys_InitWindow(Vec2_i32 size, Str8 title)
{
    Sys_Hnd handle = {};

    AcquireSRWLockExclusive(&sys_win32_gfx_state->window_srw);
    {
        Sys_Win32_Window* window = sys_win32_gfx_state->free_window;

        // - Acquire New Window
        {
            if (window)
            {
                StackPop(sys_win32_gfx_state->free_window);
            }
            else
            {
                window = PushArrayNZ(sys_win32_gfx_state->window_arena, Sys_Win32_Window, 1);
            }

            MemoryZero(window, sizeof(*window));
            DLLPushBack(sys_win32_gfx_state->first_window, sys_win32_gfx_state->last_window, window);
        }

        // - Open Window
        {
            Scratch scratch = ScratchBegin(0, 0);
            Str16   window_title = String16(scratch.arena, title);

            DWORD     dwExStyle    = 0L;
            LPCWSTR   lpClassName  = Sys_Win32_WindowClassName;
            LPCWSTR   lpWindowName = (LPCWSTR)window_title.str;
            DWORD     dwStyle      = WS_OVERLAPPEDWINDOW;
            int       X            = CW_USEDEFAULT;
            int       Y            = CW_USEDEFAULT;
            int       nWidth       = size.x;
            int       nHeight      = size.y;
            HWND      hWndParent   = 0;
            HMENU     hMenu        = 0;
            HINSTANCE hInstance    = sys_win32_hinstance;
            LPVOID    lpParam      = 0;
            
            window->hWnd = CreateWindowW(
                lpClassName, lpWindowName,
                dwStyle,
                X, Y, nWidth, nHeight,
                hWndParent, hMenu, hInstance,
                lpParam
            );

            window->hdc = GetDC(window->hWnd);

            SetWindowLongPtr(window->hWnd, GWLP_USERDATA, (LONG_PTR)window);

            ScratchEnd(scratch);
        }

        handle = Sys_Win32_GetHandle(window);
    }
    ReleaseSRWLockExclusive(&sys_win32_gfx_state->window_srw);

    return handle;
}

function void
Sys_InitialPaint(Sys_Hnd handle)
{
    Scratch       scratch = ScratchBegin(0, 0);
    Sys_EventList events  = {};

    sys_win32_tl_events_list  = &events;
    sys_win32_tl_events_arena = scratch.arena;

    Sys_Win32_Window* window = Sys_Win32_GetWindow(handle);

    ShowWindow(window->hWnd, SW_SHOW);
    UpdateWindow(window->hWnd);

    sys_win32_tl_events_arena = 0;
    sys_win32_tl_events_list  = 0;

    ScratchEnd(scratch);
}


// - Init

function void
Sys_InitialiseGfx()
{
    if (IsCurrentThreadMain() && (!sys_win32_gfx_state))
    {
        // - Init Gfx State
        {
            Arena* arena = ArenaAlloc(Gigabytes(1));
            sys_win32_gfx_state = PushArray(arena, Sys_Win32_GfxState, 1);
            sys_win32_gfx_state->arena = arena;
            sys_win32_gfx_state->window_arena = ArenaAlloc(Gigabytes(1));
            InitializeSRWLock(&sys_win32_gfx_state->window_srw);
        }

        // - DPI Awareness
        {
            Sys_Win32_SetProcessDpiAwarenessContext* SetProcessDpiAwarenessContext = 0;
            HMODULE user32 = LoadLibraryA("user32.dll");

            if (user32)
            {
                SetProcessDpiAwarenessContext = (Sys_Win32_SetProcessDpiAwarenessContext*)(void*)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
                FreeLibrary(user32);
            }

            if (SetProcessDpiAwarenessContext)
            {
                SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            }
        }

        // - Register Window Class
        WNDCLASSW window_class = {};
        {
            window_class.style         = CS_HREDRAW | CS_VREDRAW;
            window_class.lpfnWndProc   = Sys_Win32_WindowProc;
            window_class.hInstance     = sys_win32_hinstance;
            window_class.lpszClassName = Sys_Win32_WindowClassName;
            window_class.hCursor       = LoadCursor(0, IDC_ARROW);
        }
        RegisterClassW(&window_class);

        // - Setup Global Window
        {
            DWORD     dwExStyle    = 0L;
            LPCWSTR   lpClassName  = Sys_Win32_WindowClassName;
            LPCWSTR   lpWindowName = L"";
            DWORD     dwStyle      = WS_OVERLAPPEDWINDOW;
            int       X            = CW_USEDEFAULT;
            int       Y            = CW_USEDEFAULT;
            int       nWidth       = 100;
            int       nHeight      = 100;
            HWND      hWndParent   = 0;
            HMENU     hMenu        = 0;
            HINSTANCE hInstance    = sys_win32_hinstance;
            LPVOID    lpParam      = 0;
            
            sys_win32_gfx_state->global_hWnd = CreateWindowW(
                lpClassName, lpWindowName,
                dwStyle,
                X, Y, nWidth, nHeight,
                hWndParent, hMenu, hInstance,
                lpParam
            );

            sys_win32_gfx_state->global_hdc = GetDC(sys_win32_gfx_state->global_hWnd);
        }

        // - Refresh Rate
        {
            DEVMODEA device_mode = {};
            if (EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &device_mode))
            {
                sys_win32_gfx_state->refresh_rate = (f32)device_mode.dmDisplayFrequency;
            }
            else
            {
                sys_win32_gfx_state->refresh_rate = 60.f;
            }
        }
    }
}


// - Window Proc

function LRESULT
Sys_Win32_WindowProc(HWND hWnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT   result = 0;
    Sys_Event* event = 0;

    Sys_Win32_Window* window = (Sys_Win32_Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    Sys_Hnd window_handle = Sys_Win32_GetHandle(window);

    Scratch scratch = ScratchBegin(&sys_win32_tl_events_arena, 1);
    Sys_EventList fallback_event_list = {};

    if (!sys_win32_tl_events_arena)
    {
        sys_win32_tl_events_arena = scratch.arena;
        sys_win32_tl_events_list  = &fallback_event_list;
    }

    b32 is_release = 0;

    switch (message)
    {
        case WM_CLOSE:
        {
            event = PushArray(sys_win32_tl_events_arena, Sys_Event, 1);
            event->type = Sys_Event_WndClose;
            event->window = window_handle;
        } break;

        case WM_KILLFOCUS:
        {
            event = PushArray(sys_win32_tl_events_arena, Sys_Event, 1);
            event->type = Sys_Event_WndBlur;
            event->window = window_handle;
            ReleaseCapture();
        } break;

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            ReleaseCapture();
            is_release = 1;
        }
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if (!is_release)
            {
                SetCapture(hWnd);
            }

            Sys_EventType kind = is_release ? Sys_Event_Release : Sys_Event_Press;
            Sys_Key key = Sys_Key_MouseLeft;

            switch(message)
            {
                case WM_MBUTTONDOWN: case WM_MBUTTONUP: { key = Sys_Key_MouseMiddle; } break;
                case WM_RBUTTONDOWN: case WM_RBUTTONUP: { key = Sys_Key_MouseRight;  } break;
            }

            event = PushArray(sys_win32_tl_events_arena, Sys_Event, 1);
            event->type = kind;
            event->window = window_handle;
            event->key = key;
            event->position = Sys_GetMouse(window_handle);
        } break;

        case WM_SETCURSOR:
        {
            Rect2_f32 rect = Sys_GetClientRect(window_handle);
            Vec2_f32 point = Sys_GetMouse(window_handle); 
            b32 contains = Contains(rect, point);

            Sys_CursorType cursor = sys_win32_gfx_state->cursor;

            if (contains && cursor != Sys_Cursor_Null)
            {
                static b32 init;
                static HCURSOR cursors[Sys_CursorType_Count];

                if (!init)
                {
                    init = 1;
                    cursors[Sys_Cursor_Pointer] = LoadCursorA(0, IDC_ARROW);
                }

                if (cursor == Sys_Cursor_Hidden) 
                {
                    ShowCursor(0);
                }
                else
                {
                    ShowCursor(1);
                    SetCursor(cursors[cursor]);
                }
            }
            else
            {
                result = DefWindowProcW(hWnd, message, w_param, l_param);
            }
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            DefWindowProcW(hWnd, message, w_param, l_param);
        };
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            b32 is_down  =  !(l_param & (1 << 31));
            b32 was_down = !!(l_param & (1 << 30));

            Sys_EventType kind = is_down ? Sys_Event_Press : Sys_Event_Release;

            static Sys_Key keys[256];
            static b32 init;

            if (!init)
            {
                init = 1;

                for (u32 i = 'A',   j = Sys_Key_A ; i <= 'Z'   ; i++, j++) { keys[i] = Sys_Key(j); }
                for (u32 i = '0',   j = Sys_Key_0 ; i <= '9'   ; i++, j++) { keys[i] = Sys_Key(j); }
                for (u32 i = VK_F1, j = Sys_Key_F1; i <= VK_F24; i++, j++) { keys[i] = Sys_Key(j); }
                
                keys[VK_ESCAPE]     = Sys_Key_Esc;
                keys[VK_OEM_3]      = Sys_Key_GraveAccent;
                keys[VK_OEM_MINUS]  = Sys_Key_Minus;
                keys[VK_OEM_PLUS]   = Sys_Key_Equal;
                keys[VK_BACK]       = Sys_Key_Backspace;
                keys[VK_TAB]        = Sys_Key_Tab;
                keys[VK_SPACE]      = Sys_Key_Space;
                keys[VK_RETURN]     = Sys_Key_Enter;
                keys[VK_CONTROL]    = Sys_Key_Ctrl;
                keys[VK_SHIFT]      = Sys_Key_Shift;
                keys[VK_MENU]       = Sys_Key_Alt;
                keys[VK_UP]         = Sys_Key_Up;
                keys[VK_LEFT]       = Sys_Key_Left;
                keys[VK_DOWN]       = Sys_Key_Down;
                keys[VK_RIGHT]      = Sys_Key_Right;
                keys[VK_DELETE]     = Sys_Key_Delete;
                keys[VK_PRIOR]      = Sys_Key_PageUp;
                keys[VK_NEXT]       = Sys_Key_PageDown;
                keys[VK_HOME]       = Sys_Key_Home;
                keys[VK_END]        = Sys_Key_End;
                keys[VK_OEM_2]      = Sys_Key_ForwardSlash;
                keys[VK_OEM_PERIOD] = Sys_Key_Period;
                keys[VK_OEM_COMMA]  = Sys_Key_Comma;
                keys[VK_OEM_7]      = Sys_Key_Quote;
                keys[VK_OEM_4]      = Sys_Key_LeftBracket;
                keys[VK_OEM_6]      = Sys_Key_RightBracket;
                keys[VK_INSERT]     = Sys_Key_Insert;
                keys[VK_OEM_1]      = Sys_Key_Semicolon;
            }

            Sys_Key key = Sys_Key_Null;
            if (w_param < ArrayCount(keys))
            {
                key = keys[w_param];
            }

            event = PushArray(sys_win32_tl_events_arena, Sys_Event, 1);
            event->type = kind;
            event->window = window_handle;
            event->key = key;
        } break;

        case WM_SYSCOMMAND:
        {
            switch (w_param)
            {
                case SC_CLOSE:
                {
                    event = PushArray(sys_win32_tl_events_arena, Sys_Event, 1);
                    event->type = Sys_Event_WndClose;
                    event->window = window_handle;
                } break;

                case SC_KEYMENU:
                {
                } break;

                default:
                {
                    result = DefWindowProcW(hWnd, message, w_param, l_param);
                } break;
            }
        } break;

        case WM_CHAR:
        case WM_SYSCHAR:
        {
            u32 c = u32(w_param);

            if (c == '\r')
            {
                c = '\n';
            }

            if((c >= 32 && c != 127) || (c == '\t') || (c == '\n'))
            {
                event = PushArray(sys_win32_tl_events_arena, Sys_Event, 1);
                event->type = Sys_Event_Key;
                event->window = window_handle;
                event->character = c;
            }
        } break;

        case WM_PAINT:
        {
            result = DefWindowProcW(hWnd, message, w_param, l_param);
        } break;

        case WM_NCCALCSIZE:
        {
            result = DefWindowProcW(hWnd, message, w_param, l_param);
        } break;

        case WM_NCHITTEST:
        {
            result = DefWindowProcW(hWnd, message, w_param, l_param);
        } break;

        default:
        {
            result = DefWindowProcW(hWnd, message, w_param, l_param);
        } break;
    }

    if (event)
    {
        event->modifiers = Sys_Win32_GetKeyModifiers();
        DLLPushBack(sys_win32_tl_events_list->first, sys_win32_tl_events_list->last, event);
        sys_win32_tl_events_list->count++;
    }

    ScratchEnd(scratch);

    return result;
}