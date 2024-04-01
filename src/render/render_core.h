#ifndef RENDER_CORE_H
#define RENDER_CORE_H

function void Render_WindowBegin(Render_Hnd render_window, Vec2_i64 resolution);
function void Render_WindowSubmit(Render_Hnd render_window, Render_PassList* render_passes);
function void Render_WindowEnd(Render_Hnd render_window);

function void Render_FrameEnd();

function Render_Hnd Render_AllocTex2D(Vec2_i64 size, Render_Tex2DType format, Render_Tex2DUsageType kind, void* initial_data);
function void       Render_ReleaseTex2D(Render_Hnd texture);
function void       Render_FillRegionTex2D(Render_Hnd texture, Rect2_i64 region, void* data);

function Render_Hnd Render_Attach(Sys_Hnd window_handle);
function void       Render_Dettach(Sys_Hnd window_handle, Render_Hnd render_handle);

function void Render_Initialise();

#endif