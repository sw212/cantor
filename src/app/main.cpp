//
#include "core.h"
#include "sys.h"
#include "hash.h"
#include "render.h"
#include "font.h"
#include "font_cache.h"
#include "draw.h"
#include "ui.h"

//
#include "core.cpp"
#include "sys.cpp"
#include "hash.cpp"
#include "render.cpp"
#include "font.cpp"
#include "font_cache.cpp"
#include "draw.cpp"
#include "ui.cpp"

//
#include "sys_entry.cpp"
#include <stdio.h>

function void
EntryPoint()
{
    Sys_Initialise();
    Sys_InitialiseGfx();
    Render_Initialise();
    Hash_Initialise();
    Font_Core_Initialize();
    Font_Initialise(Vec2_i64{1024, 1024});
    Draw_Initialise();

    UI_Ctx* ui = UI_Initialise();

    Vec2_i32      window_dims  = {1280, 720};
    Str8          window_title = StringLiteral("Application");

    Sys_Hnd    window_handle        = Sys_InitWindow(window_dims, window_title);
    Render_Hnd window_render_handle = Render_Attach(window_handle);

    Sys_InitialPaint(window_handle);

    b32 should_quit = 0;
    while (!should_quit)
    {
        Scratch scratch = ScratchBegin(0, 0);

        Sys_EventList events = Sys_GetEvents(scratch.arena);

        {
            Rect2_f32 client_rect = Sys_GetClientRect(window_handle);
            Vec2_i64  resolution  = {(i64)fabsf(client_rect.max.x - client_rect.min.x), (i64)fabsf(client_rect.max.y - client_rect.min.y)};

            UI_SetState(ui);
            Render_WindowBegin(window_render_handle, resolution);
            Draw_BeginFrame();

            Draw_Context* draw_context = Draw_MakeContext(scratch.arena);
            Draw_PushContext(draw_context);
            {
                // UI
                {
                    UI_Begin(window_handle, &events);
                    {
                        UI_SetFontSize(9.f);

                        UI_DesiredHeight(UI_Pixels(100.f, 1.f))
                        UI_DesiredWidth(UI_Pixels(200.f, 1.f))
                        {
                            UI_SetColorBackground({.1f, .1f, .1f, 1.f});
                            UI_SetTextAlignment(UI_Align_Center);
                            {
                                UI_Button("Text_1");
                            }
                            UI_PopTextAlignment();
                            UI_PopColorBackground();

                            UI_Space(UI_Pixels(10.f, 0.f));

                            if (UI_Button("Button_1").clicked)
                            {
                                UI_Text("Clicked");
                            }
                        }
                    }
                    UI_End();
                    UI_ResolveWigDims();
                }

                // Draw
                {
                    Draw_RectParams p = {.color = {.5f,.1f,.1f,1.f}};
                    Draw_Rect(client_rect, &p);

                    UI_Draw();
                }
            }
            Draw_PopContext();

            Draw_Submit(window_render_handle, draw_context);
            Render_WindowEnd(window_render_handle);
        }

        for (Sys_Event* event = events.first; (event); event = event->next)
        {
            if (event->type == Sys_Event_WndClose)
            {
                should_quit = 1;
                break;
            }
        }

        ScratchEnd(scratch);
    }
}