#ifndef FONT_CACHE_H
#define FONT_CACHE_H

struct Font_Tag
{
    u64 v[2];
};

struct Font_Hash
{
    u64 v[2];
};

struct Font_Metrics
{
    f32 line_adv;
    f32 ascent;
    f32 descent;
    f32 capital_height;
};

struct Font_Piece
{
    Font_Piece* next;
    Render_Hnd texture;
    Rect2_f32 src_rect;
    Vec2_f32 offset;
    f32 advance;
    u32 decode_size;
};

struct Font_Run
{
    Font_Piece* first;
    Font_Piece* last;
    u64 piece_count;
    f32 advance;
};

struct Font_TagNode
{
    Font_TagNode* hash_next;
    Font_Tag tag;
    Font_Core_Hnd handle;
};

struct Font_TagSlot
{
    Font_TagNode* first;
    Font_TagNode* last;
};

struct Font_CacheNode
{
    Font_CacheNode* next;
    Font_CacheNode* prev;
    Font_Hash hash;
    Rect2_i64 alloc_region;
    f32 advance;
    f32 height;
};

struct Font_CacheSlot
{
    Font_CacheNode* first;
    Font_CacheNode* last;
};

struct Font_Atlas
{
    Atlas* allocator;
    Render_Hnd texture;
};

struct Font_AtlasNode
{
    Font_AtlasNode* next;
    Font_Atlas atlas;
};

struct Font_AtlasList
{
    Font_AtlasNode* first;
    Font_AtlasNode* last;
    u64 count;
};

struct Font_State
{
    Arena* arena;
    Font_TagSlot* tag_table;
    u64           tag_table_size;

    Font_CacheSlot* cache_table;
    u64             cache_table_size;

    Font_Atlas atlas;
};


function b32 Font_MatchTag(Font_Tag a, Font_Tag b);
function Font_Tag Font_GetTag(u128 hash);
function Font_Tag Font_GetTag(Str8 string);

function b32 Font_MatchHash(Font_Hash a, Font_Hash b);
function Font_Hash Font_GetHash(Font_Tag tag, f32 size, Str8 string);

function Font_Run Font_GetRun(Arena* arena, Font_Tag font, f32 size, Str8 string);
function f32      Font_GetAdvance(Font_Tag font, f32 size, Str8 string);
function u64      Font_GetByteOffset(Font_Tag font, f32 size, Str8 string, f32 advance);
function Str8     Font_GetTruncatedString(Font_Tag font, f32 size, Str8 string, f32 max, f32 trailer_advance);

function Font_AtlasList Font_PushAtlasList(Arena* arena);

function void Font_Initialise(Vec2_i64 glyph_atlas_size);

#endif