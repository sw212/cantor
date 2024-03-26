static Font_State* font_state = 0;

function b32
Font_MatchTag(Font_Tag a, Font_Tag b)
{
    b32 result = (a.v[0] == b.v[0]) && (a.v[1] == b.v[1]);
    return result;
}

function Font_Tag
Font_GetTag(u128 hash)
{
    Font_Tag result = {hash.v[0], hash.v[1]};

    u64 slot = result.v[1] % font_state->tag_table_size;

    Font_TagNode* node = 0;
    for (Font_TagNode* n = font_state->tag_table[slot].first; n != 0; n = n->hash_next)
    {
        if (Font_MatchTag(n->tag, result))
        {
            node = n;
            break;
        }
    }

    if (!node)
    {
        node = PushArray(font_state->arena, Font_TagNode, 1);
        node->tag = result;
        node->handle = Font_Core_Open(hash);
        QueuePush_NZ(font_state->tag_table[slot].first, font_state->tag_table[slot].last, node, hash_next, CheckNull, SetNull);
    }

    return result;
}

function b32
Font_MatchHash(Font_Hash a, Font_Hash b)
{
    b32 result = (a.v[0] == b.v[0]) && (a.v[1] == b.v[1]);
    return result;
}

function Font_Hash
Font_GetHash(Font_Tag tag, f32 size, Str8 string)
{
    union {f32 f; u64 u;} size_u = {};
    size_u.f = size;
    u64 buffer[] = { tag.v[0], tag.v[1], size_u.u };

    Font_Hash result = {};
    {
        meow_u128 font_hash   = MeowHash(MeowDefaultSeed, sizeof(buffer), buffer);
        meow_u128 string_hash = MeowHash(MeowDefaultSeed, string.size, string.str);
        MemoryCopy(&result, &font_hash, sizeof(result));
        result.v[0] = MeowU64From(font_hash, 0) ^ MeowU64From(string_hash, 1);
        result.v[1] = MeowU64From(font_hash, 1) ^ MeowU64From(string_hash, 0);
    }

    return result;
}

function Font_Core_Hnd
Font_GetHandle(Font_Tag tag)
{
    Font_Core_Hnd result = {};
    u64 slot = tag.v[1] % font_state->tag_table_size;

    Font_TagNode* node = 0;
    for (Font_TagNode* n = font_state->tag_table[slot].first; n != 0; n = n->hash_next)
    {
        if (Font_MatchTag(n->tag, tag))
        {
            node = n;
            break;
        }
    }

    if (node)
    {
        result = node->handle;
    }

    return result;
}

function Font_Metrics
Font_GetMetrics(Font_Tag tag, f32 size)
{
    Font_Metrics result = {};
    Font_Core_Hnd handle = Font_GetHandle(tag);
    Font_Core_Metrics metrics = Font_Core_GetMetrics(handle, size);
    {
        result.line_adv       = metrics.line_adv;
        result.ascent         = metrics.ascent;
        result.descent        = metrics.descent;
        result.capital_height = metrics.capital_height;
    }

    return result;
}

function Font_Run
Font_GetRun(Arena* arena, Font_Tag font, f32 size, Str8 string)
{
    Scratch scratch = ScratchBegin(&arena, 1);
    Font_Run run = {};
    Font_Metrics metrics = Font_GetMetrics(font, size);
    size = (f32)i32(size);

    u64 prev_offset = 0;

    for (u64 offset = 0; offset < string.size;)
    {
        offset++;

        if (offset > prev_offset)
        {
            //
            // (off - cache_start_off) is the size of the next "chunk" to raster at string.str + cache_start_off
            //
            Str8 hash_portion = String8(string.str + prev_offset, offset - prev_offset);
            prev_offset = offset;

            Font_Hash hash = Font_GetHash(font, size, hash_portion);
            u64       slot = hash.v[1] % font_state->cache_table_size;

            Font_CacheNode* node = 0;

            for (Font_CacheNode* n = font_state->cache_table[slot].first; n != 0; n = n->next)
            {
                if (Font_MatchHash(n->hash, hash))
                {
                    node = n;
                    break;
                }
            }

            if (!node)
            {
                Font_Core_Hnd font_handle = Font_GetHandle(font);
                Font_Atlas* atlas = &font_state->atlas;

                Font_Core_RasterResult raster = Font_Core_Raster(scratch.arena, font_handle, size, hash_portion);
                Vec2_i64 raster_dims = raster.atlas_dim;
                
                Rect2_i64 atlas_region = AtlasAllocate(font_state->arena, font_state->atlas.allocator, raster_dims);
                Rect2_i64 fill_rect = {atlas_region.min, {atlas_region.min.x + raster.atlas_dim.x, atlas_region.min.y + raster.atlas_dim.y}};
                Render_FillRegionTex2D(atlas->texture, fill_rect, raster.atlas);
                
                // push node
                {
                    node = PushArray(font_state->arena, Font_CacheNode, 1);
                    node->hash = hash;
                    node->alloc_region = atlas_region;
                    node->advance = raster.advance;
                    node->height = raster.height;
                    DLLPushBack_NPZ(font_state->cache_table[slot].first,
                                    font_state->cache_table[slot].last,
                                    node, next, prev, CheckNull, SetNull);
                }
            }

            if(node)
            {
                Vec2_f32 bl = {f32(node->alloc_region.min.x), f32(node->alloc_region.min.y)};
                Vec2_f32 tr = {f32(node->alloc_region.max.x), bl.y + node->height};

                Font_Piece* piece = PushArray(arena, Font_Piece, 1);
                QueuePush(run.first, run.last, piece);

                piece->texture     = font_state->atlas.texture;
                piece->src_rect    = {bl, tr};
                piece->advance     = node->advance;
                piece->decode_size = u32(hash_portion.size);
                piece->offset.y    = -metrics.ascent;

                run.piece_count++;
                run.advance += node->advance;
            }

        }
    }
    ScratchEnd(scratch);

    return run;
}

function f32
Font_GetAdvance(Font_Tag font, f32 size, Str8 string)
{
    Scratch scratch = ScratchBegin(0, 0);
    Font_Run run = Font_GetRun(scratch.arena, font, size, string);
    ScratchEnd(scratch);

    f32 result = run.advance;
    return result;
}

function u64
Font_GetByteOffset(Font_Tag font, f32 size, Str8 string, f32 advance)
{
    u64 result = 0;
    Scratch scratch = ScratchBegin(0, 0);
    Font_Run run = Font_GetRun(scratch.arena, font, size, string);

    u64 bytes_at  = 0;
    f32 pixels_at = 0;

    for(Font_Piece* piece = run.first; piece != 0; piece = piece->next)
    {
        if(advance < (pixels_at + 0.5f * piece->advance))
        {
            result = bytes_at;
            break;
        }

        if(advance >= (pixels_at + 0.5f * piece->advance))
        {
            result = bytes_at + piece->decode_size;
        }

        pixels_at += piece->advance;
        bytes_at  += piece->decode_size;
    }

    ScratchEnd(scratch);
    return result;
}

function Str8
Font_GetTruncatedString(Font_Tag font, f32 size, Str8 string, f32 max_advance, f32 trail_advance)
{
    // calculate truncated size
    u64 truncated_size = string.size;
    Scratch scratch = ScratchBegin(0, 0);
    {
        Font_Run run = Font_GetRun(scratch.arena, font, size, string);
        u64 bytes_at = 0;
        f32 pixels_at = 0;
        u64 string_size = 0;

        for(Font_Piece* piece = run.first; piece != 0; piece = piece->next)
        {
            // can we fit the font run so far with a trailer ?
            if((pixels_at + trail_advance) <= max_advance)
            {
                string_size = bytes_at;
            }

            // does adding the next piece cause us to overflow?
            if((pixels_at + piece->advance) > max_advance)
            {
                // if so, set string size to last fitting (font run + trailer)
                truncated_size = string_size;
                break;
            }

            pixels_at += piece->advance;
            bytes_at  += piece->decode_size;
        }
    }
    ScratchEnd(scratch);

    Str8 result = StrPrefix(string, truncated_size);
    return result;
}

function Font_AtlasList
Font_PushAtlasList(Arena* arena)
{
    Font_AtlasList list = {};
    Font_AtlasNode* node = PushArray(arena, Font_AtlasNode, 1);
    node->atlas = font_state->atlas;
    QueuePush(list.first, list.last, node);
    list.count++;

    return list;
}

function void
Font_Initialise(Vec2_i64 glyph_atlas_size)
{
    if (IsCurrentThreadMain() && !font_state)
    {
        Arena* arena = ArenaAlloc(Gigabytes(64));
        font_state = PushArray(arena, Font_State, 1);
        font_state->arena = arena;

        font_state->tag_table_size = 1024;
        font_state->tag_table = PushArray(arena, Font_TagSlot, font_state->tag_table_size);

        font_state->cache_table_size = 1024;
        font_state->cache_table = PushArray(arena, Font_CacheSlot, font_state->cache_table_size);

        font_state->atlas.allocator = AtlasInitialise(arena, glyph_atlas_size);
        font_state->atlas.texture = Render_AllocTex2D(glyph_atlas_size, Render_Tex2D_RGBA8, Render_Tex2DUsage_Dynamic, 0);
    }
}

