function Render_Hnd
Render_HndZero()
{
    Render_Hnd result = {};
    return result;
}

function b32
Render_MatchHnd(Render_Hnd a, Render_Hnd b)
{
    return (a.v_64[0] == b.v_64[0] && a.v_64[1] == b.v_64[1] && a.v_64[2] == b.v_64[2] && a.v_64[3] == b.v_64[3]);
}

function b32
Render_HndIsZero(Render_Hnd handle)
{
    b32 result = Render_MatchHnd(handle, Render_HndZero());
    return result;
}

function u64
Render_BytesPerPixel(Render_Tex2DType format)
{
    u64 result = 0;

    switch (format)
    {
        case Render_Tex2D_R8:    { result = 1; } break;
        case Render_Tex2D_RGBA8: { result = 4; } break;
        
        default: break;
    }

    return result;
}

function Render_Pass*
Render_PushPassList(Arena* arena, Render_PassList* list, Render_PassType pass_type)
{
    Render_PassNode* node = PushArray(arena, Render_PassNode, 1);
    QueuePush(list->first, list->last, node);
    list->count++;
    
    Render_Pass* pass = &node->pass;
    pass->pass_type = pass_type;

    if (pass_type == Render_PassType_UI)
    {
        pass->ui = PushArray(arena, Render_Pass_UI, 1);
    }

    return pass;
}

function void*
Render_PushGroupList(Arena* arena, Render_GroupList* list, u64 max_instance_count)
{
    void* result = 0;

    //
    // Either:
    // - no group -> create group
    // - group full & can grow -> try to grow group if it'd be contiguous on the same arena
    // - group full & cannot grow -> create new group
    //

    Render_Group* group = list->last;

    if (!group || (group->size + list->instance_size >= group->cap))
    {
        group = PushArray(arena, Render_Group, 1);
        group->cap = max_instance_count * list->instance_size;
        group->v = PushArrayNZ(arena, u8, group->cap);
        QueuePush(list->first, list->last, group);
        list->count++;
    }

    result = &group->v[group->size];
    group->size += list->instance_size;
    list->size  += list->instance_size;

    return result;
}