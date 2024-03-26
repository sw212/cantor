Draw_State* draw_state = 0;
per_thread Draw_ThreadContext* draw_thread_ctx = 0;


function Arena*
Draw_GetArena()
{
    return draw_thread_ctx->arena;
}

function Draw_Context*
Draw_GetContext()
{
    return draw_thread_ctx->top_context->context;
}

function Draw_Context*
Draw_MakeContext(Arena* arena)
{
    Draw_Context* context = PushArray(arena, Draw_Context, 1);
    {
        context->top_sampler2d = &draw_nil_tex2d_sample_kind;
        context->top_xform2d = &draw_nil_xform2d;
        context->top_clip = &draw_nil_clip;
        context->top_opacity = &draw_nil_opacity;
    }

    return context;
}

function void
Draw_PushContext(Draw_Context* context)
{
    Draw_ContextNode* node = draw_thread_ctx->free_context;

    if(node)
    {
        StackPop(draw_thread_ctx->free_context);
    }
    else
    {
        node = PushArray(draw_thread_ctx->arena, Draw_ContextNode, 1);
    }

    node->context = context;
    StackPush(draw_thread_ctx->top_context, node);
}

function void
Draw_PopContext()
{
    Draw_ContextNode* node = draw_thread_ctx->top_context;
    StackPop(draw_thread_ctx->top_context);
    StackPush(draw_thread_ctx->free_context, node);
}

function Render_Pass*
Draw_GetPass(Arena* arena, Draw_Context* context, Render_PassType pass_type)
{
    Render_PassNode* node = context->passes.last;
    Render_Pass* pass = 0;

    if (!node || (node->pass.pass_type != pass_type))
    {
        pass = Render_PushPassList(arena, &context->passes, pass_type);
    }
    else
    {
        pass = &node->pass;
    }

    return pass;
}


function Render_Rect2D*
Draw_Rect(Rect2_f32 rect, Draw_RectParams* p)
{
    Arena* arena = Draw_GetArena();
    Draw_Context* context = Draw_GetContext();
    Render_Pass* pass = Draw_GetPass(arena, context, Render_PassType_UI);

    Render_Pass_UI* pass_params = pass->ui;
    Render_Collection2DList* rects = &pass_params->rects;
    Render_Collection2D* collection  = pass_params->rects.last;

    Render_Hnd tex = Render_HndIsZero(p->tex) ? draw_state->white_texture : p->tex;

    if(!collection || (context->prev_generation != context->generation) || (!Render_MatchHnd(collection->params.tex, tex) && !Render_MatchHnd(tex, draw_state->white_texture)))
    {
        collection = PushArray(arena, Render_Collection2D, 1);
        QueuePush(rects->first, rects->last, collection);
        rects->count++;
        collection->groups.instance_size = sizeof(Render_Rect2D);
        collection->params.tex = tex;
        collection->params.sampler_type = context->top_sampler2d->v;
        collection->params.xform = context->top_xform2d->v;
        collection->params.clip  = context->top_clip->v;
        collection->params.opacity  = context->top_opacity->v;
    }

    Render_Rect2D* result = (Render_Rect2D*)Render_PushGroupList(arena, &collection->groups, 256);
    result->dst = rect;
    result->src = p->src_rect;
    result->colors[Corner_00] = p->color;
    result->colors[Corner_01] = p->color;
    result->colors[Corner_10] = p->color;
    result->colors[Corner_11] = p->color;
    result->r[Corner_00] = p->r;
    result->r[Corner_01] = p->r;
    result->r[Corner_10] = p->r;
    result->r[Corner_11] = p->r;
    result->t = p->t;
    result->s = p->s;
    result->omit_tex = f32(Render_HndIsZero(p->albedo_texture));

    return result;
}

function f32
Draw_Text(Vec2_f32  position, Font_Tag font, f32 size, Vec4_f32 color, Str8 string)
{
    Arena* arena = Draw_GetArena();
    Draw_Context* context = Draw_GetContext();
    
    Scratch scratch = ScratchBegin(&arena, 1);
    Font_Run run = Font_GetRun(scratch.arena, font, size, string);
    Vec2_f32 at = position;

    for(Font_Piece* piece = run.first; piece != 0; piece = piece->next)
    {
        Vec2_f32 dims = Dimensions(piece->src_rect);
        Vec2_f32 bl = Add(at, piece->offset);
        Vec2_f32 tr = Add(bl, dims);
        Rect2_f32 rect = {bl, tr};

        Draw_RectParams params = {.color = color, .tex = piece->texture, .src_rect = piece->src_rect};
        Draw_Rect(rect, &params);
        
        at.x += piece->advance;
    }

    ScratchEnd(scratch);

    f32 result = at.x - position.x;
    return result;
}

function void
Draw_Submit(Render_Hnd handle, Draw_Context* context)
{
    Render_WindowSubmit(handle, &context->passes);
}

function void
Draw_BeginFrame()
{
    if (!draw_thread_ctx)
    {
        Arena* arena = ArenaAlloc(Megabytes(1));
        draw_thread_ctx = PushArray(arena, Draw_ThreadContext, 1);
        draw_thread_ctx->arena = arena;
        draw_thread_ctx->start_of_frame_arena_pos = ArenaPos(arena);
    }

    ArenaPopTo(draw_thread_ctx->arena, draw_thread_ctx->start_of_frame_arena_pos);
    draw_thread_ctx->free_context = 0;
    draw_thread_ctx->top_context  = 0;
}

function void
Draw_InitialiseThread()
{
    if (!draw_thread_ctx)
    {
        Arena* arena = ArenaAlloc(Megabytes(1));
        draw_thread_ctx = PushArray(arena, Draw_ThreadContext, 1);
        draw_thread_ctx->arena = arena;
        draw_thread_ctx->start_of_frame_arena_pos = ArenaPos(arena);
    }

    ArenaPopTo(draw_thread_ctx->arena, draw_thread_ctx->start_of_frame_arena_pos);
    draw_thread_ctx->free_context = 0;
    draw_thread_ctx->top_context  = 0;
}

function void
Draw_Initialise()
{
    if (IsCurrentThreadMain() && !draw_state)
    {
        Arena* arena = ArenaAlloc(Gigabytes(8));
        draw_state = PushArray(arena, Draw_State, 1);
        draw_state->arena = arena;

        u32 white_tex[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
        draw_state->white_texture = Render_AllocTex2D({2, 2}, Render_Tex2D_RGBA8, Render_Tex2DUsage_Static, white_tex);

        Draw_InitialiseThread();
    }
}


#define Draw_StackPush(name_upper, name_lower, type, val) \
    Draw_Context *context = Draw_GetContext();\
    type old_val = context->top_##name_lower->v;\
    Draw_##name_upper *node = PushArray(draw_thread_ctx->arena, Draw_##name_upper, 1);\
    node->v = (val);\
    StackPush(context->top_##name_lower, node);\
    context->generation += 1;\
    return old_val

#define Draw_StackPop(name_upper, name_lower, type) \
    Draw_Context *context = Draw_GetContext();\
    type popped_val = context->top_##name_lower->v;\
    StackPop(context->top_##name_lower);\
    context->generation += 1;\
    return popped_val

#define Draw_StackTop(name_upper, name_lower, type) \
    Draw_Context *context = Draw_GetContext();\
    type top_val = context->top_##name_lower->v;\
    return top_val

function Render_Sampler2DType Draw_PushSampler2D(Render_Sampler2DType v) {Draw_StackPush(Sampler2D, sampler2d, Render_Sampler2DType, v);}
function Mat3x3_f32       Draw_PushXForm2D(Mat3x3_f32 v)         {Draw_StackPush(XForm2D, xform2d, Mat3x3_f32, v);}
function Rect2_f32        Draw_PushClip(Rect2_f32 v)             {Draw_StackPush(Clip, clip, Rect2_f32, v);}
function f32              Draw_PushOpacity(f32 v)                {Draw_StackPush(Opacity, opacity, f32, v);}

function Render_Sampler2DType Draw_PopSampler2D()                    {Draw_StackPop(Sampler2D, sampler2d, Render_Sampler2DType);}
function Mat3x3_f32       Draw_PopXForm2D()                      {Draw_StackPop(XForm2D, xform2d, Mat3x3_f32);}
function Rect2_f32        Draw_PopClip()                         {Draw_StackPop(Clip, clip, Rect2_f32);}
function f32              Draw_PopOpacity()                      {Draw_StackPop(Opacity, opacity, f32);}

function Render_Sampler2DType Draw_TopSampler2D()                    {Draw_StackTop(Sampler2D, sampler2d, Render_Sampler2DType);}
function Mat3x3_f32       Draw_TopXForm2D()                      {Draw_StackTop(XForm2D, xform2d, Mat3x3_f32);}
function Rect2_f32        Draw_TopClip()                         {Draw_StackTop(Clip, clip, Rect2_f32);}
function f32              Draw_TopOpacity()                      {Draw_StackTop(Opacity, opacity, f32);}

#undef Draw_StackPush
#undef Draw_StackPop
#undef Draw_StackTop