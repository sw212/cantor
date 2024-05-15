#ifndef UI_CORE_H
#define UI_CORE_H

struct UI_Key
{
    u64 v[1];
};

enum UI_MouseButtonType
{
    UI_MouseButton_Left,
    UI_MouseButton_Right,
    UI_MouseButton_Count,
};

enum UI_EventType
{
    UI_Event_NOP,
    UI_Event_Press,
    UI_Event_Release,
    UI_Event_Key,
    UI_EventType_Count,
};

struct UI_Event
{
    UI_EventType   type;

    Sys_Key key;
    Sys_KeyModifier modifiers;

    Vec2_f32 position;
    Str8 string;
};

struct UI_EventItem
{
    UI_Event event;
    UI_EventItem* next;
    UI_EventItem* prev;
};

struct UI_EventList
{
    u64 count;
    UI_EventItem* first;
    UI_EventItem* last;
};

enum UI_SizeType
{
    UI_Size_Px,       // pixels
    UI_Size_Percent,  // of parent
    UI_Size_Text,     
    UI_Size_Children, // size from children
    UI_Size_Count,
};

enum UI_Align
{
    UI_Align_Left,
    UI_Align_Center,
    UI_Align_Right,
    UI_Align_Count,
};

struct UI_Size
{
    UI_SizeType type;
    f32 value;
    f32 fractional_min_size;
};

typedef u32 UI_WigFlags;
enum
{
   UI_Wig_Disabled              = (1<<0),
   UI_Wig_AllowMouse            = (1<<1),
   UI_Wig_AllowKey              = (1<<2),

   UI_Wig_FloatX                = (1<<3),
   UI_Wig_FloatY                = (1<<4),
   UI_Wig_OverflowX             = (1<<5),
   UI_Wig_OverflowY             = (1<<6),

   UI_Wig_Clip                  = (1<<7),
   UI_Wig_IgnoreTruncate        = (1<<8),
   UI_Wig_IgnoreHash            = (1<<9),
   UI_Wig_HasText               = (1<<10),
   UI_Wig_DrawBG                = (1<<11),

   UI_Wig_Custom                = (1<<12),

   UI_Wig_Floating  = UI_Wig_FloatX | UI_Wig_FloatY,
   UI_Wig_Interactable = UI_Wig_AllowMouse | UI_Wig_AllowKey,
};

struct UI_WigText
{
    Vec4_f32 color;
    UI_Align alignment;
    f32      padding;
    Font_Tag font_tag;
    f32      font_size;
};

struct UI_WigStyle
{
    f32 r[Corner_Count]; // corner radius
    f32 t;               // border thickness
    Vec4_f32 color_background;
    Vec4_f32 color_border;
    Vec4_f32 color_overlay;
    Render_RegionTex2D_f32 region;
};

struct UI_WigBucket
{
    Draw_Context* bucket;
    f32 bucket_anchor_weights[Corner_Count];
};

#define UI_DRAW_CUSTOM(name) void name(struct UI_Wig* wig)
typedef UI_DRAW_CUSTOM(UI_DrawCustomFn);
struct UI_WigCustom
{
    UI_DrawCustomFn* fn;
    void* data;
};

struct UI_Wig
{
    UI_Wig* next;
    UI_Wig* prev;

    UI_Wig* first;
    UI_Wig* last;

    UI_Wig* parent;
    u64 child_count;

    UI_Wig* hash_next;
    UI_Wig* hash_prev;

    UI_Key key;

    UI_Size       initial_size[Axis2_Count];
    Axis2         layout_direction;
    
    UI_WigText*   text;
    Str8          string;

    UI_WigFlags   flags;
    f32           opacity;
    UI_WigStyle*  style;
    UI_WigBucket* bucket;
    UI_WigCustom  custom;


    Vec2_f32 calculated_size;
    Vec2_f32 relative_pos; // to parent

    Rect2_f32 rect;
    Rect2_f32 rect_relative;
    Vec2_f32  rect_delta[Corner_Count];

    f32 hot_t;
    f32 active_t;
    f32 disabled_t;
    f32 focus_hot_t;
    f32 focus_active_t;
    u64 init_generation;
    u64 curr_generation;
    Vec2_f32 view_off;
    Vec2_f32 target_view_off;
};

struct UI_WigTraversal
{
    UI_Wig* next;
    i32 push_count;
    i32 pop_count;
};

struct UI_Action
{
    UI_Wig* wig;
    u8 left_clicked        : 1;
    u8 left_pressed        : 1;
    u8 left_released       : 1;
    u8 left_dragging       : 1;
    u8 left_double_clicked : 1;
    u8 right_clicked       : 1;
    u8 right_pressed       : 1;
    u8 right_released      : 1;
    u8 right_dragging      : 1;
    u8 hovering            : 1;
    u8 mouse_is_over       : 1;
    u8 keyboard_pressed    : 1;
    Sys_KeyModifier modifiers;
};


struct UI_WigSlot
{
    UI_Wig* first;
    UI_Wig* last;
};

struct UI_CursorVizData
{
    UI_Key key;
    b32 is_active;
    Vec2_f32 p;
    f32 line_height;
    f32 advance;
    f32 velocity;
};

struct UI_ParentNode{UI_ParentNode* next; UI_Wig* v;};
struct UI_FlagsNode{UI_FlagsNode* next; UI_WigFlags v;};

struct UI_AbsoluteXNode{UI_AbsoluteXNode* next; f32 v;};
struct UI_AbsoluteYNode{UI_AbsoluteYNode* next; f32 v;};

struct UI_DesiredWidthNode{UI_DesiredWidthNode* next; UI_Size v;};
struct UI_DesiredHeightNode{UI_DesiredHeightNode* next; UI_Size v;};

struct UI_OpacityNode{UI_OpacityNode* next; f32 v;};
struct UI_ColorTextNode{UI_ColorTextNode* next; Vec4_f32 v;};
struct UI_ColorBackgroundNode{UI_ColorBackgroundNode* next; Vec4_f32 v;};
struct UI_ColorBorderNode{UI_ColorBorderNode* next; Vec4_f32 v;};

struct UI_Regionf32Node{UI_Regionf32Node* next; Render_RegionTex2D_f32 v;};

struct UI_FontNode{UI_FontNode* next; Font_Tag v;};
struct UI_FontSizeNode{UI_FontSizeNode* next; f32 v;};

struct UI_LayoutDirectionNode{UI_LayoutDirectionNode* next; Axis2 v;};


struct UI_TextAlignmentNode{UI_TextAlignmentNode* next; UI_Align v;};
struct UI_TextPaddingNode{UI_TextPaddingNode* next; f32 v;};

struct UI_SeedKeyNode{UI_SeedKeyNode* next; UI_Key v;};

struct UI_Ctx
{
    Arena* arena;
    Arena* frame_arenas[2];

    UI_Wig* root;
    u64     generation;

    Sys_Hnd       window;
    UI_EventList* event_list;

    Vec2_f32 mouse_pos;
    Vec2_f32 drag_start_pos;
    Arena*   drag_arena;
    Str8     drag_store;

    UI_Key hot;
    UI_Key active[UI_MouseButton_Count];
    b32    clear_hot_active;

    UI_Wig* free_wig;
    u64     free_wig_count;

    UI_WigSlot* wig_table;
    u64         wig_table_size;

    UI_ParentNode parent_nil;
    UI_FlagsNode  flags_nil;

    UI_LayoutDirectionNode layout_direction_nil;

    UI_DesiredWidthNode  desired_width_nil;
    UI_DesiredHeightNode desired_height_nil;

    UI_AbsoluteXNode absolute_x_nil;
    UI_AbsoluteYNode absolute_y_nil;
    UI_Regionf32Node regionf32_nil;

    UI_ColorTextNode       color_text_nil;
    UI_ColorBackgroundNode color_background_nil;
    UI_ColorBorderNode     color_border_nil;
    UI_OpacityNode         opacity_nil;

    UI_FontNode     font_nil;
    UI_FontSizeNode font_size_nil;

    UI_TextAlignmentNode text_alignment_nil;
    UI_TextPaddingNode   text_padding_nil;

    UI_SeedKeyNode seed_key_nil;

    #define Stack(T, name) struct { T* top; T* free; b32 pop_default; u32 pad; } name
    Stack(UI_ParentNode, parent_stack);

    Stack(UI_FlagsNode, flags_stack);

    Stack(UI_AbsoluteXNode, absolute_x_stack);
    Stack(UI_AbsoluteYNode, absolute_y_stack);

    Stack(UI_DesiredWidthNode, desired_width_stack);
    Stack(UI_DesiredHeightNode, desired_height_stack);

    Stack(UI_OpacityNode, opacity_stack);
    Stack(UI_ColorTextNode, color_text_stack);
    Stack(UI_ColorBackgroundNode, color_background_stack);
    Stack(UI_ColorBorderNode, color_border_stack);

    Stack(UI_Regionf32Node, regionf32_stack);

    Stack(UI_FontNode, font_stack);
    Stack(UI_FontSizeNode, font_size_stack);

    Stack(UI_LayoutDirectionNode, layout_direction_stack);


    Stack(UI_TextAlignmentNode, text_alignment_stack);
    Stack(UI_TextPaddingNode, text_padding_stack);

    Stack(UI_SeedKeyNode, seed_key_stack);
    #undef Stack
};


function Arena*        UI_Arena();
function UI_Wig*       UI_Root();
function Sys_Hnd       UI_Window();
function UI_EventList* UI_Events();
function Vec2_f32      UI_Mouse();

function UI_Ctx* UI_Initialise();
function void    UI_SetState(UI_Ctx* ui_ctx);

function void UI_Begin(Sys_Hnd window_handle, UI_EventList* events);
function void UI_End();

function UI_Event*    UI_EventPush(Arena* arena, UI_EventList* event_list);
function void         UI_ConsumeEvent(UI_EventList* event_list, UI_Event* event);
function UI_EventType UI_EventTypeFromSysEvent(Sys_EventType type);
function UI_EventList UI_EventListFromSysEvents(Arena* arena, Sys_Hnd window, Sys_EventList* sys_events);

#define UI_WigSetNil(wig) ((wig) = &ui_nil_wig)
function b32     UI_IsNil(UI_Wig* wig);
function UI_Wig* UI_GetWig(UI_Key key);

function UI_Key UI_ZeroKey();
function b32    UI_MatchKey(UI_Key a, UI_Key b);

function UI_Key UI_GetKey(UI_Key seed, Str8 string);
function UI_Key UI_GetKey(UI_Key seed, char* fmt, ...);

function UI_Size UI_Pixels(f32 value, f32 fractional_min_size);

function UI_WigTraversal UI_TraverseRevPreOrder(UI_Wig* wig, UI_Wig* stop, u64 sibling_offset, u64 child_offset);

function void UI_ResolveWigDimsFreePass(UI_Wig* node, Axis2 axis);
function void UI_ResolveWigDimsDownPass(UI_Wig* node, Axis2 axis);
function void UI_ResolveWigDimsUpPass(UI_Wig* node, Axis2 axis);
function void UI_ResolveWigDimsAdjustmentPass(UI_Wig* node, Axis2 axis);
function void UI_ResolveWigDimsFor(UI_Wig* node, Axis2 axis);
function void UI_ResolveWigDims();

function UI_Wig* UI_WigCreate(UI_WigFlags flags, UI_Key key);
function UI_Wig* UI_WigCreate(UI_WigFlags flags, Str8 string);
function UI_Wig* UI_WigCreate(UI_WigFlags flags, char* fmt, ...);

function void    UI_WigSetDisplayString(UI_Wig* wig, Str8 string);
function Str8    UI_WigGetDisplayString(Str8 string);
function Str8    UI_WigGetDisplayString(UI_Wig* wig);

function Str8    UI_WigGetHashString(Str8 string);

function Vec2_f32  UI_GetTextPos(UI_Wig* wig);
function UI_Action UI_GetAction(UI_Wig* wig);

function void UI_InitialiseUIStateStacks(UI_Ctx* ui_ctx);
function void UI_InitialiseUINilStateStacks(UI_Ctx* ui_ctx);
function void UI_ProcessDefaultUIStateStacks(UI_Ctx* ui_ctx);

function void UI_SetNextDesiredSize(Axis2 axis, UI_Size v);

function UI_Wig*     UI_GetParent();
function UI_WigFlags UI_GetFlags();
function f32         UI_GetAbsoluteX();
function f32         UI_GetAbsoluteY();
function UI_Size               UI_GetDesiredWidth();
function UI_Size               UI_GetDesiredHeight();
function f32                   UI_GetOpacity();
function Vec4_f32               UI_GetColorText();
function Vec4_f32               UI_GetColorBackground();
function Vec4_f32               UI_GetColorBorder();
function Render_RegionTex2D_f32           UI_GetRegionf32();
function Font_Tag                 UI_GetFont();
function f32                   UI_GetFontSize();
function Axis2                 UI_GetLayoutDirection();
function UI_Align      UI_GetTextAlignment();
function f32                   UI_GetTextPadding();
function UI_Key                UI_GetSeedKey();

function UI_Wig*               UI_SetParent(UI_Wig* v);
function UI_WigFlags           UI_SetFlags(UI_WigFlags v);
function f32                   UI_SetAbsoluteX(f32 v);
function f32                   UI_SetAbsoluteY(f32 v);
function UI_Size               UI_SetDesiredWidth(UI_Size v);
function UI_Size               UI_SetDesiredHeight(UI_Size v);
function f32                   UI_SetOpacity(f32 v);
function Vec4_f32               UI_SetColorText(Vec4_f32 v);
function Vec4_f32               UI_SetColorBackground(Vec4_f32 v);
function Vec4_f32               UI_SetColorBorder(Vec4_f32 v);
function Render_RegionTex2D_f32           UI_SetRegionf32(Render_RegionTex2D_f32 v);
function Font_Tag                 UI_SetFont(Font_Tag v);
function f32                   UI_SetFontSize(f32 v);
function Axis2                 UI_SetLayoutDirection(Axis2 v);
function UI_Align      UI_SetTextAlignment(UI_Align v);
function f32                   UI_SetTextPadding(f32 v);
function UI_Key                UI_SetSeedKey(UI_Key v);

function UI_Wig*               UI_PopParent();
function UI_WigFlags           UI_PopFlags();
function f32                   UI_PopAbsoluteX();
function f32                   UI_PopAbsoluteY();
function UI_Size               UI_PopDesiredWidth();
function UI_Size               UI_PopDesiredHeight();
function Vec4_f32               UI_PopColorText();
function Vec4_f32               UI_PopColorBackground();
function Vec4_f32               UI_PopColorBorder();
function Render_RegionTex2D_f32           UI_PopRegionf32();
function Font_Tag                 UI_PopFont();
function f32                   UI_PopFontSize();
function Axis2                 UI_PopLayoutDirection();
function UI_Align      UI_PopTextAlignment();
function f32                   UI_PopTextPadding();
function UI_Key                UI_PopSeedKey();

function UI_Wig*               UI_SetNextParent(UI_Wig* v);
function UI_WigFlags           UI_SetNextFlags(UI_WigFlags v);
function f32                   UI_SetNextAbsoluteX(f32 v);
function f32                   UI_SetNextAbsoluteY(f32 v);
function UI_Size               UI_SetNextDesiredWidth(UI_Size v);
function UI_Size               UI_SetNextDesiredHeight(UI_Size v);
function f32                   UI_SetNextOpacity(f32 v);
function Vec4_f32               UI_SetNextColorText(Vec4_f32 v);
function Vec4_f32               UI_SetNextColorBackground(Vec4_f32 v);
function Vec4_f32               UI_SetNextColorBorder(Vec4_f32 v);
function Render_RegionTex2D_f32           UI_SetNextRegionf32(Render_RegionTex2D_f32 v);
function Font_Tag              UI_SetNextFont(Font_Tag v);
function f32                   UI_SetNextFontSize(f32 v);
function Axis2                 UI_SetNextLayoutDirection(Axis2 v);
function UI_Align              UI_SetNextTextAlignment(UI_Align v);
function f32                   UI_SetNextTextPadding(f32 v);
function UI_Key                UI_SetNextSeedKey(UI_Key v);


#define UI_Parent(v) ScopedContextBlock(UI_SetParent(v), UI_PopParent())
#define UI_Flags(v) ScopedContextBlock(UI_SetFlags(v), UI_PopFlags())

#define UI_AbsoluteX(v) ScopedContextBlock(UI_SetAbsoluteX(v), UI_PopAbsoluteX())
#define UI_AbsoluteY(v) ScopedContextBlock(UI_SetAbsoluteY(v), UI_PopAbsoluteY())

#define UI_DesiredWidth(v) ScopedContextBlock(UI_SetDesiredWidth(v), UI_PopDesiredWidth())
#define UI_DesiredHeight(v) ScopedContextBlock(UI_SetDesiredHeight(v), UI_PopDesiredHeight())

#define UI_ColorText(v) ScopedContextBlock(UI_SetColorText(v), UI_PopColorText())
#define UI_ColorBackground(v) ScopedContextBlock(UI_SetColorBackground(v), UI_PopColorBackground())
#define UI_ColorBorder(v) ScopedContextBlock(UI_SetColorBorder(v), UI_PopColorBorder())

#endif
