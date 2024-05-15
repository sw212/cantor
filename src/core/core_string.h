#ifndef CORE_STRING_H
#define CORE_STRING_H

#pragma warning(push, 0)
#include "stb_sprintf.h"
#pragma warning(pop)
#include <inttypes.h>


struct Str8
{
    u8* str;
    u64 size;
};

struct Buf8
{
    u8* ptr;
    u64 size;
};

struct Str16
{
    u16* str;
    u64  size;
};

struct Str32
{
    u32* str;
    u64  size;
};

struct Str8Node
{
    Str8Node* next;
    Str8      string;
};

struct Str8List
{
    Str8Node* first;
    Str8Node* last;
    u64       node_count;
    u64       total_size;
};

struct Str8Array
{
    u64   count;
    Str8* v;
};

typedef u32 StrMatchFlag;
enum
{
    StrMatch_i       = (1<<0), // ignore case 
    StrMatch_iLength = (1<<1), // allow length differences
    StrMatch_iSlash  = (1<<2), // forward/backward slash
    StrMatch_Last    = (1<<3), // find last
};

function u8  Upper(u8 c);
function u8  Lower(u8 c);
function u8  CharFwdSlash(u8 c);
function u64 StrLength(char* str);
function u64 StrLength(u16*  str);

function Str8 String8(u8* str, u64 size);
function Str8 String8(Arena* arena, Str16 string);
function Str8 String8(Arena* arena, Str32 string);
function Str8 String8(char* str);
#define StringLiteral(str) String8((u8*)(str), sizeof(str) - 1)
#define Buf8Struct(ptr) Buf8{(u8*)(ptr), sizeof(*(ptr))}

function Str16 String16(u16* str, u64 size);
function Str16 String16(Arena* arena, Str8 string);
function Str16 String16(u16* str);

function Str32 String32(u32* str, u64 size);
function Str32 String32(Arena* arena, Str8 string);

function Str8 Substr(Str8 string, u64 start, u64 end);
function Str8 StrSkip(Str8 string, u64 min);
function Str8 StrPrefix(Str8 string, u64 size);

function b32 StrMatch(Str8 A, Str8 B, StrMatchFlag flags);
function u64 FindSubstr(Str8 string, Str8 substr, u64 start_pos, StrMatchFlag flags);

function Str8 StrPushCopy(Arena* arena, Str8 string);
function Str8 StrPushFV(Arena* arena, char* fmt, va_list args);
function Str8 StrPushF(Arena* arena, char* fmt, ...);
function Str8 StrPushFillByte(Arena *arena, u64 size, u8 byte);

function void StrListPushNode(Str8List* list, Str8Node* node);
function void StrListPush(Arena* arena, Str8List* list, Str8 string);

function Str8 StrTrimToLastSlash(Str8 string);

#endif