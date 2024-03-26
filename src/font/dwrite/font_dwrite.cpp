static Font_Core_DWrite_State* font_dwrite_state = 0;

static Font_Core_DWrite_LoaderVT font_dwrite_loader_vt =
{
    Font_Core_DWrite_NOPQueryInterface,
    Font_Core_DWrite_NOPAddRef,
    Font_Core_DWrite_NOPRelease,
    Font_Core_DWrite_CreateStream,
};

static Font_Core_DWrite_StreamVT font_dwrite_stream_vt =
{
    Font_Core_DWrite_NOPQueryInterface,
    Font_Core_DWrite_AddRef,
    Font_Core_DWrite_Release,
    Font_Core_DWrite_ReadFileFragment,
    Font_Core_DWrite_ReleaseFileFragment,
    Font_Core_DWrite_GetFileSize,
    Font_Core_DWrite_GetLastWriteTime,
};

static Font_Core_DWrite_Loader font_dwrite_loader = {&font_dwrite_loader_vt};


// - 

function HRESULT
Font_Core_DWrite_NOPQueryInterface(void* this_ptr, REFIID riid, void** ppvObject)
{
    return E_NOINTERFACE;
}

function ULONG
Font_Core_DWrite_NOPAddRef(void* this_ptr)
{
    return 1;
}

function ULONG
Font_Core_DWrite_NOPRelease(void* this_ptr)
{
    return 1;
}


function HRESULT
Font_Core_DWrite_CreateStream(Font_Core_DWrite_Loader* this_ptr, void* key, u32 key_size, IDWriteFontFileStream** out_stream)
{
    HRESULT result = S_OK;
    {
        Assert(key_size == sizeof(u128));
        Font_Core_DWrite_Stream* stream = font_dwrite_state->free_stream;
        if (!stream)
        {
            stream = PushArrayNZ(font_dwrite_state->arena, Font_Core_DWrite_Stream, 1);
        }
        else
        {
            StackPop(font_dwrite_state->free_stream);
        }

        MemoryZeroStruct(stream);
        stream->lpVtbl = &font_dwrite_stream_vt;
        stream->scope = Hash_ScopeOpen();
        MemoryCopy(&stream->hash, key, sizeof(u128));
        stream->ref_count = 1;
        *out_stream = (IDWriteFontFileStream *)stream;
    }

    return result;
}

function ULONG
Font_Core_DWrite_AddRef(Font_Core_DWrite_Stream* this_ptr)
{
    ULONG result = ULONG(++this_ptr->ref_count);
    return result;
}

function ULONG
Font_Core_DWrite_Release(Font_Core_DWrite_Stream* this_ptr)
{
    this_ptr->ref_count -= 1;

    if(!this_ptr->ref_count)
    {
        // TODO: close scope
        StackPush(font_dwrite_state->free_stream, this_ptr);
    }

    return ULONG(this_ptr->ref_count);
}

function HRESULT
Font_Core_DWrite_ReadFileFragment(Font_Core_DWrite_Stream* this_ptr, void** fragment_start, u64 off, u64 size, void** fragment_ctx_out)
{
    HRESULT result = S_OK;
    {
        Str8 data = Hash_DataFromHash(this_ptr->scope, this_ptr->hash);
        *fragment_start = data.str + off;
        *fragment_ctx_out = 0;
    }
    return result;
}

function void
Font_Core_DWrite_ReleaseFileFragment(Font_Core_DWrite_Stream* this_ptr, void* fragment_ctx)
{
    // noop
}

function HRESULT
Font_Core_DWrite_GetFileSize(Font_Core_DWrite_Stream* this_ptr, u64* out_file_size)
{
    HRESULT result = S_OK;
    {
        Str8 data = Hash_DataFromHash(this_ptr->scope, this_ptr->hash);
        *out_file_size = data.size;
    }

    return result;
}

function HRESULT
Font_Core_DWrite_GetLastWriteTime(Font_Core_DWrite_Stream* this_ptr, u64* out_time)
{
    HRESULT result = S_OK;
    *out_time = 0;

    return result;
}


function Font_Core_DWrite
Font_Core_DWrite_GetFont(Font_Core_Hnd handle)
{
    Font_Core_DWrite result = {};
    result.file = (IDWriteFontFile*)handle.v[0];
    result.face = (IDWriteFontFace*)handle.v[1];

    return result;
}

function Font_Core_Hnd
Font_Core_DWrite_GetHandle(Font_Core_DWrite font)
{
    Font_Core_Hnd result = {};
    result.v[0] = u64(font.file);
    result.v[1] = u64(font.face);

    return result;
}


// -

function void
Font_Core_Initialize()
{
    if(IsCurrentThreadMain() && font_dwrite_state == 0)
    {
        HRESULT hresult = 0;

        Arena* arena = ArenaAlloc(Gigabytes(4));
        font_dwrite_state = PushArray(arena, Font_Core_DWrite_State, 1);
        font_dwrite_state->arena = arena;

        hresult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory), (IUnknown**)&font_dwrite_state->factory);
        hresult = font_dwrite_state->factory->RegisterFontFileLoader((IDWriteFontFileLoader *)&font_dwrite_loader);
        hresult = font_dwrite_state->factory->CreateRenderingParams(&font_dwrite_state->default_render_params);

        // make dwrite rendering params
        {
            FLOAT gamma = font_dwrite_state->default_render_params->GetGamma();
            FLOAT enhanced_contrast = font_dwrite_state->default_render_params->GetEnhancedContrast();
            FLOAT clear_type_level = font_dwrite_state->default_render_params->GetClearTypeLevel();
            hresult = font_dwrite_state->factory->CreateCustomRenderingParams(gamma,
                                                                            enhanced_contrast,
                                                                            clear_type_level,
                                                                            DWRITE_PIXEL_GEOMETRY_FLAT,
                                                                            DWRITE_RENDERING_MODE_DEFAULT,
                                                                            &font_dwrite_state->render_params);
        }

        // setup dwrite/gdi interop
        hresult = font_dwrite_state->factory->GetGdiInterop(&font_dwrite_state->gdi_interop);
    }
}

function Font_Core_Hnd
Font_Core_Open(u128 hash)
{
    HRESULT hresult = 0;

    IDWriteFontFile* font_file = 0;
    IDWriteFontFace* font_face = 0;

    hresult = font_dwrite_state->factory->CreateCustomFontFileReference(&hash, sizeof(hash), (IDWriteFontFileLoader*)&font_dwrite_loader, &font_file);
    hresult = font_dwrite_state->factory->CreateFontFace(DWRITE_FONT_FACE_TYPE_UNKNOWN, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);

    Font_Core_DWrite font = {.file = font_file, .face = font_face};

    Font_Core_Hnd result = Font_Core_DWrite_GetHandle(font);
    return result;
}

function void
Font_Core_Close(Font_Core_Hnd handle)
{
    Font_Core_DWrite font = Font_Core_DWrite_GetFont(handle);
    font.face->Release();
    font.file->Release();
}

function Font_Core_Metrics
Font_Core_GetMetrics(Font_Core_Hnd handle, f32 size)
{
    Font_Core_DWrite font = Font_Core_DWrite_GetFont(handle);

    DWRITE_FONT_METRICS metrics = {};
    if (font.face)
    {
        font.face->GetMetrics(&metrics);
    }

    Font_Core_Metrics result = {};
    if (font.face)
    {
        f32 units_per_em = f32(metrics.designUnitsPerEm);
        result.line_adv       = (96.f/72.f) * size * f32(metrics.lineGap)   / units_per_em;
        result.ascent         = (96.f/72.f) * size * f32(metrics.ascent)    / units_per_em;
        result.descent        = (96.f/72.f) * size * f32(metrics.descent)   / units_per_em;
        result.capital_height = (96.f/72.f) * size * f32(metrics.capHeight) / units_per_em;
    }

    return result;
}

function Font_Core_RasterResult
Font_Core_Raster(Arena* arena, Font_Core_Hnd handle, f32 size, Str8 string)
{
    Scratch scratch = ScratchBegin(&arena, 1);
    HRESULT hresult = 0;

    COLORREF background = RGB(0, 0, 0);
    COLORREF foreground = RGB(255, 255, 255);

    Str32 str = String32(scratch.arena, string);
    Font_Core_DWrite font = Font_Core_DWrite_GetFont(handle);

    DWRITE_FONT_METRICS font_metrics = {};
    if(font.face)
    {
        font.face->GetMetrics(&font_metrics);
    }
    f32 design_units_per_em = f32(font_metrics.designUnitsPerEm);

    u16* glyph_indices = PushArrayNZ(scratch.arena, u16, str.size);
    if(font.face)
    {
        hresult = font.face->GetGlyphIndices(str.str, u32(str.size), glyph_indices);
    }

    u64 glyphs_count = str.size;
    DWRITE_GLYPH_METRICS* glyphs_metrics = PushArrayNZ(scratch.arena, DWRITE_GLYPH_METRICS, glyphs_count);
    if(font.face)
    {
        hresult = font.face->GetGdiCompatibleGlyphMetrics(size, 1.f, 0, 1, glyph_indices, u32(glyphs_count), glyphs_metrics, 0);
    }

    f32      advance   = 0;
    Vec2_i64 atlas_dim = {};
    if(font.face)
    {
        atlas_dim.y = i64((96.f/72.f) * size * f32(font_metrics.ascent + font_metrics.descent) / design_units_per_em);
        for(u64 idx = 0; idx < glyphs_count; idx++)
        {
            DWRITE_GLYPH_METRICS* glyph_metrics = glyphs_metrics + idx;

            f32 glyph_advance_width  = (96.f/72.f) * size * f32(glyph_metrics->advanceWidth)   / design_units_per_em;
            f32 glyph_advance_height = (96.f/72.f) * size * f32(glyph_metrics->advanceHeight)  / design_units_per_em;

            advance    += (f32)i64(glyph_advance_width);
            atlas_dim.x = i64(Max(f32(atlas_dim.x), advance));
        }

        atlas_dim.x  = ((atlas_dim.x + 7) / 8) * 8;
        atlas_dim.x += 4;
        atlas_dim.y += 2;
    }

    IDWriteBitmapRenderTarget* render_target = 0;
    if(font.face)
    {
        hresult = font_dwrite_state->gdi_interop->CreateBitmapRenderTarget(0, u32(atlas_dim.x), u32(atlas_dim.y), &render_target);
        render_target->SetPixelsPerDip(1.0);
    }

    HDC dc = 0;
    if(font.face)
    {
        dc = render_target->GetMemoryDC();
        HGDIOBJ original = SelectObject(dc, GetStockObject(DC_PEN));
        SetDCPenColor(dc, background);
        SelectObject(dc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(dc, background);
        Rectangle(dc, 0, 0, i32(atlas_dim.x), i32(atlas_dim.y));
        SelectObject(dc, original);
    }

    Vec2_f32 draw_at = {1.f, f32(atlas_dim.y) - 2.f};
    if(font.face)
    {
        f32 ascent  = (96.f/72.f) * size * font_metrics.ascent  / design_units_per_em;
        f32 descent = (96.f/72.f) * size * font_metrics.descent / design_units_per_em;
        draw_at.y -= descent;
    }

    DWRITE_GLYPH_RUN glyph_run = {};
    if(font.face)
    {
        glyph_run.fontFace     = font.face;
        glyph_run.fontEmSize   = size * 96.f / 72.f;
        glyph_run.glyphCount   = u32(str.size);
        glyph_run.glyphIndices = glyph_indices;
    }

    RECT bbox = {};
    if(font.face)
    {
        hresult = render_target->DrawGlyphRun(draw_at.x, draw_at.y,
                                            DWRITE_MEASURING_MODE_NATURAL,
                                            &glyph_run,
                                            font_dwrite_state->render_params,
                                            foreground,
                                            &bbox);
    }

    DIBSECTION dib_section = {};
    if(font.face)
    {
        HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);
        GetObject(bitmap, sizeof(dib_section), &dib_section);
    }

    Font_Core_RasterResult result = {};
    if(font.face)
    {
        result.atlas_dim = atlas_dim;
        result.atlas     = PushArrayNZ(arena, u8, atlas_dim.x*atlas_dim.y*4);
        result.advance   = advance;
        result.height    = i16(f32(bbox.bottom) + 4.f);

        {
            u8* src = (u8*)dib_section.dsBm.bmBits;
            u8* dst = (u8*)result.atlas;

            u64 src_pitch = (u64)dib_section.dsBm.bmWidthBytes;
            u64 dst_pitch = atlas_dim.x * 4;

            u8* src_row = (u8*)src;
            u8* dst_row = dst;

            for(i64 y = 0; y < atlas_dim.y; y++)
            {
                u8* src_at  = src_row;
                u8* dst_at = dst_row;

                for(i64 x = 0; x < atlas_dim.x; x++)
                {
                    dst_at[0] = 0xFF;
                    dst_at[1] = 0xFF;
                    dst_at[2] = 0xFF;
                    dst_at[3] = src_at[0];
                    src_at += 4;
                    dst_at += 4;
                }

                src_row += src_pitch;
                dst_row += dst_pitch;
            }
        }

        render_target->Release();
    }
    ScratchEnd(scratch);

    return result;
}