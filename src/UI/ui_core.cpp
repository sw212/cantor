f32 ui_cursor_blink_t = 0;

read_only UI_Wig ui_nil_wig = { &ui_nil_wig, &ui_nil_wig, &ui_nil_wig, &ui_nil_wig, &ui_nil_wig, 0, &ui_nil_wig, &ui_nil_wig, };
read_only UI_WigText   ui_nil_wig_text = {};
read_only UI_WigStyle  ui_nil_wig_style = {};
read_only UI_WigBucket ui_nil_wig_bucket = {};


per_thread UI_Ctx* ui_ctx = 0;


function Arena*
UI_Arena()
{
    return ui_ctx->frame_arenas[ui_ctx->generation % ArrayCount(ui_ctx->frame_arenas)];
}

function UI_Wig*
UI_Root()
{
    return ui_ctx->root;
}

function Sys_Hnd
UI_Window()
{
    return ui_ctx->window;
}

function UI_EventList*
UI_Events()
{
    return ui_ctx->event_list;
}

function Vec2_f32
UI_Mouse()
{
    return ui_ctx->mouse_pos;
}

function UI_Ctx*
UI_Initialise()
{
    Arena* arena = ArenaAlloc(Gigabytes(64));

    UI_Ctx* ui = PushArray(arena, UI_Ctx, 1);
    ui->arena = arena;
    ui->wig_table_size = 4096;
    ui->wig_table = PushArray(arena, UI_WigSlot, ui->wig_table_size);
    for(u64 idx = 0; idx < ArrayCount(ui->frame_arenas); idx++)
    {
        ui->frame_arenas[idx] = ArenaAlloc(Gigabytes(8));
    }
    ui->drag_arena = ArenaAlloc(Megabytes(64));

    Hash_SubmitStaticData(ui_font_data, ui_font_hash);
    
    UI_InitialiseUINilStateStacks(ui);

    return ui;
}

function void
UI_SetState(UI_Ctx* ui)
{
    ui_ctx = ui;
}

function void
UI_Begin(Sys_Hnd window_handle, Sys_EventList* event_list)
{
    ui_ctx->generation++;
    ArenaClear(UI_Arena());

    ui_ctx->root = &ui_nil_wig;
    ui_ctx->window = window_handle;
    ui_ctx->event_list = event_list;

    ui_ctx->frame_has_hot = 0;
    ui_ctx->mouse_pos = Sys_GetMouse(window_handle);

    UI_InitialiseUIStateStacks(ui_ctx);

    // if clear signal -> clear hot & active
    if (ui_ctx->clear_hot_active)
    {
        ui_ctx->clear_hot_active = 0;
        MemoryZeroStruct(&ui_ctx->hot);
        MemoryZeroArray(ui_ctx->active);
    }

    // clear unused wigs from hash table
    for (u64 slot = 0; slot < ui_ctx->wig_table_size; slot++)
    {
        for (UI_Wig* wig = ui_ctx->wig_table[slot].first,* next = 0; !UI_IsNil(wig); wig = next)
        {
            next = wig->hash_next;

            if (UI_MatchKey(wig->key, UI_ZeroKey()) || wig->curr_generation + 1 < ui_ctx->generation)
            {
                DLLRemove_NPZ(ui_ctx->wig_table[slot].first, ui_ctx->wig_table[slot].last, wig, hash_next, hash_prev, UI_IsNil, UI_WigSetNil);
                StackPush(ui_ctx->free_wig, wig);
                ui_ctx->free_wig_count++;
            }
        }
    }

    // clear hot/active keys for cleared wigs
    {
        // active
        for (Side side = Side(0); side < Side_Count; side = Side(side + 1))
        {
            UI_Wig* wig = UI_GetWig(ui_ctx->active[side]);
            if (UI_IsNil(wig))
            {
                Sys_Key key = (side == Side_Min ? Sys_Key_MouseLeft : Sys_Key_MouseRight);
                b32 release = 0;

                for (Sys_Event* event = event_list->first; event != 0; event = event->next)
                {
                    if (Sys_HndMatch(window_handle, event->window) && event->type == Sys_Event_Release && event->key == key)
                    {
                        release = 1;
                        break;
                    }
                }

                if (release)
                {
                    MemoryZeroStruct(&ui_ctx->active[side]);
                }
            }
        }

        // hot
        UI_Wig* wig = UI_GetWig(ui_ctx->hot);
        if ((UI_IsNil(wig)) &&
            (UI_MatchKey(UI_ZeroKey(), ui_ctx->active[Side_Min]) || !UI_MatchKey(ui_ctx->hot, ui_ctx->active[Side_Min])) &&
            (UI_MatchKey(UI_ZeroKey(), ui_ctx->active[Side_Max]) || !UI_MatchKey(ui_ctx->hot, ui_ctx->active[Side_Max])))
        {
            ui_ctx->hot = UI_ZeroKey();
        }
    }

    // root
    Rect2_f32 client_rect = Sys_GetClientRect(window_handle);
    Vec2_f32  client_dims = Dimensions(client_rect);
    UI_SetNextDesiredWidth(UI_Pixels(client_dims.x, 1.f));
    UI_SetNextDesiredHeight(UI_Pixels(client_dims.y, 1.f));
    UI_SetNextLayoutDirection(Axis2_Y);
    UI_Wig* root = UI_WigCreate(0, (char*)("root_%" PRIx64 ""), window_handle.v[0]);
    UI_SetParent(root);
}

function void
UI_End()
{
    UI_PopParent();
}


function UI_Event*
UI_EventPush(Arena* arena, UI_EventList* event_list)
{
    UI_EventItem* item = PushArray(arena, UI_EventItem, 1);
    UI_Event* result = &item->event;

    event_list->count++;
    DLLPushBack(event_list->first, event_list->last, item);

    return result;
}

function void
UI_ConsumeEvent(UI_EventList* event_list, UI_EventItem* event)
{
    DLLRemove(event_list->first, event_list->last, event);
    event_list->count--;
}

function UI_EventType
UI_EventTypeFromSysEvent(Sys_EventType type)
{
    UI_EventType result = UI_Event_NOP;

    switch (type)
    {
        case Sys_Event_Press:   { result = UI_Event_Press;   } break;
        case Sys_Event_Release: { result = UI_Event_Release; } break;
        case Sys_Event_Key:     { result = UI_Event_Key;     } break;
        default: break;
    }

    return result;
}

function UI_EventList
UI_EventListFromSysEvents(Arena* arena, Sys_Hnd window, Sys_EventList* sys_events)
{
    UI_EventList result = {};

    for (Sys_Event* se = sys_events->first; se != 0; se = se->next)
    {
        if (!Sys_HndMatch(se->window, window))
        {
            continue;
        }

        b32 consume = 0;

        // TODO: special keys (e.g. ESC)?

        // general keys
        if (!consume && se->type == Sys_Event_Key)
        {
            UI_Event* e = UI_EventPush(arena, &result);
            e->type = UI_Event_Key;
            e->string = String8(arena, String32(&se->character, 1)); 
        }

        if (!consume && (se->type == Sys_Event_Press || se->type == Sys_Event_Release))
        {
            consume = 1;
            UI_Event* e = UI_EventPush(arena, &result);
            e->type = UI_EventTypeFromSysEvent(se->type);
            e->key  = se->key;
            e->modifiers = se->modifiers;
            e->position  = se->position;
        }

        if (consume)
        {
            Sys_ConsumeEvent(sys_events, se);
        }
    }

    return result;
}

function b32
UI_IsNil(UI_Wig* wig)
{
    return wig == 0 || wig == &ui_nil_wig;
}

function UI_Wig*
UI_GetWig(UI_Key key)
{
    UI_Wig* result = &ui_nil_wig;
    u64 slot = key.v[0] % ui_ctx->wig_table_size;

    if (!UI_MatchKey(key, UI_ZeroKey()))
    {
        for (UI_Wig* wig = ui_ctx->wig_table[slot].first; !UI_IsNil(wig); wig = wig->hash_next)
        {
            if (UI_MatchKey(wig->key, key))
            {
                result = wig;
                break;
            }
        }
    }

    return result;
}

function UI_Key
UI_ZeroKey()
{
    UI_Key result = {};
    return result;
}

function b32
UI_MatchKey(UI_Key a, UI_Key b)
{
    return a.v[0] == b.v[0];
}

function UI_Key
UI_GetKey(UI_Key seed, Str8 string)
{
    UI_Key key = {};
    if(string.size != 0)
    {
        MemoryCopyStruct(&key, &seed);

        for(u64 i = 0; i < string.size; i++)
        {
            key.v[0] = ((key.v[0] << 7) + key.v[0]) + string.str[i];
        }
    }

    return key;
}

function UI_Key
UI_GetKey(UI_Key seed, char* fmt, ...)
{
    Scratch scratch = ScratchBegin(0, 0);
    va_list args;
    va_start(args, fmt);
    Str8 string = StrPushFV(scratch.arena, fmt, args);
    UI_Key result = UI_GetKey(seed, string);
    va_end(args);
    ScratchEnd(scratch);

    return result;
}

function Str8
UI_WigGetHashString(Str8 string)
{
    u64 hash_pos = FindSubstr(string, StringLiteral("###"), 0, 0);

    if(hash_pos < string.size)
    {
        string = StrSkip(string, hash_pos);
    }

    return string;
}

function UI_Size
UI_Pixels(f32 value, f32 fractional_min_size)
{
    UI_Size result = {UI_Size_Px, value, fractional_min_size};
    return result;
}

function UI_WigTraversal
UI_TraverseRevPreOrder(UI_Wig* wig, UI_Wig* stop_wig, u64 sibling_offset, u64 child_offset)
{
    UI_WigTraversal result = {};
    result.next = &ui_nil_wig;

    #define BoxFromOffset(wig, offset)* (UI_Wig**)((u8*)wig + offset)
    if(!UI_IsNil(BoxFromOffset(wig, child_offset)))
    {
        result.next = BoxFromOffset(wig, child_offset);
        result.push_count = 1;
    }
    else for(UI_Wig* w = wig; !UI_IsNil(w) && w != stop_wig; w = w->parent)
    {
        if(!UI_IsNil(BoxFromOffset(w, sibling_offset)))
        {
            result.next = BoxFromOffset(w, sibling_offset);
            break;
        }

        result.pop_count += 1;
    }

    return result;
    #undef BoxFromOffset
}

function void
UI_ResolveWigDimsFreePass(UI_Wig* node, Axis2 axis)
{
    //
    // calculate sizes for widgets that can be detemined independently (standalone) from others
    //  - size that is specified explicitly (e.g. size given in pixels)
    //  - size that can be calculated immediately e.g. a rect containing some string
    //

    switch(node->initial_size[axis].type)
    {
        case UI_Size_Px:
        {
            node->calculated_size.v[axis] = node->initial_size[axis].value;
            node->calculated_size.v[axis] = Floor(node->calculated_size.v[axis]);
        } break;

        case UI_Size_Text:
        {
            Str8 string = UI_WigGetDisplayString(node);

            switch (axis)
            {
                case Axis2_X:
                {
                    node->calculated_size.v[axis] = Font_GetAdvance(node->text->font_tag, node->text->font_size, string);
                    node->calculated_size.v[axis] += node->text->padding*2.f;
                    node->calculated_size.v[axis] = Ceil(node->calculated_size.v[axis]);
                } break;
                
                case Axis2_Y:
                {
                    Font_Metrics metrics = Font_GetMetrics(node->text->font_tag, node->text->font_size);
                    node->calculated_size.v[axis] = metrics.line_adv + metrics.ascent + metrics.descent;
                    node->calculated_size.v[axis] = Floor(node->calculated_size.v[axis]);
                } break;
            }
        } break;

        default: break;
    }

    for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
    {
        UI_ResolveWigDimsFreePass(child, axis);
    }
}

function void
UI_ResolveWigDimsDownPass(UI_Wig* node, Axis2 axis)
{
    //
    // top -> bottom
    // left -> right
    // calculate sizes for widgets that depend on parent size
    //  - size that is specified as some fraction of its parent
    //

    switch(node->initial_size[axis].type)
    {
        case UI_Size_Percent:
        {
            UI_Wig* parent = &ui_nil_wig;
            
            for (UI_Wig* b = node->parent; !UI_IsNil(b); b = b->parent)
            {
                if (b->initial_size[axis].type != UI_Size_Children)
                {
                    parent = b;
                    break;
                }
            }

            if (!UI_IsNil(parent))
            {
                node->calculated_size.v[axis] = parent->calculated_size.v[axis] * node->initial_size[axis].value;
                node->calculated_size.v[axis] = Floor(node->calculated_size.v[axis]);       
            }
        } break;

        default: break;
    }

    for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
    {
        UI_ResolveWigDimsDownPass(child, axis);
    }
}

function void
UI_ResolveWigDimsUpPass(UI_Wig* node, Axis2 axis)
{
    //
    // bottom -> top
    // left -> right
    // calculate sizes for widgets that depend on children size
    //  - size that is specified as the union of child rects
    //

    for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
    {
        UI_ResolveWigDimsDownPass(child, axis);
    }

    switch (node->initial_size[axis].type)
    {
        case UI_Size_Children:
        {
            f32 value = 0.f;

            for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
            {
                value = (axis == node->layout_direction) ? (value + child->calculated_size.v[axis]) :
                                                            Max(value, child->calculated_size.v[axis]);
            }

            node->calculated_size.v[axis] = Floor(value);
        } break;

        default: break;
    }
}

function void
UI_ResolveWigDimsAdjustmentPass(UI_Wig* node, Axis2 axis)
{
    b32 overflow_allowed = b32(node->flags & (UI_Wig_OverflowX << axis));

    f32 node_size = node->calculated_size.v[axis];
    f32 children_size = 0.f;
    f32 max_allowed_adjustment = 0.f;

    if (!overflow_allowed)
    {
        for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
        {
            b32 wig_floating = b32(child->flags & (UI_Wig_FloatX << axis));

            if (!wig_floating)
            {
                if (axis == node->layout_direction)
                {
                    children_size += child->calculated_size.v[axis];
                }
                else
                {
                    children_size = Max(children_size, child->calculated_size.v[axis]);
                }

                f32 child_allowed_adjustment = child->calculated_size.v[axis] * (1.f - child->initial_size[axis].fractional_min_size);
                max_allowed_adjustment += child_allowed_adjustment;
            }
        }

        f32 overflow = children_size - node_size;

        if ((overflow > 0) && (max_allowed_adjustment > 0))
        {
            for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
            {
                b32 wig_floating = b32(child->flags & (UI_Wig_FloatX << axis));

                if (!wig_floating)
                {
                    f32 child_allowed_adjustment = child->calculated_size.v[axis] * (1.f - child->initial_size[axis].fractional_min_size);
                    f32 child_size_adjust = 0.f;

                    if (axis == node->layout_direction)
                    {
                        child_size_adjust = (child_allowed_adjustment / max_allowed_adjustment) * overflow;
                    }
                    else
                    {
                        child_size_adjust = child->calculated_size.v[axis] - node_size;
                    }

                    child_size_adjust = Clamp(0, child_size_adjust, child_allowed_adjustment);
                    child->calculated_size.v[axis] -= child_size_adjust;
                    child->calculated_size.v[axis] = Floor(child->calculated_size.v[axis]);
                }
            }
        }
    }

    // positioning
    {
        if (axis == node->layout_direction)
        {
            f32 at = 0.f;

            for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
            {
                b32 wig_floating = b32(child->flags & (UI_Wig_FloatX << axis));

                if (!wig_floating)
                {
                    child->relative_pos.v[axis] = at;
                    at += child->calculated_size.v[axis];
                }                
            }
        }
        else
        {
            for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
            {
                b32 wig_floating = b32(child->flags & (UI_Wig_FloatX << axis));
                if (!wig_floating)
                {
                    child->relative_pos.v[axis] = 0.f;
                }
            }
        }

        for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
        {
            Rect2_f32 prev_rect_relative = child->rect_relative;
            child->rect_relative.min.v[axis] = child->relative_pos.v[axis];
            child->rect_relative.max.v[axis] = child->rect_relative.min.v[axis] + child->calculated_size.v[axis];

            Vec2_f32 prev_rect_tl = {prev_rect_relative.min.x, prev_rect_relative.max.y};
            Vec2_f32      rect_tl = {child->rect_relative.min.x, child->rect_relative.max.y};

            Vec2_f32 prev_rect_br = {prev_rect_relative.max.x, prev_rect_relative.min.y};
            Vec2_f32      rect_br = {child->rect_relative.max.x, child->rect_relative.min.y};
            
            child->rect_delta[Corner_01].v[axis] = rect_tl.v[axis] - prev_rect_tl.v[axis];
            child->rect_delta[Corner_10].v[axis] = rect_br.v[axis] - prev_rect_br.v[axis];

            child->rect_delta[Corner_00].v[axis] = child->rect_relative.min.v[axis] - prev_rect_relative.min.v[axis];
            child->rect_delta[Corner_11].v[axis] = child->rect_relative.max.v[axis] - prev_rect_relative.max.v[axis];

            child->rect.min.v[axis] = node->rect.min.v[axis] + child->rect_relative.min.v[axis] - node->view_off.v[axis];
            child->rect.max.v[axis] = child->rect.min.v[axis] + child->calculated_size.v[axis];

            b32 wig_floating = b32(child->flags & (UI_Wig_FloatX << axis));
            if (!wig_floating)
            {
                child->rect.min.v[axis] = Floor(child->rect.min.v[axis]);
                child->rect.max.v[axis] = Floor(child->rect.max.v[axis]);
            }
        }
    }

    for (UI_Wig* child = node->first; !UI_IsNil(child); child = child->next)
    {
        UI_ResolveWigDimsAdjustmentPass(child, axis);
    }
}

function void
UI_ResolveWigDimsFor(UI_Wig* wig, Axis2 axis)
{
    UI_ResolveWigDimsFreePass(wig, axis);
    UI_ResolveWigDimsDownPass(wig, axis);
    UI_ResolveWigDimsUpPass(wig, axis);
    UI_ResolveWigDimsAdjustmentPass(wig, axis);
}

function void
UI_ResolveWigDims()
{
    UI_Wig* root = UI_Root();
    UI_ResolveWigDimsFor(root, Axis2_X);
    UI_ResolveWigDimsFor(root, Axis2_Y);
}

function UI_Wig*
UI_WigCreate(UI_WigFlags flags, UI_Key key)
{
    UI_Wig* wig = UI_GetWig(key);

    // already in use
    if (wig->curr_generation == ui_ctx->generation)
    {
        wig = &ui_nil_wig;
        key = UI_ZeroKey();
    }

    // if not allocated / wig in use
    b32 did_allocate = 0;
    if (UI_IsNil(wig))
    {
        u64 slot = key.v[0] % ui_ctx->wig_table_size;
        did_allocate = 1;
        wig = ui_ctx->free_wig;

        if (UI_IsNil(wig))
        {
            wig = PushArray(ui_ctx->arena, UI_Wig, 1);
        }
        else
        {
            StackPop(ui_ctx->free_wig);
            MemoryZeroStruct(wig);
            ui_ctx->free_wig_count--;
        }

        DLLPushBack_NPZ(ui_ctx->wig_table[slot].first, ui_ctx->wig_table[slot].last, wig, hash_next, hash_prev, UI_IsNil, UI_WigSetNil);
        wig->key = key;
    }

    // insert wig
    UI_Wig* parent = UI_GetParent();
    if (UI_IsNil(parent))
    {
        ui_ctx->root = wig;
    }
    else
    {
        DLLPushBack_NPZ(parent->first, parent->last, wig, next, prev, UI_IsNil, UI_WigSetNil);
        parent->child_count++;
        wig->parent = parent;
    }

    // wig state
    if (!UI_IsNil(wig))
    {
        wig->child_count = 0;
        wig->first = wig->last = &ui_nil_wig;
        wig->flags = flags | UI_GetFlags();
        wig->initial_size[Axis2_X] = UI_GetDesiredWidth();
        wig->initial_size[Axis2_Y] = UI_GetDesiredHeight();
        wig->layout_direction  = UI_GetLayoutDirection();
        wig->curr_generation   = ui_ctx->generation;
        
        wig->text = &ui_nil_wig_text;
        wig->style = &ui_nil_wig_style;
        wig->bucket = &ui_nil_wig_bucket;
        
        if (wig->flags & UI_Wig_HasText)
        {
            wig->text = PushArray(UI_Arena(), UI_WigText, 1);
            wig->text->alignment  = UI_GetTextAlignment();
            wig->text->padding    = UI_GetTextPadding();
            wig->text->font_tag   = UI_GetFont();
            wig->text->font_size  = UI_GetFontSize();
            wig->text->color      = UI_GetColorText();
        }

        if (wig->flags & (UI_Wig_DrawBG))
        {
            wig->style = PushArray(UI_Arena(), UI_WigStyle, 1);
            wig->style->color_background = UI_GetColorBackground();
            wig->style->region           = UI_GetRegionf32();
        }
        
        // absolute position
        wig->relative_pos.x = UI_GetAbsoluteX();
        wig->relative_pos.y = UI_GetAbsoluteY();
        
        if (did_allocate)
        {
            wig->init_generation = ui_ctx->generation;
        }
    }

    UI_ProcessDefaultUIStateStacks(ui_ctx);

    return wig;
}

function UI_Wig*
UI_WigCreate(UI_WigFlags flags, Str8 string)
{
    UI_Key seed = UI_GetSeedKey();

    Str8 hash_string = UI_WigGetHashString(string);
    UI_Key key = UI_GetKey(seed, hash_string);

    UI_Wig* wig = UI_WigCreate(flags, key);
    wig->string = StrPushCopy(UI_Arena(), string);

    return wig;
}

function UI_Wig*
UI_WigCreate(UI_WigFlags flags, char* fmt, ...)
{
    Scratch scratch = ScratchBegin(0, 0);
    va_list args;

    va_start(args, fmt);
    Str8 string = StrPushFV(scratch.arena, fmt, args);
    UI_Wig* result = UI_WigCreate(flags, string);
    va_end(args);

    ScratchEnd(scratch);

    return result;
}

function void
UI_WigSetDisplayString(UI_Wig* wig, Str8 string)
{
    wig->string = StrPushCopy(UI_Arena(), string);
    wig->flags |= UI_Wig_IgnoreHash;
}

function Str8
UI_WigGetDisplayString(Str8 string)
{
    u64 pos = FindSubstr(string, StringLiteral("##"), 0, 0);

    if (pos < string.size)
    {
        string.size = pos;
    }

    return string;
}

function Str8
UI_WigGetDisplayString(UI_Wig* wig)
{
    Str8 result = wig->string;

    if (!(wig->flags & UI_Wig_IgnoreHash))
    {
        result = UI_WigGetDisplayString(result);
    }

    return result;
}

function Vec2_f32
UI_GetTextPos(UI_Wig* wig)
{
    Font_Tag tag = wig->text->font_tag;
    Font_Metrics metrics = Font_GetMetrics(tag, wig->text->font_size);
    Vec2_f32 result = {};
    {
        result.y = ((wig->rect.min.y + wig->rect.max.y) / 2.f) + (metrics.ascent / 2.f) - (metrics.descent / 4.f);
    }
    
    switch(wig->text->alignment)
    {
        case UI_Align_Left:
        {
            result.x = wig->rect.min.x + wig->text->padding;
        } break;

        case UI_Align_Center:
        {
            Str8 display_string = UI_WigGetDisplayString(wig);
            f32  string_advance = Font_GetAdvance(wig->text->font_tag, wig->text->font_size, display_string);

            result.x = ((wig->rect.min.x + wig->rect.max.x) / 2.f) - (string_advance / 2.f);
        } break;

        case UI_Align_Right:
        {
            Str8 display_string = UI_WigGetDisplayString(wig);
            f32  string_advance = Font_GetAdvance(wig->text->font_tag, wig->text->font_size, display_string);

            result.x = (wig->rect.max.x - wig->text->padding) - string_advance;
        } break;

        default: break;
    }

    return result;
}

function UI_Action
UI_GetAction(UI_Wig* wig)
{
    UI_Action result = {};
    result.wig = wig;
    UI_EventList* event_list = UI_Events();
    Rect2_f32 rect = wig->rect;

    // clipping
    Rect2_f32 clip_rect = wig->rect;
    for (UI_Wig* parent = wig->parent; !UI_IsNil(parent); parent = parent->parent)
    {
        if (parent->flags & UI_Wig_Clip)
        {
            clip_rect = Intersection(clip_rect, parent->rect);
        }
    }

    // event handling
    for(UI_EventItem* event_item = event_list->first; event_item != 0; event_item = event_item->next)
    {
        b32 consume = 0;

        UI_Event* event = &event_item->event;

        b32 is_mouse_event = (event->key == Sys_Key_MouseLeft || event->key == Sys_Key_MouseRight);
        b32 event_in_clip_rect = Contains(clip_rect, event->position);

        // if can mouse interact && wig was not created this frame
        if (wig->flags & UI_Wig_AllowMouse && wig->init_generation != wig->curr_generation)
        {
            // mouse down event inside wig
            if (is_mouse_event && event_in_clip_rect && event->type == UI_Event_Press)
            {
                consume = 1;
                ui_ctx->hot = wig->key;

                if (event->key == Sys_Key_MouseLeft)
                {
                    ui_ctx->active[UI_MouseButton_Left] = wig->key;
                    ui_ctx->drag_start_pos = event->position;
                    result.left_pressed = 1;
                }
                if (event->key == Sys_Key_MouseRight)
                {
                    ui_ctx->active[UI_MouseButton_Right] = wig->key;
                    result.right_pressed = 1;
                }
            }

            // mouse button release for active wig
            if (is_mouse_event && event->type == UI_Event_Release)
            {
                if (UI_MatchKey(ui_ctx->active[UI_MouseButton_Left], wig->key))
                {
                    consume = 1;
                    result.left_released = 1;
                    
                    if (event_in_clip_rect)
                    {
                        result.left_pressed = 1;
                    }

                    ui_ctx->active[UI_MouseButton_Left] = UI_ZeroKey();
                }

                if (UI_MatchKey(ui_ctx->active[UI_MouseButton_Right], wig->key))
                {
                    consume = 1;
                    result.right_released = 1;
                    
                    if (event_in_clip_rect)
                    {
                        result.right_pressed = 1;
                    }

                    ui_ctx->active[UI_MouseButton_Right] = UI_ZeroKey();
                }
            }
        }

        if (consume)
        {
            UI_ConsumeEvent(event_list, event);
        }
    }

    Vec2_f32 mouse_pos = UI_Mouse();
    b32 mouse_in_clip_rect = Contains(clip_rect, mouse_pos);
    // if there is:
    //  no curr hot && no curr active && mouse is over -> set hot
    if (mouse_in_clip_rect)
    {
        result.mouse_is_over = 1;

        if (UI_MatchKey(ui_ctx->hot, UI_ZeroKey())) // no curr hot wig
        {
            result.hovering = 1;

            u32 active_wig = 0;

            for (UI_MouseButtonType mb = UI_MouseButtonType(0); mb < UI_MouseButton_Count; mb = UI_MouseButtonType(mb + 1))
            {
                if (!UI_MatchKey(ui_ctx->active[mb], UI_ZeroKey()))
                {
                    active_wig = 1;
                    break;
                }
            }

            if (!active_wig) // no curr active wig ( for any mouse button )
            {
                ui_ctx->hot = wig->key;
            }
        }
    }
    // wig is hot && does not contain mouse -> clear hot
    else if (UI_MatchKey(ui_ctx->hot, wig->key))
    {
        ui_ctx->hot = UI_ZeroKey();
    }

    if (wig->flags & UI_Wig_AllowMouse)
    {
        if (UI_MatchKey(ui_ctx->active[UI_MouseButton_Left], wig->key))
        {
            result.left_dragging = 1;
            ui_ctx->hot = wig->key;
        }

        if (UI_MatchKey(ui_ctx->active[UI_MouseButton_Right], wig->key))
        {
            result.right_dragging = 1;
            ui_ctx->hot = wig->key;
        }
    }

    return result;
}

function UI_Action
UI_GetActionOld(UI_Wig* wig)
{
    UI_Action result = {};
    result.wig = wig;

    Rect2_f32 rect = wig->rect;
    Vec2_f32 mouse_pos = UI_Mouse();
    b32 wig_contains_mouse = Contains(rect, mouse_pos);

    // clipping
    {
        Rect2_f32 clip_rect = wig->rect;

        for (UI_Wig* parent = wig->parent; !UI_IsNil(parent); parent = parent->parent)
        {
            if (parent->flags & UI_Wig_Clip)
            {
                clip_rect = Intersection(clip_rect, parent->rect);
            }
        }

        if (!Contains(clip_rect, mouse_pos))
        {
            wig_contains_mouse = 0;
        }
    }

    result.mouse_is_over = u8(wig_contains_mouse);

    // mouse events
    if (wig->init_generation != wig->curr_generation && wig->flags & UI_Wig_AllowMouse)
    {
        Sys_Event* left_pressed = 0;
        Sys_Event* right_pressed = 0;
        Sys_Event* left_released = 0;
        Sys_Event* right_released = 0;
        
        for(Sys_Event* event = UI_EventList()->first; event != 0; event = event->next)
        {
            if(Sys_HndMatch(event->window, UI_Window()))
            {
                if(event->type == Sys_Event_Press   && event->key == Sys_Key_MouseLeft)  { left_pressed = event; }
                if(event->type == Sys_Event_Press   && event->key == Sys_Key_MouseRight) { right_pressed = event; }
                if(event->type == Sys_Event_Release && event->key == Sys_Key_MouseLeft)  { left_released = event; }
                if(event->type == Sys_Event_Release && event->key == Sys_Key_MouseRight) { right_released = event; }
            }
        }

        // hot wig
        if ((UI_MatchKey(ui_ctx->active[Side_Min], UI_ZeroKey())) &&              // no curr active key
            (UI_MatchKey(ui_ctx->hot, UI_ZeroKey()) || !ui_ctx->frame_has_hot) && // no curr hot key || no hot key so far
            (wig_contains_mouse))                                                 // wig contains mouse
        {
            ui_ctx->hot = wig->key;
            ui_ctx->frame_has_hot = 1;
        }
        else if((UI_MatchKey(ui_ctx->hot, wig->key) ) &&                                 // wig is hot
                (wig_contains_mouse || UI_MatchKey(ui_ctx->active[Side_Min], wig->key))) // wig contains mouse || wig is active
        {
            ui_ctx->hot = wig->key;
            ui_ctx->frame_has_hot = 1;
        }
        else if (UI_MatchKey(ui_ctx->hot, wig->key) && !wig_contains_mouse) // wig is hot && does not contain mouse
        {
            ui_ctx->hot = UI_ZeroKey();
        }

        // left active wig
        if (UI_MatchKey(ui_ctx->hot, wig->key) &&                  // wig is hot
            UI_MatchKey(ui_ctx->active[Side_Min], UI_ZeroKey()) && // no curr active key
            left_pressed)                                          // LMB event
        {
            Sys_ConsumeEvent(UI_EventList(), left_pressed);
            ui_ctx->active[Side_Min] = wig->key;
            ui_ctx->drag_start_pos = mouse_pos;
            result.pressed = 1;
            result.dragging = 1;
            result.modifiers |= left_pressed->modifiers;
        }
        else if (UI_MatchKey(ui_ctx->active[Side_Min], wig->key))  // wig is active
        {
            result.dragging = 1;
            result.modifiers |= Sys_GetModifiers();

            if (left_released)
            {
                Sys_ConsumeEvent(UI_EventList(), left_released);
                ui_ctx->active[Side_Min] = UI_ZeroKey();
                result.clicked = u8(wig_contains_mouse);
                result.released = 1;
                result.modifiers |= left_released->modifiers;
            }
        }

        // right active wig
        if (UI_MatchKey(ui_ctx->hot, wig->key) &&                  // wig is hot
            UI_MatchKey(ui_ctx->active[Side_Max], UI_ZeroKey()) && // no curr active key
            right_pressed)                                         // RMB event
        {
            Sys_ConsumeEvent(UI_EventList(), right_pressed);
            ui_ctx->active[Side_Max] = wig->key;
            ui_ctx->drag_start_pos = mouse_pos;
            result.right_pressed = 1;
            result.right_dragging = 1;
            result.modifiers |= right_pressed->modifiers;
        }
        else if (UI_MatchKey(ui_ctx->active[Side_Max], wig->key))  // wig is active
        {
            result.right_dragging = 1;
            result.modifiers |= Sys_GetModifiers();

            if (right_released)
            {
                Sys_ConsumeEvent(UI_EventList(), right_released);
                ui_ctx->active[Side_Max] = UI_ZeroKey();
                result.right_clicked = u8(wig_contains_mouse);
                result.right_released = 1;
                result.modifiers |= right_released->modifiers;
            }
        }

        if(UI_MatchKey(ui_ctx->hot, wig->key) &&
           UI_MatchKey(ui_ctx->active[Side_Min], UI_ZeroKey()) &&
           UI_MatchKey(ui_ctx->active[Side_Max], UI_ZeroKey()) &&
           wig_contains_mouse)
        {
            result.hovering = 1;
        }
    }

    if ((wig->flags & UI_Wig_Disabled) || ui_ctx->clear_hot_active)
    {
        result.clicked = 0;
        result.pressed = 0;
        result.double_clicked = 0;
        result.right_clicked = 0;
        result.right_pressed = 0;
        result.keyboard_pressed = 0;
        result.dragging = 0;
    }

    return result;
}


// - Stack

function void
UI_InitialiseUIStateStacks(UI_Ctx* ui)
{
    #define InitStack(name) ui->name##_stack.top = &ui->name##_nil; ui->name##_stack.free = 0; ui->name##_stack.pop_default = 0;
    InitStack(parent)
    InitStack(flags)

    InitStack(absolute_x)
    InitStack(absolute_y)

    InitStack(desired_width)
    InitStack(desired_height)

    InitStack(opacity)
    InitStack(color_text)
    InitStack(color_background)
    InitStack(color_border)

    InitStack(regionf32)

    InitStack(font)
    InitStack(font_size)

    InitStack(layout_direction)

    InitStack(text_alignment)
    InitStack(text_padding)

    InitStack(seed_key)
    #undef InitStack
}

function void
UI_InitialiseUINilStateStacks(UI_Ctx* ui)
{
    ui->parent_nil.v = &ui_nil_wig;
    ui->flags_nil.v = 0;

    ui->absolute_x_nil.v = 0;
    ui->absolute_y_nil.v = 0;

    ui->desired_width_nil.v = UI_Pixels(200.f, 1.f);
    ui->desired_height_nil.v = UI_Pixels(2.f, 1.f);

    ui->opacity_nil.v = 1.f;
    ui->color_text_nil.v = Vec4_f32{1, 1, 1, 1};
    ui->color_background_nil.v = Vec4_f32{0.3f, 0.3f, 0.3f, 1.f};
    ui->color_border_nil.v = Vec4_f32{1, 1, 1, 0.3f};

    ui->regionf32_nil.v = {};

    ui->font_nil.v = Font_GetTag(ui_font_hash);
    ui->font_size_nil.v = 16.f;

    ui->layout_direction_nil.v = Axis2_X;

    ui->text_alignment_nil.v = UI_Align_Left;
    ui->text_padding_nil.v = 0.f;

    ui->seed_key_nil.v = UI_ZeroKey();
}

function void
UI_ProcessDefaultUIStateStacks(UI_Ctx* ui)
{
    #define PopStack(name, func) if (ui->name.pop_default) { func(); ui->name.pop_default = 0; }
    PopStack(parent_stack, UI_PopParent)
    PopStack(flags_stack, UI_PopFlags)

    PopStack(absolute_x_stack, UI_PopAbsoluteX)
    PopStack(absolute_y_stack, UI_PopAbsoluteY)

    PopStack(desired_width_stack, UI_PopDesiredWidth)
    PopStack(desired_height_stack, UI_PopDesiredHeight)

    PopStack(color_text_stack, UI_PopColorText)
    PopStack(color_background_stack, UI_PopColorBackground)
    PopStack(color_border_stack, UI_PopColorBorder)

    PopStack(regionf32_stack, UI_PopRegionf32)

    PopStack(font_stack, UI_PopFont)
    PopStack(font_size_stack, UI_PopFontSize)

    PopStack(layout_direction_stack, UI_PopLayoutDirection)

    PopStack(text_alignment_stack, UI_PopTextAlignment)
    PopStack(text_padding_stack, UI_PopTextPadding)

    PopStack(seed_key_stack, UI_PopSeedKey)
    #undef PopStack
}

function void
UI_SetNextDesiredSize(Axis2 axis, UI_Size v)
{
    (axis == Axis2_X ? UI_SetNextDesiredWidth : UI_SetNextDesiredHeight)(v);
}

//

function UI_Wig* UI_GetParent()
{
    return ui_ctx->parent_stack.top->v;
}

function UI_WigFlags UI_GetFlags()
{
    return ui_ctx->flags_stack.top->v;
}

function f32 UI_GetAbsoluteX()
{
    return ui_ctx->absolute_x_stack.top->v;
}

function f32 UI_GetAbsoluteY()
{
    return ui_ctx->absolute_y_stack.top->v;
}

function UI_Size UI_GetDesiredWidth()
{
    return ui_ctx->desired_width_stack.top->v;
}

function UI_Size UI_GetDesiredHeight()
{
    return ui_ctx->desired_height_stack.top->v;
}

function Vec4_f32 UI_GetColorText()
{
    return ui_ctx->color_text_stack.top->v;
}

function Vec4_f32 UI_GetColorBackground()
{
    return ui_ctx->color_background_stack.top->v;
}

function Vec4_f32 UI_GetColorBorder()
{
    return ui_ctx->color_border_stack.top->v;
}

function Render_RegionTex2D_f32 UI_GetRegionf32()
{
    return ui_ctx->regionf32_stack.top->v;
}

function Font_Tag UI_GetFont()
{
    return ui_ctx->font_stack.top->v;
}

function f32 UI_GetFontSize()
{
    return ui_ctx->font_size_stack.top->v;
}

function Axis2 UI_GetLayoutDirection()
{
    return ui_ctx->layout_direction_stack.top->v;
}

function UI_Align UI_GetTextAlignment()
{
    return ui_ctx->text_alignment_stack.top->v;
}

function f32 UI_GetTextPadding()
{
    return ui_ctx->text_padding_stack.top->v;
}

function UI_Key UI_GetSeedKey()
{
    return ui_ctx->seed_key_stack.top->v;
}

function UI_Wig*
UI_SetParent(UI_Wig* v)
{
    UI_ParentNode* node = ui_ctx->parent_stack.free;
    if (node)
    {
        StackPop(ui_ctx->parent_stack.free);
    }
    else
    {
        node = (UI_ParentNode* )ArenaPush((UI_Arena()), sizeof(UI_ParentNode));
    }

    UI_Wig* old_value = ui_ctx->parent_stack.top->v;
    node->v = v;
    node->next = ui_ctx->parent_stack.top;
    ui_ctx->parent_stack.top = node;
    ui_ctx->parent_stack.pop_default = 0;
    
    return old_value;
}

function UI_WigFlags 
UI_SetFlags(UI_WigFlags v)
{
    UI_FlagsNode* node = ui_ctx->flags_stack.free;
    if (node)
    {
        StackPop(ui_ctx->flags_stack.free);
    }
    else
    {
        node = (UI_FlagsNode* )ArenaPush((UI_Arena()), sizeof(UI_FlagsNode));
    }

    UI_WigFlags old_value = ui_ctx->flags_stack.top->v;
    node->v = v;
    node->next = ui_ctx->flags_stack.top;
    ui_ctx->flags_stack.top = node;
    ui_ctx->flags_stack.pop_default = 0;
    
    return old_value;
}

function f32 
UI_SetAbsoluteX(f32 v)
{
    UI_AbsoluteXNode* node = ui_ctx->absolute_x_stack.free;
    if (node)
    {
        StackPop(ui_ctx->absolute_x_stack.free);
    }
    else
    {
        node = (UI_AbsoluteXNode* )ArenaPush((UI_Arena()), sizeof(UI_AbsoluteXNode));
    }

    f32 old_value = ui_ctx->absolute_x_stack.top->v;
    node->v = v;
    node->next = ui_ctx->absolute_x_stack.top;
    ui_ctx->absolute_x_stack.top = node;
    ui_ctx->absolute_x_stack.pop_default = 0;
    
    return old_value;
}

function f32 
UI_SetAbsoluteY(f32 v)
{
    UI_AbsoluteYNode* node = ui_ctx->absolute_y_stack.free;
    if (node)
    {
        StackPop(ui_ctx->absolute_y_stack.free);
    }
    else
    {
        node = (UI_AbsoluteYNode* )ArenaPush((UI_Arena()), sizeof(UI_AbsoluteYNode));
    }

    f32 old_value = ui_ctx->absolute_y_stack.top->v;
    node->v = v;
    node->next = ui_ctx->absolute_y_stack.top;
    ui_ctx->absolute_y_stack.top = node;
    ui_ctx->absolute_y_stack.pop_default = 0;
    
    return old_value;
}

function UI_Size 
UI_SetDesiredWidth(UI_Size v)
{
    UI_DesiredWidthNode* node = ui_ctx->desired_width_stack.free;
    if (node)
    {
        StackPop(ui_ctx->desired_width_stack.free);
    }
    else
    {
        node = (UI_DesiredWidthNode* )ArenaPush((UI_Arena()), sizeof(UI_DesiredWidthNode));
    }

    UI_Size old_value = ui_ctx->desired_width_stack.top->v;
    node->v = v;
    node->next = ui_ctx->desired_width_stack.top;
    ui_ctx->desired_width_stack.top = node;
    ui_ctx->desired_width_stack.pop_default = 0;
    
    return old_value;
}

function UI_Size 
UI_SetDesiredHeight(UI_Size v)
{
    UI_DesiredHeightNode* node = ui_ctx->desired_height_stack.free;
    if (node)
    {
        StackPop(ui_ctx->desired_height_stack.free);
    }
    else
    {
        node = (UI_DesiredHeightNode* )ArenaPush((UI_Arena()), sizeof(UI_DesiredHeightNode));
    }

    UI_Size old_value = ui_ctx->desired_height_stack.top->v;
    node->v = v;
    node->next = ui_ctx->desired_height_stack.top;
    ui_ctx->desired_height_stack.top = node;
    ui_ctx->desired_height_stack.pop_default = 0;
    
    return old_value;
}

function Vec4_f32 
UI_SetColorText(Vec4_f32 v)
{
    UI_ColorTextNode* node = ui_ctx->color_text_stack.free;
    if (node)
    {
        StackPop(ui_ctx->color_text_stack.free);
    }
    else
    {
        node = (UI_ColorTextNode* )ArenaPush((UI_Arena()), sizeof(UI_ColorTextNode));
    }

    Vec4_f32 old_value = ui_ctx->color_text_stack.top->v;
    node->v = v;
    node->next = ui_ctx->color_text_stack.top;
    ui_ctx->color_text_stack.top = node;
    ui_ctx->color_text_stack.pop_default = 0;
    
    return old_value;
}

function Vec4_f32 
UI_SetColorBackground(Vec4_f32 v)
{
    UI_ColorBackgroundNode* node = ui_ctx->color_background_stack.free;
    if (node)
    {
        StackPop(ui_ctx->color_background_stack.free);
    }
    else
    {
        node = (UI_ColorBackgroundNode* )ArenaPush((UI_Arena()), sizeof(UI_ColorBackgroundNode));
    }

    Vec4_f32 old_value = ui_ctx->color_background_stack.top->v;
    node->v = v;
    node->next = ui_ctx->color_background_stack.top;
    ui_ctx->color_background_stack.top = node;
    ui_ctx->color_background_stack.pop_default = 0;
    
    return old_value;
}

function Vec4_f32 
UI_SetColorBorder(Vec4_f32 v)
{
    UI_ColorBorderNode* node = ui_ctx->color_border_stack.free;
    if (node)
    {
        StackPop(ui_ctx->color_border_stack.free);
    }
    else
    {
        node = (UI_ColorBorderNode* )ArenaPush((UI_Arena()), sizeof(UI_ColorBorderNode));
    }

    Vec4_f32 old_value = ui_ctx->color_border_stack.top->v;
    node->v = v;
    node->next = ui_ctx->color_border_stack.top;
    ui_ctx->color_border_stack.top = node;
    ui_ctx->color_border_stack.pop_default = 0;
    
    return old_value;
}

function Render_RegionTex2D_f32 
UI_SetRegionf32(Render_RegionTex2D_f32 v)
{
    UI_Regionf32Node* node = ui_ctx->regionf32_stack.free;
    if (node)
    {
        StackPop(ui_ctx->regionf32_stack.free);
    }
    else
    {
        node = (UI_Regionf32Node* )ArenaPush((UI_Arena()), sizeof(UI_Regionf32Node));
    }

    Render_RegionTex2D_f32 old_value = ui_ctx->regionf32_stack.top->v;
    node->v = v;
    node->next = ui_ctx->regionf32_stack.top;
    ui_ctx->regionf32_stack.top = node;
    ui_ctx->regionf32_stack.pop_default = 0;
    
    return old_value;
}

function Font_Tag 
UI_SetFont(Font_Tag v)
{
    UI_FontNode* node = ui_ctx->font_stack.free;
    if (node)
    {
        StackPop(ui_ctx->font_stack.free);
    }
    else
    {
        node = (UI_FontNode* )ArenaPush((UI_Arena()), sizeof(UI_FontNode));
    }

    Font_Tag old_value = ui_ctx->font_stack.top->v;
    node->v = v;
    node->next = ui_ctx->font_stack.top;
    ui_ctx->font_stack.top = node;
    ui_ctx->font_stack.pop_default = 0;
    
    return old_value;
}

function f32 
UI_SetFontSize(f32 v)
{
    UI_FontSizeNode* node = ui_ctx->font_size_stack.free;
    if (node)
    {
        StackPop(ui_ctx->font_size_stack.free);
    }
    else
    {
        node = (UI_FontSizeNode* )ArenaPush((UI_Arena()), sizeof(UI_FontSizeNode));
    }

    f32 old_value = ui_ctx->font_size_stack.top->v;
    node->v = v;
    node->next = ui_ctx->font_size_stack.top;
    ui_ctx->font_size_stack.top = node;
    ui_ctx->font_size_stack.pop_default = 0;
    
    return old_value;
}

function Axis2 
UI_SetLayoutDirection(Axis2 v)
{
    UI_LayoutDirectionNode* node = ui_ctx->layout_direction_stack.free;
    if (node)
    {
        StackPop(ui_ctx->layout_direction_stack.free);
    }
    else
    {
        node = (UI_LayoutDirectionNode* )ArenaPush((UI_Arena()), sizeof(UI_LayoutDirectionNode));
    }

    Axis2 old_value = ui_ctx->layout_direction_stack.top->v;
    node->v = v;
    node->next = ui_ctx->layout_direction_stack.top;
    ui_ctx->layout_direction_stack.top = node;
    ui_ctx->layout_direction_stack.pop_default = 0;
    
    return old_value;
}

function UI_Align 
UI_SetTextAlignment(UI_Align v)
{
    UI_TextAlignmentNode* node = ui_ctx->text_alignment_stack.free;
    if (node)
    {
        StackPop(ui_ctx->text_alignment_stack.free);
    }
    else
    {
        node = (UI_TextAlignmentNode* )ArenaPush((UI_Arena()), sizeof(UI_TextAlignmentNode));
    }

    UI_Align old_value = ui_ctx->text_alignment_stack.top->v;
    node->v = v;
    node->next = ui_ctx->text_alignment_stack.top;
    ui_ctx->text_alignment_stack.top = node;
    ui_ctx->text_alignment_stack.pop_default = 0;
    
    return old_value;
}

function f32 
UI_SetTextPadding(f32 v)
{
    UI_TextPaddingNode* node = ui_ctx->text_padding_stack.free;
    if (node)
    {
        StackPop(ui_ctx->text_padding_stack.free);
    }
    else
    {
        node = (UI_TextPaddingNode* )ArenaPush((UI_Arena()), sizeof(UI_TextPaddingNode));
    }

    f32 old_value = ui_ctx->text_padding_stack.top->v;
    node->v = v;
    node->next = ui_ctx->text_padding_stack.top;
    ui_ctx->text_padding_stack.top = node;
    ui_ctx->text_padding_stack.pop_default = 0;
    
    return old_value;
}

function UI_Key 
UI_SetSeedKey(UI_Key v)
{
    UI_SeedKeyNode* node = ui_ctx->seed_key_stack.free;
    if (node)
    {
        StackPop(ui_ctx->seed_key_stack.free);
    }
    else
    {
        node = (UI_SeedKeyNode* )ArenaPush((UI_Arena()), sizeof(UI_SeedKeyNode));
    }

    UI_Key old_value = ui_ctx->seed_key_stack.top->v;
    node->v = v;
    node->next = ui_ctx->seed_key_stack.top;
    ui_ctx->seed_key_stack.top = node;
    ui_ctx->seed_key_stack.pop_default = 0;
    
    return old_value;
}

function UI_Wig*
UI_PopParent()
{
    UI_ParentNode* popped = ui_ctx->parent_stack.top;
    if (popped != &ui_ctx->parent_nil)
    {
        StackPop(ui_ctx->parent_stack.top);
        StackPush(ui_ctx->parent_stack.free, popped);
    }

    return popped->v;
}

function UI_WigFlags 
UI_PopFlags()
{
    UI_FlagsNode* popped = ui_ctx->flags_stack.top;
    if (popped != &ui_ctx->flags_nil)
    {
        StackPop(ui_ctx->flags_stack.top);
        StackPush(ui_ctx->flags_stack.free, popped);
    }

    return popped->v;
}

function f32 
UI_PopAbsoluteX()
{
    UI_AbsoluteXNode* popped = ui_ctx->absolute_x_stack.top;
    if (popped != &ui_ctx->absolute_x_nil)
    {
        StackPop(ui_ctx->absolute_x_stack.top);
        StackPush(ui_ctx->absolute_x_stack.free, popped);
    }

    return popped->v;
}

function f32 
UI_PopAbsoluteY()
{
    UI_AbsoluteYNode* popped = ui_ctx->absolute_y_stack.top;
    if (popped != &ui_ctx->absolute_y_nil)
    {
        StackPop(ui_ctx->absolute_y_stack.top);
        StackPush(ui_ctx->absolute_y_stack.free, popped);
    }

    return popped->v;
}

function UI_Size 
UI_PopDesiredWidth()
{
    UI_DesiredWidthNode* popped = ui_ctx->desired_width_stack.top;
    if (popped != &ui_ctx->desired_width_nil)
    {
        StackPop(ui_ctx->desired_width_stack.top);
        StackPush(ui_ctx->desired_width_stack.free, popped);
    }

    return popped->v;
}

function UI_Size 
UI_PopDesiredHeight()
{
    UI_DesiredHeightNode* popped = ui_ctx->desired_height_stack.top;
    if (popped != &ui_ctx->desired_height_nil)
    {
        StackPop(ui_ctx->desired_height_stack.top);
        StackPush(ui_ctx->desired_height_stack.free, popped);
    }

    return popped->v;
}

function Vec4_f32 
UI_PopColorText()
{
    UI_ColorTextNode* popped = ui_ctx->color_text_stack.top;
    if (popped != &ui_ctx->color_text_nil)
    {
        StackPop(ui_ctx->color_text_stack.top);
        StackPush(ui_ctx->color_text_stack.free, popped);
    }

    return popped->v;
}

function Vec4_f32 
UI_PopColorBackground()
{
    UI_ColorBackgroundNode* popped = ui_ctx->color_background_stack.top;
    if (popped != &ui_ctx->color_background_nil)
    {
        StackPop(ui_ctx->color_background_stack.top);
        StackPush(ui_ctx->color_background_stack.free, popped);
    }

    return popped->v;
}

function Vec4_f32 
UI_PopColorBorder()
{
    UI_ColorBorderNode* popped = ui_ctx->color_border_stack.top;
    if (popped != &ui_ctx->color_border_nil)
    {
        StackPop(ui_ctx->color_border_stack.top);
        StackPush(ui_ctx->color_border_stack.free, popped);
    }

    return popped->v;
}

function Render_RegionTex2D_f32 
UI_PopRegionf32()
{
    UI_Regionf32Node* popped = ui_ctx->regionf32_stack.top;
    if (popped != &ui_ctx->regionf32_nil)
    {
        StackPop(ui_ctx->regionf32_stack.top);
        StackPush(ui_ctx->regionf32_stack.free, popped);
    }

    return popped->v;
}

function Font_Tag 
UI_PopFont()
{
    UI_FontNode* popped = ui_ctx->font_stack.top;
    if (popped != &ui_ctx->font_nil)
    {
        StackPop(ui_ctx->font_stack.top);
        StackPush(ui_ctx->font_stack.free, popped);
    }

    return popped->v;
}

function f32 
UI_PopFontSize()
{
    UI_FontSizeNode* popped = ui_ctx->font_size_stack.top;
    if (popped != &ui_ctx->font_size_nil)
    {
        StackPop(ui_ctx->font_size_stack.top);
        StackPush(ui_ctx->font_size_stack.free, popped);
    }

    return popped->v;
}

function Axis2 
UI_PopLayoutDirection()
{
    UI_LayoutDirectionNode* popped = ui_ctx->layout_direction_stack.top;
    if (popped != &ui_ctx->layout_direction_nil)
    {
        StackPop(ui_ctx->layout_direction_stack.top);
        StackPush(ui_ctx->layout_direction_stack.free, popped);
    }

    return popped->v;
}

function UI_Align 
UI_PopTextAlignment()
{
    UI_TextAlignmentNode* popped = ui_ctx->text_alignment_stack.top;
    if (popped != &ui_ctx->text_alignment_nil)
    {
        StackPop(ui_ctx->text_alignment_stack.top);
        StackPush(ui_ctx->text_alignment_stack.free, popped);
    }

    return popped->v;
}

function f32 
UI_PopTextPadding()
{
    UI_TextPaddingNode* popped = ui_ctx->text_padding_stack.top;
    if (popped != &ui_ctx->text_padding_nil)
    {
        StackPop(ui_ctx->text_padding_stack.top);
        StackPush(ui_ctx->text_padding_stack.free, popped);
    }

    return popped->v;
}

function UI_Key 
UI_PopSeedKey()
{
    UI_SeedKeyNode* popped = ui_ctx->seed_key_stack.top;
    if (popped != &ui_ctx->seed_key_nil)
    {
        StackPop(ui_ctx->seed_key_stack.top);
        StackPush(ui_ctx->seed_key_stack.free, popped);
    }

    return popped->v;
}

function UI_Wig*
UI_SetNextParent(UI_Wig* v)
{
    UI_ParentNode* node = ui_ctx->parent_stack.free;
    if (node)
    {
        StackPop(ui_ctx->parent_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_ParentNode, 1);
    }
    
    UI_Wig* old_value = ui_ctx->parent_stack.top->v;
    node->v = v;
    node->next = ui_ctx->parent_stack.top;
    ui_ctx->parent_stack.top = node;
    ui_ctx->parent_stack.pop_default = 1;
    
    return old_value;
}

function UI_WigFlags 
UI_SetNextFlags(UI_WigFlags v)
{
    UI_FlagsNode* node = ui_ctx->flags_stack.free;
    if (node)
    {
        StackPop(ui_ctx->flags_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_FlagsNode, 1);
    }
    
    UI_WigFlags old_value = ui_ctx->flags_stack.top->v;
    node->v = v;
    node->next = ui_ctx->flags_stack.top;
    ui_ctx->flags_stack.top = node;
    ui_ctx->flags_stack.pop_default = 1;
    
    return old_value;
}

function f32 
UI_SetNextAbsoluteX(f32 v)
{
    UI_AbsoluteXNode* node = ui_ctx->absolute_x_stack.free;
    if (node)
    {
        StackPop(ui_ctx->absolute_x_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_AbsoluteXNode, 1);
    }
    
    f32 old_value = ui_ctx->absolute_x_stack.top->v;
    node->v = v;
    node->next = ui_ctx->absolute_x_stack.top;
    ui_ctx->absolute_x_stack.top = node;
    ui_ctx->absolute_x_stack.pop_default = 1;
    
    return old_value;
}

function f32 
UI_SetNextAbsoluteY(f32 v)
{
    UI_AbsoluteYNode* node = ui_ctx->absolute_y_stack.free;
    if (node)
    {
        StackPop(ui_ctx->absolute_y_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_AbsoluteYNode, 1);
    }
    
    f32 old_value = ui_ctx->absolute_y_stack.top->v;
    node->v = v;
    node->next = ui_ctx->absolute_y_stack.top;
    ui_ctx->absolute_y_stack.top = node;
    ui_ctx->absolute_y_stack.pop_default = 1;
    
    return old_value;
}

function UI_Size 
UI_SetNextDesiredWidth(UI_Size v)
{
    UI_DesiredWidthNode* node = ui_ctx->desired_width_stack.free;
    if (node)
    {
        StackPop(ui_ctx->desired_width_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_DesiredWidthNode, 1);
    }
    
    UI_Size old_value = ui_ctx->desired_width_stack.top->v;
    node->v = v;
    node->next = ui_ctx->desired_width_stack.top;
    ui_ctx->desired_width_stack.top = node;
    ui_ctx->desired_width_stack.pop_default = 1;
    
    return old_value;
}

function UI_Size 
UI_SetNextDesiredHeight(UI_Size v)
{
    UI_DesiredHeightNode* node = ui_ctx->desired_height_stack.free;
    if (node)
    {
        StackPop(ui_ctx->desired_height_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_DesiredHeightNode, 1);
    }
    
    UI_Size old_value = ui_ctx->desired_height_stack.top->v;
    node->v = v;
    node->next = ui_ctx->desired_height_stack.top;
    ui_ctx->desired_height_stack.top = node;
    ui_ctx->desired_height_stack.pop_default = 1;
    
    return old_value;
}

function Vec4_f32 
UI_SetNextColorText(Vec4_f32 v)
{
    UI_ColorTextNode* node = ui_ctx->color_text_stack.free;
    if (node)
    {
        StackPop(ui_ctx->color_text_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_ColorTextNode, 1);
    }
    
    Vec4_f32 old_value = ui_ctx->color_text_stack.top->v;
    node->v = v;
    node->next = ui_ctx->color_text_stack.top;
    ui_ctx->color_text_stack.top = node;
    ui_ctx->color_text_stack.pop_default = 1;
    
    return old_value;
}

function Vec4_f32 
UI_SetNextColorBackground(Vec4_f32 v)
{
    UI_ColorBackgroundNode* node = ui_ctx->color_background_stack.free;
    if (node)
    {
        StackPop(ui_ctx->color_background_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_ColorBackgroundNode, 1);
    }
    
    Vec4_f32 old_value = ui_ctx->color_background_stack.top->v;
    node->v = v;
    node->next = ui_ctx->color_background_stack.top;
    ui_ctx->color_background_stack.top = node;
    ui_ctx->color_background_stack.pop_default = 1;
    
    return old_value;
}

function Vec4_f32 
UI_SetNextColorBorder(Vec4_f32 v)
{
    UI_ColorBorderNode* node = ui_ctx->color_border_stack.free;
    if (node)
    {
        StackPop(ui_ctx->color_border_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_ColorBorderNode, 1);
    }
    
    Vec4_f32 old_value = ui_ctx->color_border_stack.top->v;
    node->v = v;
    node->next = ui_ctx->color_border_stack.top;
    ui_ctx->color_border_stack.top = node;
    ui_ctx->color_border_stack.pop_default = 1;
    
    return old_value;
}

function Render_RegionTex2D_f32 
UI_SetNextRegionf32(Render_RegionTex2D_f32 v)
{
    UI_Regionf32Node* node = ui_ctx->regionf32_stack.free;
    if (node)
    {
        StackPop(ui_ctx->regionf32_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_Regionf32Node, 1);
    }
    
    Render_RegionTex2D_f32 old_value = ui_ctx->regionf32_stack.top->v;
    node->v = v;
    node->next = ui_ctx->regionf32_stack.top;
    ui_ctx->regionf32_stack.top = node;
    ui_ctx->regionf32_stack.pop_default = 1;
    
    return old_value;
}

function Font_Tag 
UI_SetNextFont(Font_Tag v)
{
    UI_FontNode* node = ui_ctx->font_stack.free;
    if (node)
    {
        StackPop(ui_ctx->font_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_FontNode, 1);
    }
    
    Font_Tag old_value = ui_ctx->font_stack.top->v;
    node->v = v;
    node->next = ui_ctx->font_stack.top;
    ui_ctx->font_stack.top = node;
    ui_ctx->font_stack.pop_default = 1;
    
    return old_value;
}

function f32 
UI_SetNextFontSize(f32 v)
{
    UI_FontSizeNode* node = ui_ctx->font_size_stack.free;
    if (node)
    {
        StackPop(ui_ctx->font_size_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_FontSizeNode, 1);
    }
    
    f32 old_value = ui_ctx->font_size_stack.top->v;
    node->v = v;
    node->next = ui_ctx->font_size_stack.top;
    ui_ctx->font_size_stack.top = node;
    ui_ctx->font_size_stack.pop_default = 1;
    
    return old_value;
}

function Axis2 
UI_SetNextLayoutDirection(Axis2 v)
{
    UI_LayoutDirectionNode* node = ui_ctx->layout_direction_stack.free;
    if (node)
    {
        StackPop(ui_ctx->layout_direction_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_LayoutDirectionNode, 1);
    }
    
    Axis2 old_value = ui_ctx->layout_direction_stack.top->v;
    node->v = v;
    node->next = ui_ctx->layout_direction_stack.top;
    ui_ctx->layout_direction_stack.top = node;
    ui_ctx->layout_direction_stack.pop_default = 1;
    
    return old_value;
}

function UI_Align 
UI_SetNextTextAlignment(UI_Align v)
{
    UI_TextAlignmentNode* node = ui_ctx->text_alignment_stack.free;
    if (node)
    {
        StackPop(ui_ctx->text_alignment_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_TextAlignmentNode, 1);
    }
    
    UI_Align old_value = ui_ctx->text_alignment_stack.top->v;
    node->v = v;
    node->next = ui_ctx->text_alignment_stack.top;
    ui_ctx->text_alignment_stack.top = node;
    ui_ctx->text_alignment_stack.pop_default = 1;
    
    return old_value;
}

function f32 
UI_SetNextTextPadding(f32 v)
{
    UI_TextPaddingNode* node = ui_ctx->text_padding_stack.free;
    if (node)
    {
        StackPop(ui_ctx->text_padding_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_TextPaddingNode, 1);
    }
    
    f32 old_value = ui_ctx->text_padding_stack.top->v;
    node->v = v;
    node->next = ui_ctx->text_padding_stack.top;
    ui_ctx->text_padding_stack.top = node;
    ui_ctx->text_padding_stack.pop_default = 1;
    
    return old_value;
}

function UI_Key 
UI_SetNextSeedKey(UI_Key v)
{
    UI_SeedKeyNode* node = ui_ctx->seed_key_stack.free;
    if (node)
    {
        StackPop(ui_ctx->seed_key_stack.free);
    }
    else
    {
        node = PushArray(UI_Arena(), UI_SeedKeyNode, 1);
    }
    
    UI_Key old_value = ui_ctx->seed_key_stack.top->v;
    node->v = v;
    node->next = ui_ctx->seed_key_stack.top;
    ui_ctx->seed_key_stack.top = node;
    ui_ctx->seed_key_stack.pop_default = 1;
    
    return old_value;
}
