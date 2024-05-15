#ifndef RENDER_D3D11_H
#define RENDER_D3D11_H

#pragma warning(push, 0)
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#pragma warning(pop)

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")


struct Render_D3D11_Rect2D
{
    Vec2_f32   viewport;
    f32        opacity;
    f32        pad_a;
    Mat4x4_f32 albedo_channel_map;
    Vec2_f32   albedo_tex_size;
    Vec2_f32   pad_b;
    Vec4_f32   xform[3];
    Vec2_f32   xform_scale;
};

struct Render_D3D11_Sprite3D
{
    Mat4x4_f32 xform;
    Mat4x4_f32 albedo_channel_map;
    Vec2_f32   albedo_tex_size;
    f32        alpha_test_min;
};

struct Render_D3D11_PointLight3D
{
    Mat4x4_f32 xform;
    Mat4x4_f32 inverse_view_projection;
};

struct Render_D3D11_DebugLine3D
{
    Mat4x4_f32 xform;
};

struct Render_D3D11_Command
{
    u64 size;
};

Render_D3D11_Command render_d3d11_command_table[] =
{
    {},
    {sizeof(Render_D3D11_Rect2D)},
    {sizeof(Render_D3D11_Sprite3D)},
    {sizeof(Render_D3D11_PointLight3D)},
    {sizeof(Render_D3D11_DebugLine3D)},
};


enum Render_D3D11_CommandType
{
    Render_D3D11_Command_Nil,
    Render_D3D11_Command_Rect2D,
    Render_D3D11_Command_Sprite3D,
    Render_D3D11_Command_PointLight3D,
    Render_D3D11_Command_DebugLine3D,
    Render_D3D11_Command_Count,
};

enum Render_D3D11_ShaderType
{
    Render_D3D11_Shader_Nil,
    Render_D3D11_Shader_Rect2D,
    Render_D3D11_Shader_Sprite3D,
    Render_D3D11_Shader_PointLight3D,
    Render_D3D11_Shader_FramebufferBlit,
    Render_D3D11_Shader_DebugLine3D,
    Render_D3D11_Shader_Count,
};

struct Render_D3D11_Tex2D
{
    ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* view;
    Vec2_i32 size;
    Render_Tex2DType format;
    Render_Tex2DUsageType kind;
};

struct Render_D3D11_Buffer
{
    ID3D11Buffer* obj;
    u64 size;
};

struct Render_D3D11_OverflowBuffer
{
    Render_D3D11_OverflowBuffer* next;
    ID3D11Buffer* buffer;
};

struct Render_D3D11_State
{
    Arena* arena;
    Sys_Hnd device_mutex;

    ID3D11Device1*           device;
    ID3D11DeviceContext1*    device_ctx;
    
    ID3D11Device*            base_device;
    ID3D11DeviceContext*     base_device_ctx;

    IDXGIDevice1*            device_dxgi;
    IDXGIAdapter*            adapter_dxgi;
    IDXGIFactory2*           factory_dxgi;

    ID3D11RasterizerState1*  rasterizer;
    ID3D11BlendState*        blend_state_default;
    ID3D11BlendState*        blend_state_additive;

    ID3D11SamplerState*      sampler_nearest;
    ID3D11SamplerState*      sampler_linear;

    ID3D11DepthStencilState* noop_ds_state;
    ID3D11DepthStencilState* plain_ds_state;
    ID3D11DepthStencilState* write_ds_state;
    ID3D11DepthStencilState* read_ds_state;

    ID3D11Buffer* global_buffer_table[Render_D3D11_Command_Count];

    ID3D11InputLayout*  input_layout_table[Render_D3D11_Shader_Count];
    ID3D11VertexShader* shader_table_vertex[Render_D3D11_Shader_Count];
    ID3D11PixelShader*  shader_table_pixel[Render_D3D11_Shader_Count];

    ID3D11Buffer* scratch_64kb;
    ID3D11Buffer* scratch_1mb;
    ID3D11Buffer* scratch_2mb;
    ID3D11Buffer* scratch_4mb;
    ID3D11Buffer* scratch_8mb;

    Render_Hnd white_texture;
    Render_Hnd backup_texture;

    Arena* overflow_arena;
    Render_D3D11_OverflowBuffer* overflow_buffer_first;
    Render_D3D11_OverflowBuffer* overflow_buffer_last;
};

struct Render_D3D11_WindowAttach
{
    IDXGISwapChain1*          swapchain;
    ID3D11Texture2D*          framebuffer;
    ID3D11RenderTargetView*   framebuffer_rtv;

    ID3D11Texture2D*          gbuffer_color;
    ID3D11RenderTargetView*   gbuffer_color_rtv;
    ID3D11ShaderResourceView* gbuffer_color_srv;

    ID3D11Texture2D*          gbuffer_depth;
    ID3D11DepthStencilView*   gbuffer_depth_dsv;
    ID3D11ShaderResourceView* gbuffer_depth_srv;

    ID3D11Texture2D*          geometry_color;
    ID3D11RenderTargetView*   geometry_color_rtv;
    ID3D11ShaderResourceView* geometry_color_srv;

    ID3D11Texture2D*          geometry_depth;
    ID3D11DepthStencilView*   geometry_depth_dsv;
    ID3D11ShaderResourceView* geometry_depth_srv;

    // ID3D11Texture2D*          gbuffer_scratch_color;
    // ID3D11RenderTargetView*   gbuffer_scratch_color_rtv;
    // ID3D11ShaderResourceView* gbuffer_scratch_color_srv;

    Vec2_i64 prev_resolution;
};

function DXGI_FORMAT Render_D3D11_GetFormat(Render_Tex2DType fmt);

function Render_D3D11_Buffer Render_D3D11_GetBuffer(Render_Hnd handle);
function Render_D3D11_Tex2D  Render_D3D11_GetTex2D(Render_Hnd handle);
function Render_Hnd          Render_D3D11_GetHandle(Render_D3D11_Tex2D texture);

function Render_D3D11_WindowAttach* Render_D3D11_GetWindowAttach(Render_Hnd handle);
function Render_Hnd                 Render_D3D11_GetHandle(Render_D3D11_WindowAttach* render_window);

function void          Render_D3D11_BufferWrite(ID3D11DeviceContext1* ctx, ID3D11Buffer* buffer, Buf8 data);
function ID3D11Buffer* Render_D3D11_GetInstanceBuffer(Render_GroupList* list);
function Mat4x4_f32    Render_D3D11_GetSampleChannelMap(Render_Tex2DType format);
#endif