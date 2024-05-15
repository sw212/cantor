#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

// - Helpers

function u8
Upper(u8 c)
{
    return (c >= 'a' && c <= 'z') ? c ^ 32 : c;
}

function u8
Lower(u8 c)
{
    return (c >= 'A' && c <= 'Z') ? c ^ 32 : c;
}

function u8
CharToForwardSlash(u8 c)
{
    return (c == '\\') ? '/' : c;
}

function u64
StrLength(char* str)
{
    //
    // returns length* not* including null char
    //

    u64 result = 0;
    while(str[result])
    {
        result++;
    }

    return result;
}

function u64
StrLength(u16* str)
{
    //
    // returns length* not* including null char
    //

    u64 result = 0;
    while(str[result])
    {
        result++;
    }

    return result;
}


// - String Constructors

function Str8
String8(u8* str, u64 size)
{
    Str8 result = {};
    result.str  = str;
    result.size = size;

    return result;
}

function Str8
String8(Arena* arena, Str16 string)
{
    u64 size = string.size;
    u8* str = PushArray(arena, u8, size + 1);

    u8 at  = 0;
    while(at < size)
    {
        str[at] = u8(string.str[at]);
        at++;
    }
    str[size] = 0;

    Str8 result = {str, size};
    return result;
}

function Str8
String8(Arena* arena, Str32 string)
{
    u64 size = string.size;
    u8* str = PushArray(arena, u8, size + 1);

    u8 at  = 0;
    while(at < size)
    {
        str[at] = u8(string.str[at]);
        at++;
    }
    str[size] = 0;

    Str8 result = {str, size};
    return result;
}

function Str8
String8(char* str)
{
    Str8 result = String8((u8*)str, StrLength(str));
    
    return result;
}

function Str16
String16(u16* str, u64 size)
{
    Str16 result = {};
    result.str  = str;
    result.size = size;

    return result;
}

function Str16
String16(Arena* arena, Str8 string)
{
    u64 size = string.size;
    u16* str = PushArray(arena, u16, size + 1);

    u8 at  = 0;
    while(at < size)
    {
        str[at] = string.str[at];
        at++;
    }
    str[size] = 0;

    Str16 result = {str, size};
    return result;
}

function Str16
String16(u16* str)
{
    Str16 result = String16(str, StrLength(str));
    
    return result;
}

function Str32
String32(u32* str, u64 size)
{
    Str32 result = {};
    result.str  = str;
    result.size = size;

    return result;
}

function Str32
String32(Arena* arena, Str8 string)
{
    u64 size = string.size;
    u32* str = PushArray(arena, u32, size + 1);

    u8 at  = 0;
    while(at < size)
    {
        str[at] = string.str[at];
        at++;
    }
    str[size] = 0;

    Str32 result = {str, size};
    return result;
}


// - Substrings

function Str8
Substr(Str8 string, u64 start, u64 end)
{
    u64 min = Min(start, string.size);
    u64 max = Min(end, string.size);

    Assert(min <= max);

    Str8 result = {};
    result.size = max - min;
    result.str  = string.str + min;

    return result;
}

function Str8
StrSkip(Str8 string, u64 min)
{
    Str8 result = Substr(string, Min(string.size, min), Max(string.size, min));
    return result;
}

function Str8
StrPrefix(Str8 string, u64 size)
{
    Str8 result = Substr(string, 0, size);
    return result;
}


// - String Match

function b32
StrMatch(Str8 a, Str8 b, StrMatchFlag flags)
{
    b32 result = 0;

    if ((a.size == b.size) || (flags & StrMatch_iLength))
    {
        result = 1;

        for (u64 i = 0; i < a.size; i++)
        {
            u8 l = a.str[i];
            u8 r = b.str[i];

            b32 match = (l == r);
            if (flags & StrMatch_i ) { match |= (Lower(l) == Lower(r)); }
            if (flags & StrMatch_iSlash) { match |= (CharToForwardSlash(l) == CharToForwardSlash(r)); }

            if (!match)
            {
                result = 0;
                break;
            }
        }
    }

    return result;
}

function u64
FindSubstr(Str8 string, Str8 substr, u64 start_pos, StrMatchFlag flags)
{
    //
    // finds string start index of matching substr
    //

    u64 result = string.size;

    for (u64 i = start_pos; i < string.size; i++)
    {
        if ((substr.size + i) <= string.size)
        {
            Str8 slice = Substr(string, i, substr.size + i);
            if (StrMatch(slice, substr, flags))
            {
                result = i;

                if (!(flags & StrMatch_Last))
                {
                    break;
                }
            }
        }
    }

    return result;
}


// - String Copy

function Str8
StrPushCopy(Arena* arena, Str8 string)
{
    Str8 result = {};
    result.size = string.size;
    result.str  = PushArray(arena, u8, string.size + 1);
    MemoryCopy(result.str, string.str, string.size);

    return result;
}

function Str8
StrPushFV(Arena* arena, char* format_str, va_list args)
{
    Str8 result = {};

    va_list args2;
    va_copy(args2, args);
    u32 needed_bytes = stbsp_vsnprintf(0, 0, format_str, args) + 1;
    result.str = PushArrayNZ(arena, u8, needed_bytes);
    result.size = needed_bytes - 1;
    stbsp_vsnprintf((char*)result.str, needed_bytes, format_str, args2);

    return result;
}

function Str8
StrPushF(Arena* arena, char* format_str, ...)
{
    Str8 result = {};

    va_list args;
    va_start(args, format_str);
    result = StrPushFV(arena, format_str, args);
    va_end(args);

    return result;
}


// - String List

function void
StrListPushNode(Str8List* list, Str8Node* node)
{
    QueuePush(list->first, list->last, node);
    list->node_count++;
    list->total_size += node->string.size;
}

function void
StrListPush(Arena* arena, Str8List* list, Str8 string)
{
    Str8Node* node = PushArray(arena, Str8Node, 1);
    node->string = string;
    StrListPushNode(list, node);
}


// - Paths
function Str8 StrTrimToLastSlash(Str8 string)
{
    u64 last_slash = FindSubstr(string, StringLiteral("/"), 0, StrMatch_Last | StrMatch_iSlash);

    if (last_slash < string.size)
    {
        string.size = last_slash + 1;
    }

    return string;
}