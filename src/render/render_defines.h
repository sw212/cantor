#ifndef RENDER_DEFINES_H
#define RENDER_DEFINES_H

union Render_Hnd
{
    u64 v_64[4];
    u32 v_32[8];
};

typedef u32 Render_VertexFlags;
enum Render_VertexType
{
    Render_Vertex_Normals = (1<<0),
    Render_Vertex_UVs     = (1<<1),
    Render_Vertex_Colors  = (1<<2),
};

enum Render_BlendType
{
    Render_Blend_Normal,
    Render_Blend_Additive,
    Render_Blend_Count
};

enum Render_Tex2DType
{
    Render_Tex2D_Unknown,
    Render_Tex2D_R8,
    Render_Tex2D_RGBA8,
    Render_Tex2D_Count,
};

enum Render_Sampler2DType
{
    Render_Sampler2D_Linear,
    Render_Sampler2D_Nearest,
    Render_Sampler2D_Count,
};

enum Render_Tex2DUsageType
{
    Render_Tex2DUsage_Static,
    Render_Tex2DUsage_Dynamic,
};

struct Render_RegionTex2D_f32
{
    Render_Hnd texture;
    Rect2_f32  region;
};


struct Render_Rect2D
{
    Rect2_f32 dst;
    Rect2_f32 src;

    Vec4_f32 colors[Corner_Count];
    
    f32 r[Corner_Count]; // corner radius
    f32 t;               // border thickness
    f32 s;               // softness
    
    f32 omit_tex;
    f32 _pad;
};

struct Render_Mesh3D
{
    Mat4x4_f32 transform;
};

struct Render_Group
{
    Render_Group* next;
    u8* v;
    u64 size; // bytes
    u64 cap;  // bytes
};

struct Render_GroupList
{
    Render_Group* first;
    Render_Group* last;
    u64 count;         // number of render groups
    u64 size;          // in bytes of all render groups
    u64 instance_size; // in bytes
};

struct Render_Collection2DParams
{
    Render_Hnd           tex;
    Render_Sampler2DType sampler_type;
    Mat3x3_f32  xform;
    Rect2_f32   clip;
    f32         opacity;
};

struct Render_Collection2D
{
    Render_Collection2D* next;
    Render_GroupList     groups;
    Render_Collection2DParams params;
};

struct Render_Collection2DList
{
    Render_Collection2D* first;
    Render_Collection2D* last;
    u64 count;
};

struct Render_Collection3DParams
{
    Render_Hnd vertices;
    Render_Hnd indices;
    Render_Hnd texture;
    Mat4x4_f32 transform;
    Render_VertexFlags flags;
    Render_Sampler2DType sampler;
};

struct Render_Collection3DItem
{
    Render_Collection3DItem*  next;
    Render_GroupList          groups;
    Render_Collection3DParams params;

    u64 hash;
};

struct Render_Collection3D
{
    Render_Collection3DItem** items;
    u64 count;
};


enum Render_PassType
{
    Render_PassType_Null,
    Render_PassType_UI,
    Render_PassType_3D,
    Render_PassType_Count,
};

struct Render_Pass_UI
{
    Rect2_f32 viewport; // TODO: is this needed?
    Render_Collection2DList rects;
};

struct Render_Pass_3D
{
    Rect2_f32 viewport;
    Rect2_f32 clip;
    
    Mat4x4_f32 view;
    Mat4x4_f32 proj;

    Render_Collection3D meshes;
};

struct Render_Pass
{
    Render_PassType pass_type;

    union
    {
        void* params;
        Render_Pass_UI* ui;
        Render_Pass_3D* geom;
    };
};

struct Render_PassNode
{
    Render_PassNode* next;
    Render_Pass      pass;
};

struct Render_PassList
{
    Render_PassNode* first;
    Render_PassNode* last;
    u64 count;
};

function Render_Hnd Render_HndZero();
function b32        Render_HndIsZero(Render_Hnd handle);
function b32        Render_MatchHnd(Render_Hnd a, Render_Hnd b);

function u64 Render_BytesPerPixel(Render_Tex2DType format);

function Render_Pass* Render_PushPassList(Arena* arena, Render_PassList* list, Render_PassType kind);
function void*        Render_PushGroupList(Arena* arena, Render_GroupList* list, u64 max_instance_count);

#endif
