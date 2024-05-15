#ifndef DRAW_H
#define DRAW_H

struct Draw_State
{
    Arena* arena;
    Render_Hnd white_texture;
};

struct Draw_Sampler2D
{
    Draw_Sampler2D*  next;
    Render_Sampler2DType v;
};

struct Draw_XForm2D
{
    Draw_XForm2D* next;
    Mat3x3_f32 v;
};

struct Draw_Clip
{
    Draw_Clip* next;
    Rect2_f32 v;
};

struct Draw_Opacity
{
    Draw_Opacity* next;
    f32 v;
};

struct Draw_Context
{
    Render_PassList passes;
 
    u64 generation;
    u64 prev_generation;

    Draw_Clip*      top_clip;
    Draw_XForm2D*   top_xform2d;
    Draw_Sampler2D* top_sampler2d;
    Draw_Opacity*   top_opacity;
};

struct Draw_ContextNode
{
    Draw_ContextNode* next;
    Draw_Context* context;
};

struct Draw_ThreadContext
{
    Arena* arena;
    u64    start_of_frame_arena_pos; // for resetting transient frame allocations
    Draw_ContextNode* top_context;
    Draw_ContextNode* free_context;
};

struct Draw_RectParams
{
    Vec4_f32 color;
    union
    {
        f32 corner_radius;
        f32 r;
    };
    union
    {
        f32 border_thickness;
        f32 t;
    };
    union
    {
        f32 softness;
        f32 s;
    };
    union
    {
        Render_RegionTex2D_f32 region;
        struct
        {
            union
            {
                Render_Hnd albedo_texture;
                Render_Hnd tex;
            };
            union
            {
                Rect2_f32 src_rect;
                Rect2_f32 src;
            };
        };
    };
};


read_only static Draw_Sampler2D draw_nil_tex2d_sample_kind = {0, Render_Sampler2D_Nearest};
read_only static Draw_XForm2D     draw_nil_xform2d = {0, {1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f}};
read_only static Draw_Clip            draw_nil_clip = {0, {}};
read_only static Draw_Opacity    draw_nil_opacity = {0, 1.f};


function u64 Draw_Hash(Buf8 data);

function Arena*        Draw_GetArena();
function Draw_Context* Draw_GetContext();
function Draw_Context* Draw_MakeContext(Arena* arena);

function void Draw_PushContext(Draw_Context* context);
function void Draw_PopContext();

function void Draw_Submit(Sys_Hnd window_handle, Draw_Context* context);
function void Draw_BeginFrame();

function Render_Pass* Draw_GetPass(Arena* arena, Draw_Context* context, Render_PassType kind);

function Render_Rect2D* Draw_Rect(Rect2_f32 rect, Draw_RectParams* p);
function f32            Draw_Text(Vec2_f32  position, Font_Tag font, f32 size, Vec4_f32 color, Str8 string);
function Render_Mesh3D* Draw_Mesh(Render_Hnd vertices, Render_Hnd indices, Render_Hnd texture, Mat4x4_f32 transform, Render_VertexFlags flags);

function void Draw_InitialiseThread();
function void Draw_Initialise();

#endif