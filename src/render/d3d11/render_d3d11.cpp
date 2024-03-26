Render_D3D11_State *render_d3d11_state = 0;

static D3D11_INPUT_ELEMENT_DESC render_d3d11_rect2d_input_layout[] =
{
 { "POS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,                            0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
 { "TEX",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
 { "COL",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
 { "COL",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
 { "COL",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
 { "COL",  3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
 { "CRAD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
 { "STY",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

function DXGI_FORMAT
Render_D3D11_GetFormat(Render_Tex2DType fmt)
{
    DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;

    switch (fmt)
    {
        case Render_Tex2D_R8:    { result = DXGI_FORMAT_R8_UNORM; } break;
        case Render_Tex2D_RGBA8: { result = DXGI_FORMAT_R8G8B8A8_UNORM; } break;
        
        default: break;
    }

    return result;
}

function Render_D3D11_Tex2D
Render_D3D11_GetTex2D(Render_Hnd handle)
{
    Render_D3D11_Tex2D result = {};
    {
        result.texture = (ID3D11Texture2D*)handle.v_64[0];
        result.view    = (ID3D11ShaderResourceView*)handle.v_64[1];
        result.size.x  = handle.v_32[4];
        result.size.y  = handle.v_32[5];
        result.format  = (Render_Tex2DType)handle.v_32[6];
        result.kind    = (Render_Tex2DUsageType)handle.v_32[7];
    }

    return result;
}

function Render_Hnd
Render_D3D11_GetHandle(Render_D3D11_Tex2D texture)
{
    Render_Hnd result = {};
    {
        result.v_64[0] = u64(texture.texture);
        result.v_64[1] = u64(texture.view);
        result.v_32[4] = texture.size.x;
        result.v_32[5] = texture.size.y;
        result.v_32[6] = texture.format;
        result.v_32[7] = texture.kind;
    }

    return result;
}

function Render_D3D11_WindowAttach*
Render_D3D11_GetWindowAttach(Render_Hnd handle)
{
    Render_D3D11_WindowAttach* result = (Render_D3D11_WindowAttach*)handle.v_64[0];
    return result;
}

function Render_Hnd
Render_D3D11_GetHandle(Render_D3D11_WindowAttach* render_window)
{
    Render_Hnd result = {};
    result.v_64[0] = u64(render_window);
    return result;
}

function void
Render_D3D11_BufferWrite(ID3D11DeviceContext1* ctx, ID3D11Buffer* buffer, Buf8 data)
{
    D3D11_MAPPED_SUBRESOURCE map = {};
    ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    MemoryCopy(map.pData, data.ptr, data.size);
    ctx->Unmap(buffer, 0);
}

function ID3D11Buffer*
Render_D3D11_GetInstanceBuffer(Render_GroupList* list)
{
    u32 size = u32(list->size);

    // acquire buffer
    ID3D11Buffer* buffer = 0;
    if (buffer) {}
    else if(size <= Kilobytes(64)) { buffer = render_d3d11_state->scratch_64kb; }
    else if(size <= Megabytes(1) ) { buffer = render_d3d11_state->scratch_1mb;  }
    else if(size <= Megabytes(2) ) { buffer = render_d3d11_state->scratch_2mb;  }
    else if(size <= Megabytes(4) ) { buffer = render_d3d11_state->scratch_4mb;  }
    else if(size <= Megabytes(8) ) { buffer = render_d3d11_state->scratch_8mb;  }
    else
    {
        Render_D3D11_OverflowBuffer* node = PushArray(render_d3d11_state->overflow_arena, Render_D3D11_OverflowBuffer, 1);
        D3D11_BUFFER_DESC desc = {};
        {
            desc.ByteWidth      = size;
            desc.Usage          = D3D11_USAGE_DYNAMIC;
            desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        render_d3d11_state->device->CreateBuffer(&desc, 0, &node->buffer);
        QueuePush(render_d3d11_state->overflow_buffer_first, render_d3d11_state->overflow_buffer_last, node);
        buffer = node->buffer;
    }

    // upload data
    {
        D3D11_MAPPED_SUBRESOURCE resource = {};
        render_d3d11_state->device_ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        u8* at = (u8*)resource.pData;
        for (Render_Group* group = list->first; group != 0; group = group->next)
        {
            MemoryCopy(at, group->v, group->size);
            at += group->size;
        }
        render_d3d11_state->device_ctx->Unmap(buffer, 0);
    }

    return buffer;
}

function Mat4x4_f32
Render_D3D11_GetSampleChannelMap(Render_Tex2DType format)
{
    Mat4x4_f32 result = Identity4(1.f);

    switch(format)
    {
        case Render_Tex2D_R8:
        {
            result = {1.f, 1.f, 1.f, 1.f,
                   0.f, 0.f, 0.f, 0.f,
                   0.f, 0.f, 0.f, 0.f,
                   0.f, 0.f, 0.f, 0.f,};
        } break;

        default: break;
    }

    return result;
}

function void
Render_WindowBegin(Render_Hnd render_window, Vec2_i64 resolution)
{
    Sys_Mutex(render_d3d11_state->device_mutex)
    {
        Render_D3D11_WindowAttach*   w = Render_D3D11_GetWindowAttach(render_window);
        ID3D11Device1*               d = render_d3d11_state->device;
        ID3D11DeviceContext1*      ctx = render_d3d11_state->device_ctx;

        b32 resolution_changed = (w->prev_resolution.x != resolution.x || w->prev_resolution.y != resolution.y);

        w->prev_resolution = resolution;

        if (resolution_changed)
        {
            w->framebuffer_rtv->Release();
            w->framebuffer->Release();
            w->swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            w->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&w->framebuffer));
            render_d3d11_state->device->CreateRenderTargetView(w->framebuffer, 0, &w->framebuffer_rtv);

            if(w->gbuffer_depth_srv)         {w->gbuffer_depth_srv->Release();        }
            if(w->gbuffer_depth_dsv)         {w->gbuffer_depth_dsv->Release();        }
            if(w->gbuffer_depth)             {w->gbuffer_depth->Release();            }
            if(w->gbuffer_color_srv)         {w->gbuffer_color_srv->Release();        }
            if(w->gbuffer_color_rtv)         {w->gbuffer_color_rtv->Release();        }
            if(w->gbuffer_color)             {w->gbuffer_color->Release();            }
            if(w->gbuffer_scratch_color_srv) {w->gbuffer_scratch_color_srv->Release();}
            if(w->gbuffer_scratch_color_rtv) {w->gbuffer_scratch_color_rtv->Release();}
            if(w->gbuffer_scratch_color)     {w->gbuffer_scratch_color->Release();    }

            // g-buffer
            {
                D3D11_TEXTURE2D_DESC color_desc = {};
                {
                    w->framebuffer->GetDesc(&color_desc);
                    color_desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
                    color_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                }

                D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
                {
                    rtv_desc.Format        = color_desc.Format;
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                {
                    srv_desc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srv_desc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MipLevels = u32(-1);
                }

                d->CreateTexture2D(&color_desc, 0, &w->gbuffer_color);
                d->CreateRenderTargetView(w->gbuffer_color, &rtv_desc, &w->gbuffer_color_rtv);
                d->CreateShaderResourceView(w->gbuffer_color, &srv_desc, &w->gbuffer_color_srv);
            }

            {
                D3D11_TEXTURE2D_DESC depth_desc = {};
                {
                    w->framebuffer->GetDesc(&depth_desc);
                    depth_desc.Format    = DXGI_FORMAT_R24G8_TYPELESS;
                    depth_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
                }

                D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
                {
                    dsv_desc.Flags              = 0;
                    dsv_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    dsv_desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsv_desc.Texture2D.MipSlice = 0;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                {
                    srv_desc.Format                    = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                    srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MostDetailedMip = 0;
                    srv_desc.Texture2D.MipLevels       = u32(-1);
                }

                d->CreateTexture2D(&depth_desc, 0, &w->gbuffer_depth);
                d->CreateDepthStencilView(w->gbuffer_depth, &dsv_desc, &w->gbuffer_depth_dsv);
                d->CreateShaderResourceView(w->gbuffer_depth, &srv_desc, &w->gbuffer_depth_srv);
            }

            {
                D3D11_TEXTURE2D_DESC color_desc = {};
                {
                    w->framebuffer->GetDesc(&color_desc);
                    color_desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
                    color_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                }

                D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
                {
                    rtv_desc.Format        = color_desc.Format;
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                }

                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                {
                    srv_desc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srv_desc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MipLevels = u32(-1);
                }

                d->CreateTexture2D(&color_desc, 0, &w->gbuffer_scratch_color);
                d->CreateRenderTargetView(w->gbuffer_scratch_color, &rtv_desc, &w->gbuffer_scratch_color_rtv);
                d->CreateShaderResourceView(w->gbuffer_scratch_color, &srv_desc, &w->gbuffer_scratch_color_srv);
            }
        }

        f32 clear[4] = {0.f, 0.f, 0.f, 0.f};
        ctx->ClearRenderTargetView(w->framebuffer_rtv, clear);
    }
}

function void
Render_WindowSubmit(Render_Hnd render_window, Render_PassList* render_passes)
{
    Sys_Mutex(render_d3d11_state->device_mutex)
    {
        Render_D3D11_WindowAttach*   w = Render_D3D11_GetWindowAttach(render_window);
        ID3D11DeviceContext1*      ctx = render_d3d11_state->device_ctx;

        for (Render_PassNode* pass_node  = render_passes->first;
                              pass_node != 0;
                              pass_node  = pass_node->next)
        {
            Render_Pass* pass = &pass_node->pass;

            switch (pass->pass_type)
            {
                default: { } break;

                case Render_PassType_UI:
                {
                    Render_Pass_UI* UI = pass->ui;

                    for (Render_Collection2D* node = UI->rects.first; node != 0; node = node->next)
                    {
                        Render_GroupList* groups = &node->groups;
                        Render_Collection2DParams* params = &node->params;

                        ID3D11Buffer* buffer = Render_D3D11_GetInstanceBuffer(groups);

                        // texture + sampler
                        Render_Hnd  albedo_texture_handle = Render_HndIsZero(params->tex) ? render_d3d11_state->backup_texture : params->tex;
                        Render_D3D11_Tex2D albedo_texture = Render_D3D11_GetTex2D(albedo_texture_handle);
                        Mat4x4_f32     albedo_channel_map = Render_D3D11_GetSampleChannelMap(albedo_texture.format);

                        Render_Sampler2DType sampler_type = params->sampler_type;
                        ID3D11SamplerState*       sampler = render_d3d11_state->sampler_linear;

                        switch(sampler_type)
                        {
                            case Render_Sampler2D_Nearest: { sampler = render_d3d11_state->sampler_nearest; } break;
                            case Render_Sampler2D_Linear:  { sampler = render_d3d11_state->sampler_linear;  } break;
                            default: break;
                        }

                        Vec2_i64 resolution = w->prev_resolution;

                        // viewport + rasterizer
                        f32 min_depth = 0.f;
                        f32 max_depth = 1.f;

                        Rect2_f32            viewport = UI->viewport;
                        D3D11_VIEWPORT d3d11_viewport = {0.f, 0.f, f32(resolution.x), f32(resolution.y), min_depth, max_depth};

                        if (viewport.min.x || viewport.max.x || viewport.min.y || viewport.max.y)
                        {
                            d3d11_viewport.TopLeftX = viewport.min.x;
                            d3d11_viewport.TopLeftY = viewport.max.y;
                            d3d11_viewport.Width    = viewport.max.x - viewport.min.x;
                            d3d11_viewport.Height   = viewport.max.y - viewport.min.y;
                        }

                        ctx->RSSetViewports(1, &d3d11_viewport);
                        ctx->RSSetState(render_d3d11_state->rasterizer);

                        // scissor rect
                        {
                            D3D11_RECT rect = {};
                            rect.bottom = LONG(resolution.y);
                            rect.right  = LONG(resolution.x);

                            Rect2_f32 clip = params->clip;
                            if (clip.max.x > clip.min.x && clip.max.y > clip.min.y)
                            {
                                rect.top    = LONG(clip.max.y);
                                rect.left   = LONG(clip.min.x);
                                rect.bottom = LONG(clip.min.y);
                                rect.right  = LONG(clip.max.x);
                            }

                            ctx->RSSetScissorRects(1, &rect);
                        }

                        ctx->OMSetRenderTargets(1, &w->framebuffer_rtv, 0);
                        ctx->OMSetBlendState(render_d3d11_state->blend_state_default, 0, 0xFFFFFFFF);
                        ctx->OMSetDepthStencilState(render_d3d11_state->noop_ds_state, 0);

                        ID3D11Buffer* cmd_buffer = render_d3d11_state->global_buffer_table[Render_D3D11_Shader_Rect2D];
                        ID3D11VertexShader* vertex_shader = render_d3d11_state->shader_table_vertex[Render_D3D11_Shader_Rect2D];
                        ID3D11PixelShader*   pixel_shader = render_d3d11_state->shader_table_pixel[Render_D3D11_Shader_Rect2D];
                        ID3D11InputLayout*   input_layout = render_d3d11_state->input_layout_table[Render_D3D11_Shader_Rect2D];

                        Render_D3D11_Rect2D cmds = {};
                        {
                            cmds.viewport = Vec2_f32{f32(resolution.x), f32(resolution.y)};
                            cmds.opacity       = params->opacity;

                            cmds.albedo_channel_map = albedo_channel_map;
                            cmds.albedo_tex_size = {f32(albedo_texture.size.x), f32(albedo_texture.size.y)};

                            Mat3x3_f32 xform = params->xform;
                            cmds.xform[0] = {xform.v[0][0], xform.v[0][1], xform.v[0][2], 0.0f};
                            cmds.xform[1] = {xform.v[1][0], xform.v[1][1], xform.v[1][2], 0.0f};
                            cmds.xform[2] = {xform.v[2][0], xform.v[2][1], xform.v[2][2], 0.0f};

                            cmds.xform_scale.x = Sqrt(Square(xform.v[0][0]) + Square(xform.v[1][0]));
                            cmds.xform_scale.y = Sqrt(Square(xform.v[0][1]) + Square(xform.v[1][1]));
                        }
                        Render_D3D11_BufferWrite(ctx, cmd_buffer, Buf8Struct(&cmds));

                        // Topology & Layout
                        u32 stride = u32(groups->instance_size);
                        u32 offset = 0;
                        ctx->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // instanced rect
                        ctx->IASetInputLayout(input_layout);
                        ctx->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

                        ctx->VSSetShader(vertex_shader, 0, 0);
                        ctx->VSSetConstantBuffers(0, 1, &cmd_buffer);
                        
                        ctx->PSSetShader(pixel_shader, 0, 0);
                        ctx->PSSetConstantBuffers(0, 1, &cmd_buffer);
                        ctx->PSSetShaderResources(0, 1, &albedo_texture.view);

                        u64 inst_count = groups->size / groups->instance_size;
                        ctx->DrawInstanced(4, u32(inst_count), 0, 0);
                    }
                } break;
            }
        }
    }
}

function void
Render_WindowEnd(Render_Hnd render_window)
{
    Sys_Mutex(render_d3d11_state->device_mutex)
    {
        Render_D3D11_WindowAttach*   w = Render_D3D11_GetWindowAttach(render_window);
        ID3D11DeviceContext1*      ctx = render_d3d11_state->device_ctx;
        w->swapchain->Present(1, 0);
        ctx->ClearState();
    }
}

function void
Render_FrameEnd()
{
    for (Render_D3D11_OverflowBuffer* node  = render_d3d11_state->overflow_buffer_first;
                                          node != 0;
                                          node  = node->next)
    {
        node->buffer->Release();
    }
    
    ArenaClear(render_d3d11_state->overflow_arena);
    render_d3d11_state->overflow_buffer_first = 0;
    render_d3d11_state->overflow_buffer_last  = 0;
}

function Render_Hnd
Render_AllocTex2D(Vec2_i64 size, Render_Tex2DType format, Render_Tex2DUsageType usage, void* initial_data)
{
    D3D11_USAGE d3d11_usage = D3D11_USAGE_DEFAULT;
    UINT access_flags = 0;

    switch(usage)
    {
        case Render_Tex2DUsage_Static:
        {
            d3d11_usage = initial_data ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
            access_flags = 0;
        }break;

        case Render_Tex2DUsage_Dynamic:
        {
            d3d11_usage = D3D11_USAGE_DEFAULT;
            access_flags = D3D11_CPU_ACCESS_WRITE;
        }break;

        default: break;
    }

    D3D11_TEXTURE2D_DESC texture_desc = {};
    {
        texture_desc.Width            = UINT(size.x);
        texture_desc.Height           = UINT(size.y);
        texture_desc.MipLevels        = 1;
        texture_desc.ArraySize        = 1;
        texture_desc.Format           = Render_D3D11_GetFormat(format);
        texture_desc.SampleDesc.Count = 1;
        texture_desc.Usage            = d3d11_usage;
        texture_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
        texture_desc.CPUAccessFlags   = access_flags;
    }

    D3D11_SUBRESOURCE_DATA initial_subrsrc_data = {};
    {
        initial_subrsrc_data.pSysMem          = initial_data;
        initial_subrsrc_data.SysMemPitch      = UINT(Render_BytesPerPixel(format) * size.x);
        initial_subrsrc_data.SysMemSlicePitch = 0;
    }

    Render_D3D11_Tex2D texture = {0};

    Sys_Mutex(render_d3d11_state->device_mutex)
    {
        render_d3d11_state->device->CreateTexture2D(&texture_desc, initial_data ? &initial_subrsrc_data : 0, &texture.texture);
        render_d3d11_state->device->CreateShaderResourceView(texture.texture, 0, &texture.view);
    }

    texture.size = {i32(size.x), i32(size.y)};
    texture.format = format;

    Render_Hnd result = Render_D3D11_GetHandle(texture);
    return result;
}

function void
Render_ReleaseTex2D(Render_Hnd texture_handle)
{
    Sys_Mutex(render_d3d11_state->device_mutex)
    {
        Render_D3D11_Tex2D tex = Render_D3D11_GetTex2D(texture_handle);
        tex.texture->Release();
        tex.view->Release();
    }
}

function void
Render_FillRegionTex2D(Render_Hnd texture_handle, Rect2_i64 region, void* data)
{
    Sys_Mutex(render_d3d11_state->device_mutex)
    {
        Render_D3D11_Tex2D tex = Render_D3D11_GetTex2D(texture_handle);
        u64 bytes_per_pixel = Render_BytesPerPixel(tex.format);
        Vec2_i64 dim = Dimensions(region);

        // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_box
        D3D11_BOX dst_box =
        {
            UINT(region.min.x), UINT(region.min.y), 0,
            UINT(region.max.x), UINT(region.max.y), 1,
        };
        render_d3d11_state->device_ctx->UpdateSubresource(tex.texture, 0, &dst_box, data, UINT(dim.x * bytes_per_pixel), 0);
    }
}

function Render_Hnd
Render_Attach(Sys_Hnd window_handle)
{
    Render_D3D11_WindowAttach* render_window = (Render_D3D11_WindowAttach*)Sys_ReserveMemory(sizeof(Render_D3D11_WindowAttach));
    Sys_CommitMemory(render_window, sizeof(*render_window));

    Sys_Win32_Window* window = Sys_Win32_GetWindow(window_handle);
    HWND hWnd = window->hWnd;

    Sys_Mutex(render_d3d11_state->device_mutex)
    {
        //
        // https://gist.github.com/d7samurai/261c69490cce0620d0bfc93003cd1052
        //

        // SwapChain
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        {
            swapchain_desc.Width              = 0; // use window width
            swapchain_desc.Height             = 0; // use window height
            swapchain_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapchain_desc.Stereo             = FALSE;
            swapchain_desc.SampleDesc.Count   = 1;
            swapchain_desc.SampleDesc.Quality = 0;
            swapchain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchain_desc.BufferCount        = 2;
            swapchain_desc.Scaling            = DXGI_SCALING_STRETCH;
            swapchain_desc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
            swapchain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
            swapchain_desc.Flags              = 0;
        }

        render_d3d11_state->factory_dxgi->CreateSwapChainForHwnd(
            render_d3d11_state->device,
            hWnd,
            &swapchain_desc,
            0,                               // NULL -> windowed swap chain
            0,                               // NULL -> don't restrict content to output target (e.g. a monitor)
            &render_window->swapchain
        );

        // Render Target
        render_window->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)(&render_window->framebuffer));
        render_d3d11_state->device->CreateRenderTargetView(render_window->framebuffer, 0, &render_window->framebuffer_rtv);
    }

    Render_Hnd result = Render_D3D11_GetHandle(render_window);
    return result;
}

function void
Render_Dettach(Sys_Hnd window_handle, Render_Hnd render_handle)
{
    Render_D3D11_WindowAttach* render_window = Render_D3D11_GetWindowAttach(render_handle);

    #define ReleaseResource(v) if (render_window->v) { render_window->v->Release(); }
    ReleaseResource(gbuffer_depth_srv)
    ReleaseResource(gbuffer_depth_dsv)
    ReleaseResource(gbuffer_depth)
    ReleaseResource(gbuffer_color_srv)
    ReleaseResource(gbuffer_color_rtv)
    ReleaseResource(gbuffer_color)
    ReleaseResource(gbuffer_scratch_color_srv)
    ReleaseResource(gbuffer_scratch_color_rtv)
    ReleaseResource(gbuffer_scratch_color)
    ReleaseResource(framebuffer_rtv)
    ReleaseResource(framebuffer)
    ReleaseResource(swapchain)
    #undef ReleaseResource

    Sys_ReleaseMemory(render_window, sizeof(*render_window));
}

function void
Render_Initialise()
{
    //
    // https://gist.github.com/d7samurai/261c69490cce0620d0bfc93003cd1052
    //

    if (IsCurrentThreadMain() && !render_d3d11_state)
    {
        Arena* arena = ArenaAlloc(Gigabytes(16));
        render_d3d11_state = PushArray(arena, Render_D3D11_State, 1);
        render_d3d11_state->arena = arena;
        render_d3d11_state->device_mutex = Sys_MutexAlloc();
        
        // Base Device
        // TODO: setup debug layer/validation/message queue
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Required for Direct2D interoperability with Direct3D resources.
        D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};

        D3D11CreateDevice(
            0,                                     // use first adapter from IDXGIFactory1::EnumAdapters
            D3D_DRIVER_TYPE_HARDWARE,              // are we using hardware or software rendering
            0,                                     // software rasteriser dll ( if software rendering )
            flags,
            feature_levels,
            ArrayCount(feature_levels),
            D3D11_SDK_VERSION,                     // use D3D11_SDK_VERSION
            &render_d3d11_state->base_device,
            0,                                     // for storing returned D3D_FEATURE_LEVEL
            &render_d3d11_state->base_device_ctx
        );

        // Device
        render_d3d11_state->base_device->QueryInterface(__uuidof(ID3D11Device1), (void **)(&render_d3d11_state->device));
        render_d3d11_state->base_device_ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)(&render_d3d11_state->device_ctx));

        // DXGI
        render_d3d11_state->device->QueryInterface(__uuidof(IDXGIDevice1), (void **)(&render_d3d11_state->device_dxgi));
        render_d3d11_state->device_dxgi->GetAdapter(&render_d3d11_state->adapter_dxgi);
        render_d3d11_state->adapter_dxgi->GetParent(__uuidof(IDXGIFactory2), (void **)(&render_d3d11_state->factory_dxgi));

        // Rasterizer
        {
            D3D11_RASTERIZER_DESC1 rasterizer_desc = {};
            {
                rasterizer_desc.FillMode      = D3D11_FILL_SOLID;
                rasterizer_desc.CullMode      = D3D11_CULL_BACK;
                rasterizer_desc.ScissorEnable = 1;
            }
            render_d3d11_state->device->CreateRasterizerState1(&rasterizer_desc, &render_d3d11_state->rasterizer);
        }

        // Blend States
        {
            D3D11_BLEND_DESC default_blend_desc = {};
            {
                default_blend_desc.RenderTarget[0].BlendEnable            = 1;
                default_blend_desc.RenderTarget[0].SrcBlend               = D3D11_BLEND_SRC_ALPHA;
                default_blend_desc.RenderTarget[0].DestBlend              = D3D11_BLEND_INV_SRC_ALPHA;
                default_blend_desc.RenderTarget[0].BlendOp                = D3D11_BLEND_OP_ADD;
                default_blend_desc.RenderTarget[0].SrcBlendAlpha          = D3D11_BLEND_ONE;
                default_blend_desc.RenderTarget[0].DestBlendAlpha         = D3D11_BLEND_ZERO;
                default_blend_desc.RenderTarget[0].BlendOpAlpha           = D3D11_BLEND_OP_ADD;
                default_blend_desc.RenderTarget[0].RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;
            }
            render_d3d11_state->device->CreateBlendState(&default_blend_desc, &render_d3d11_state->blend_state_default);

            D3D11_BLEND_DESC additive_blend_desc = {};
            {
                additive_blend_desc.RenderTarget[0].BlendEnable           = 1;
                additive_blend_desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
                additive_blend_desc.RenderTarget[0].DestBlend             = D3D11_BLEND_ONE; 
                additive_blend_desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
                additive_blend_desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ZERO;
                additive_blend_desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
                additive_blend_desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
                additive_blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            }
            render_d3d11_state->device->CreateBlendState(&additive_blend_desc, &render_d3d11_state->blend_state_additive);     
        }

        // Samplers
        {
            D3D11_SAMPLER_DESC sampler_nearest = {};
            {
                sampler_nearest.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
                sampler_nearest.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_nearest.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_nearest.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_nearest.ComparisonFunc = D3D11_COMPARISON_NEVER;
            }
            render_d3d11_state->device->CreateSamplerState(&sampler_nearest, &render_d3d11_state->sampler_nearest);

            D3D11_SAMPLER_DESC sampler_linear = {};
            {
                sampler_linear.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                sampler_linear.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_linear.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_linear.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
                sampler_linear.ComparisonFunc = D3D11_COMPARISON_NEVER;
            }
            render_d3d11_state->device->CreateSamplerState(&sampler_linear, &render_d3d11_state->sampler_linear);
        }

        // Depth/Stencil
        {
                D3D11_DEPTH_STENCIL_DESC noop_depth = {};
                {
                    noop_depth.DepthEnable    = FALSE;
                    noop_depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                    noop_depth.DepthFunc      = D3D11_COMPARISON_LESS;
                }
                render_d3d11_state->device->CreateDepthStencilState(&noop_depth, &render_d3d11_state->noop_ds_state);

                D3D11_DEPTH_STENCIL_DESC plain_depth = {};
                {
                    plain_depth.DepthEnable    = TRUE;
                    plain_depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                    plain_depth.DepthFunc      = D3D11_COMPARISON_LESS;
                }
                render_d3d11_state->device->CreateDepthStencilState(&plain_depth, &render_d3d11_state->plain_ds_state);

                D3D11_DEPTH_STENCIL_DESC write_depth = {};
                {
                    write_depth.DepthEnable    = FALSE;
                    write_depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                    write_depth.DepthFunc      = D3D11_COMPARISON_LESS;
                }
                render_d3d11_state->device->CreateDepthStencilState(&write_depth, &render_d3d11_state->write_ds_state);

                D3D11_DEPTH_STENCIL_DESC read = {};
                {
                    read.DepthEnable    = TRUE;
                    read.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
                    read.DepthFunc      = D3D11_COMPARISON_LESS;
                }
                render_d3d11_state->device->CreateDepthStencilState(&read, &render_d3d11_state->read_ds_state);
        }

        // Buffers
        for(Render_D3D11_CommandType kind = (Render_D3D11_CommandType)(Render_D3D11_Command_Nil+1);
                                 kind <  Render_D3D11_Command_Count;
                                 kind = (Render_D3D11_CommandType)(kind+1))
        {
            D3D11_BUFFER_DESC desc = {0};
            {
                UINT byte_width = UINT(render_d3d11_command_table[kind].size);
                desc.ByteWidth      = ((byte_width + 15) / 16) * 16;
                desc.Usage          = D3D11_USAGE_DYNAMIC;
                desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            render_d3d11_state->device->CreateBuffer(&desc, 0, &render_d3d11_state->global_buffer_table[kind]);
        }

        // Rect2D Shader
        {
            // Vertex Shader
            ID3DBlob* vtx_source_blob = 0;
            ID3DBlob* vtx_errors_blob = 0;
            ID3D11VertexShader* vtx_shader = 0;
            Str8         vtx_shader_errors = {};
            {
                LPCWSTR file_name = L"rect2d.hlsl";
                D3D_SHADER_MACRO* defines   = 0;
                ID3DInclude*      includer  = 0;
                LPCSTR            entry     = "vs_main";
                LPCSTR            target    = "vs_5_0";
                UINT              flags1    = 0;
                UINT              flags2    = 0;
                D3DCompileFromFile(file_name, defines, includer, entry, target, flags1, flags2, &vtx_source_blob, &vtx_errors_blob);

                if (vtx_errors_blob)
                {
                    vtx_shader_errors = String8((u8*)vtx_errors_blob->GetBufferPointer(), (u64)vtx_errors_blob->GetBufferSize());
                }
                else
                {
                    render_d3d11_state->device->CreateVertexShader(
                        vtx_source_blob->GetBufferPointer(),
                        vtx_source_blob->GetBufferSize(),
                        0,
                        &vtx_shader
                    );  
                }
            }

            // Input Layout
            ID3D11InputLayout* input_layout = 0;
            {
                render_d3d11_state->device->CreateInputLayout(
                    render_d3d11_rect2d_input_layout,
                    ArrayCount(render_d3d11_rect2d_input_layout),
                    vtx_source_blob->GetBufferPointer(),
                    vtx_source_blob->GetBufferSize(),
                    &input_layout);
            }

            // Pixel Shader
            ID3DBlob* pixel_source_blob = 0;
            ID3DBlob* pixel_errors_blob = 0;
            ID3D11PixelShader* pixel_shader = 0;
            Str8        pixel_shader_errors = {};
            {
                LPCWSTR file_name = L"rect2d.hlsl";
                D3D_SHADER_MACRO* defines   = 0;
                ID3DInclude*      includer  = 0;
                LPCSTR            entry     = "ps_main";
                LPCSTR            target    = "ps_5_0";
                UINT              flags1    = 0;
                UINT              flags2    = 0;
                D3DCompileFromFile(file_name, defines, includer, entry, target, flags1, flags2, &pixel_source_blob, &pixel_errors_blob);

                if (pixel_errors_blob)
                {
                    pixel_shader_errors = String8((u8*)pixel_errors_blob->GetBufferPointer(), (u64)pixel_errors_blob->GetBufferSize());
                }
                else
                {
                    render_d3d11_state->device->CreatePixelShader(
                        pixel_source_blob->GetBufferPointer(),
                        pixel_source_blob->GetBufferSize(),
                        0,
                        &pixel_shader
                    );  
                }
            }

            render_d3d11_state->input_layout_table[Render_D3D11_Shader_Rect2D]  = input_layout;
            render_d3d11_state->shader_table_vertex[Render_D3D11_Shader_Rect2D] = vtx_shader;
            render_d3d11_state->shader_table_pixel[Render_D3D11_Shader_Rect2D]  = pixel_shader;
        }

        // Scratch Buffers
        {
            D3D11_BUFFER_DESC buffer_64KB = {};
            {
                buffer_64KB.ByteWidth      = Kilobytes(64);
                buffer_64KB.Usage          = D3D11_USAGE_DYNAMIC;
                buffer_64KB.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
                buffer_64KB.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            render_d3d11_state->device->CreateBuffer(&buffer_64KB, 0, &render_d3d11_state->scratch_64kb);
        
            D3D11_BUFFER_DESC buffer_1MB = {};
            {
                buffer_1MB.ByteWidth      = Megabytes(1);
                buffer_1MB.Usage          = D3D11_USAGE_DYNAMIC;
                buffer_1MB.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
                buffer_1MB.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            render_d3d11_state->device->CreateBuffer(&buffer_1MB, 0, &render_d3d11_state->scratch_1mb);

            D3D11_BUFFER_DESC buffer_2MB = {};
            {
                buffer_2MB.ByteWidth      = Megabytes(2);
                buffer_2MB.Usage          = D3D11_USAGE_DYNAMIC;
                buffer_2MB.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
                buffer_2MB.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            render_d3d11_state->device->CreateBuffer(&buffer_2MB, 0, &render_d3d11_state->scratch_2mb);
        
            D3D11_BUFFER_DESC buffer_4MB = {};
            {
                buffer_4MB.ByteWidth      = Megabytes(4);
                buffer_4MB.Usage          = D3D11_USAGE_DYNAMIC;
                buffer_4MB.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
                buffer_4MB.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            render_d3d11_state->device->CreateBuffer(&buffer_4MB, 0, &render_d3d11_state->scratch_4mb);
        
            D3D11_BUFFER_DESC buffer_8MB = {};
            {
                buffer_8MB.ByteWidth      = Megabytes(8);
                buffer_8MB.Usage          = D3D11_USAGE_DYNAMIC;
                buffer_8MB.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
                buffer_8MB.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            render_d3d11_state->device->CreateBuffer(&buffer_8MB, 0, &render_d3d11_state->scratch_8mb);
        }

        // white texture
        u32 white_tex = 0xFFFFFFFF;
        render_d3d11_state->white_texture = Render_AllocTex2D({1, 1}, Render_Tex2D_RGBA8, Render_Tex2DUsage_Static, &white_tex);

        // backup texture
        u8 backup_tex[] =
        {
            0xff, 0x00, 0xff, 0xff, 0x11, 0x00, 0x11, 0xff,
            0x11, 0x00, 0x11, 0xff, 0xff, 0x00, 0xff, 0xff,
        };
        render_d3d11_state->backup_texture = Render_AllocTex2D({2, 2}, Render_Tex2D_RGBA8, Render_Tex2DUsage_Static, &backup_tex[0]);

        render_d3d11_state->overflow_arena = ArenaAlloc(Kilobytes(256));
    }
}