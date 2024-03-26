#ifndef SYS_GFX_H
#define SYS_GFX_H

#include "sys_key.h"

enum Sys_EventType
{
    Sys_Event_NOP,
    Sys_Event_Press,
    Sys_Event_Release,
    Sys_Event_Key,
    Sys_Event_WndClose,
    Sys_Event_WndBlur,
    Sys_EventType_Count
};

typedef u32 Sys_KeyModifier;
enum
{
    Sys_KeyModifier_Alt   = (1<<0),
    Sys_KeyModifier_Shift = (1<<1),
    Sys_KeyModifier_Ctrl  = (1<<2),
};

struct Sys_Event
{
    Sys_Event* next;
    Sys_Event* prev;
    Sys_Hnd window;

    Sys_EventType   type;
    Sys_KeyModifier modifiers;

    Sys_Key  key;
    u32      character;

    Vec2_f32 position;
    Vec2_f32 scroll;

    Str8     path;
};

struct Sys_EventList
{
    Sys_Event* first;
    Sys_Event* last;
    u64 count;
};

enum Sys_CursorType
{
    Sys_Cursor_Null,
    Sys_Cursor_Pointer,
    Sys_Cursor_Hidden,
    Sys_CursorType_Count,
};


function Vec2_f32  Sys_GetMouse(Sys_Hnd handle);
function Rect2_f32 Sys_GetClientRect(Sys_Hnd handle);

function b32       Sys_WindowFocused(Sys_Hnd handle);

function Sys_KeyModifier Sys_GetModifiers();
function Sys_EventList   Sys_GetEvents(Arena* arena);
function void            Sys_ConsumeEvent(Sys_EventList* events, Sys_Event* event);

function b32             Sys_ConsumeKeyPress(Sys_EventList* events, Sys_Hnd window, Sys_Key key, Sys_KeyModifier mods);
function b32             Sys_ConsumeKeyRelease(Sys_EventList* events, Sys_Hnd window, Sys_Key key, Sys_KeyModifier mods);

function Sys_Hnd    Sys_InitWindow(Vec2_i32 size, Str8 title);
function void       Sys_InitialPaint(Sys_Hnd handle);

function void Sys_InitialiseGfx();


#endif