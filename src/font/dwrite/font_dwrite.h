#ifndef FONT_DWRITE_H
#define FONT_DWRITE_H

#pragma warning(push, 0)
#include <dwrite.h>
#pragma warning(pop)

#pragma comment(lib, "dwrite.lib")

struct Font_Core_DWrite_LoaderVT
{
    HRESULT (*QueryInterface)(void* this_ptr, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(void* this_ptr);
    ULONG   (*Release)(void* this_ptr);
    HRESULT (*CreateStreamFromKey)(struct Font_Core_DWrite_Loader* this_ptr, void* key, u32 key_size, IDWriteFontFileStream** out_stream);
};

struct Font_Core_DWrite_Loader
{
    Font_Core_DWrite_LoaderVT* lpVtbl;
};

struct Font_Core_DWrite_StreamVT
{
    HRESULT (*QueryInterface)(void* this_ptr, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(struct Font_Core_DWrite_Stream* this_ptr);
    ULONG   (*Release)(struct Font_Core_DWrite_Stream* this_ptr);
    HRESULT (*ReadFileFragment)(struct Font_Core_DWrite_Stream* this_ptr, void** fragment_start, u64 off, u64 size, void** fragment_ctx_out);
    void    (*ReleaseFileFragment)(struct Font_Core_DWrite_Stream* this_ptr, void* fragment_ctx);
    HRESULT (*GetFileSize)(struct Font_Core_DWrite_Stream* this_ptr, u64* out_file_size);
    HRESULT (*GetLastWriteTime)(struct Font_Core_DWrite_Stream* this_ptr, u64* out_time);
};

struct Font_Core_DWrite_Stream
{
    Font_Core_DWrite_StreamVT* lpVtbl;
    Font_Core_DWrite_Stream* next;
    Hash_Scope* scope;
    u128 hash;
    u64 ref_count;
};

struct Font_Core_DWrite_State
{
    Arena* arena;
    IDWriteFactory* factory;
    IDWriteRenderingParams*  default_render_params;
    IDWriteRenderingParams*  render_params;
    IDWriteGdiInterop*       gdi_interop;
    Font_Core_DWrite_Stream* free_stream;
};

struct Font_Core_DWrite
{
    IDWriteFontFile* file;
    IDWriteFontFace* face;
};


function HRESULT Font_Core_DWrite_NOPQueryInterface(void* this_ptr, REFIID riid, void** ppvObject);
function ULONG   Font_Core_DWrite_NOPAddRef(void* this_ptr);
function ULONG   Font_Core_DWrite_NOPRelease(void* this_ptr);

function HRESULT Font_Core_DWrite_CreateStream(Font_Core_DWrite_Loader* this_ptr, void* key, u32 key_size, IDWriteFontFileStream** out_stream);
function ULONG   Font_Core_DWrite_AddRef(Font_Core_DWrite_Stream* this_ptr);
function ULONG   Font_Core_DWrite_Release(Font_Core_DWrite_Stream* this_ptr);
function HRESULT Font_Core_DWrite_ReadFileFragment(Font_Core_DWrite_Stream* this_ptr, void** fragment_start, u64 off, u64 size, void** fragment_ctx_out);
function void    Font_Core_DWrite_ReleaseFileFragment(Font_Core_DWrite_Stream* this_ptr, void* fragment_ctx);
function HRESULT Font_Core_DWrite_GetFileSize(Font_Core_DWrite_Stream* this_ptr, u64* out_file_size);
function HRESULT Font_Core_DWrite_GetLastWriteTime(Font_Core_DWrite_Stream* this_ptr, u64* out_time);

function Font_Core_DWrite Font_Core_DWrite_GetFont(Font_Core_Hnd handle);
function Font_Core_Hnd    Font_Core_DWrite_GetHandle(Font_Core_DWrite font);

#endif