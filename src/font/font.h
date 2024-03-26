#ifndef FONT_H
#define FONT_H

struct Font_Core_Hnd
{
    u64 v[2];
};

/*
https://learn.microsoft.com/en-us/windows/win32/api/dwrite/ns-dwrite-dwrite_font_metrics

The number of font design units per em unit.
Font files use their own coordinate system of font design units.
A font design unit is the smallest measurable unit in the em square, an imaginary square that is used to size and align glyphs.
The concept of em square is used as a reference scale factor when defining font size and device transformation semantics.
The size of one em square is also commonly used to compute the paragraph indentation value.
*/
struct Font_Core_Metrics
{
    f32 design_units_per_em;
    f32 ascent;
    f32 descent;
    f32 line_adv;
    f32 capital_height;
};

struct Font_Core_RasterResult
{
    Vec2_i64 atlas_dim;
    void*    atlas;
    f32      advance;
    i16      height;
};

function void                   Font_Core_Initialize();
function Font_Core_Hnd          Font_Core_Open(u128 hash);
function void                   Font_Core_Close(Font_Core_Hnd handle);
function Font_Core_Metrics      Font_Core_GetMetrics(Font_Core_Hnd handle, f32 size);
function Font_Core_RasterResult Font_Core_Raster(Arena* arena, Font_Core_Hnd handle, f32 size, Str8 string);


#include "dwrite/font_dwrite.h"

#endif