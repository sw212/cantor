#ifndef CORE_DEFINES_H
#define CORE_DEFINES_H

#include <stdint.h>
#include <math.h>

#include <intrin.h>

#pragma warning(push, 0)
#include "meow_hash_x64_aesni.h"
#pragma warning(pop)

#define function static
#define Global   static

# define per_thread __declspec(thread)

# pragma section(".ro", read)
# define read_only __declspec(allocate(".ro"))

// -
#define Assert(x) do { if(!(x)) { __debugbreak(); } } while(0)

// -
#define MemoryCopy memcpy
#define MemoryMove memmove
#define MemorySet  memset

#define MemoryCopyStruct(dst, src) do { Assert(sizeof(*(dst)) == sizeof(*(src))); MemoryCopy((dst), (src), sizeof(*(dst))); } while(0)
#define MemoryCopyArray(dst, src)  do { Assert(sizeof(dst) == sizeof(src)); MemoryCopy((dst), (src), sizeof(src)); }while(0)

#define MemoryZero(ptr, size) MemorySet((ptr), 0, (size))
#define MemoryZeroStruct(ptr) MemoryZero((ptr), sizeof(*(ptr)))
#define MemoryZeroArray(arr)  MemoryZero((arr), sizeof(arr))

// -
#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))
#define IntFromPtr(p) (U64)(((U8*)p) - 0)
#define PtrFromInt(i) (void*)(((U8*)0) + i)
#define Member(type, member_name) ((type *)0)->member_name
#define OffsetOf(type, member_name) IntFromPtr(&Member(type, member_name))
#define BaseFromMember(type, member_name, ptr) (type *)((U8 *)(ptr) - OffsetOf(type, member_name))

// -
#define Bytes(n)      (n)
#define Kilobytes(n)  (n << 10)
#define Megabytes(n)  (n << 20)
#define Gigabytes(n)  (((u64)n) << 30)
#define Terabytes(n)  (((u64)n) << 40)

#define Thousand(n)   ((n)*1000)
#define Million(n)    ((n)*1000000)
#define Billion(n)    ((n)*1000000000LL)

// -
#define Swap(type, a, b) do{ type _swapper_ = a; a = b; b = _swapper_; }while(0)

// -
#define CheckNull(p) ((p)==0)
#define SetNull(p) ((p)=0)

// -
#define QueuePush_NZ(first,last,node,next,zchk,zset)\
(\
    zchk(first) ? (((first) = (last) = (node)), zset((node)->next)) : \
                  ((last)->next=(node), (last) = (node), zset((node)->next))\
)

#define QueuePushFront_NZ(first,last,node,next,zchk,zset)\
(\
    zchk(first) ? (((first) = (last) = (node)), zset((node)->next)) : \
                  ((node)->next = (first)), ((first) = (node))\
)

#define QueuePop_NZ(first,last,next,zset)\
(\
    (first)==(last) ? (zset(first), zset(last)) : \
                      ((first) = (first)->next)\
)

#define StackPush_N(first,node,next) ((node)->next = (first), (first)=(node))
#define StackPop_NZ(first,next,zchk) (zchk(first) ? 0 : ((first)=(first)->next))

// -
#define DLLInsert_NPZ(first, last, at, node, next, prev, zero_check, zero_set)\
(\
    zero_check(first) ? (((first) = (last) = (node)), zero_set((node)->next), zero_set((node)->prev)) : \
    zero_check(at)    ? (zero_set((node)->prev), (node)->next = (first), (zero_check(first) ? (0) : ((first)->prev = (node))), (first) = (node)) :\
                        ((zero_check((at)->next) ? (0) : (((at)->next->prev) = (node))), (node)->next = (at)->next, (node)->prev = (at), (at)->next = (node),((at) == (last) ? (last) = (node) : (0)))\
)

#define DLLRemove_NPZ(first,last,node,next,prev,zchk,zset)\
(\
        ((first)==(node)) ? ((first)=(first)->next, (zchk(first) ? (zset(last)) : zset((first)->prev))) : \
        ((last)==(node))  ? ((last)=(last)->prev, (zchk(last) ? (zset(first)) : zset((last)->next))) : \
                            ((zchk((node)->next) ? (0) : ((node)->next->prev=(node)->prev)), (zchk((node)->prev) ? (0) : ((node)->prev->next=(node)->next)))\
)

#define DLLPushBack_NPZ(first, last, node, next, prev, zchk, zset) DLLInsert_NPZ(first, last, last, node, next, prev, zchk, zset)


// -
#define QueuePush(f,l,n)         QueuePush_NZ(f,l,n,next,CheckNull,SetNull)
#define QueuePushFront(f,l,n)    QueuePushFront_NZ(f,l,n,next,CheckNull,SetNull)
#define QueuePop(f,l)            QueuePop_NZ(f,l,next,SetNull)
#define StackPush(f,n)           StackPush_N(f,n,next)
#define StackPop(f)              StackPop_NZ(f,next,CheckNull)
#define DLLPushBack(f,l,n)       DLLPushBack_NPZ(f,l,n,next,prev,CheckNull,SetNull)
#define DLLPushFront(f,l,n)      DLLPushBack_NPZ(l,f,n,prev,next,CheckNull,SetNull)
#define DLLInsert(f,l,p,n)       DLLInsert_NPZ(f,l,p,n,next,prev,CheckNull,SetNull)
#define DLLRemove(f,l,n)         DLLRemove_NPZ(f,l,n,next,prev,CheckNull,SetNull)

// -
#define Min(a, b) (((a)<(b)) ? (a) : (b))
#define Max(a, b) (((a)>(b)) ? (a) : (b))
#define Clamp(a, x, b) (((a)>(x))?(a):((b)<(x))?(b):(x))

// -
#define ScopedContextBlock(start, end) for(int _i_ = ((start), 0); _i_ == 0; _i_ += 1, (end))
#define ScopedContextBlockChecked(begin, end) for(int _i_ = 2 * !(begin); (_i_ == 2 ? ((end), 0) : !_i_); _i_ += 1, (end))

// -
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef i32      b32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
struct u128
{
    u64 v[2];
};
inline function b32 U128_Match(u128 a, u128 b) {return a.v[0] == b.v[0] && a.v[1] == b.v[1];}

typedef float    f32;
typedef double   f64;

Global u32 SignF32             = 0x80000000;
Global u32 ExponentF32         = 0x7F800000;
Global u32 MantissaF32         = 0x7FFFFF;

Global u64 SignF64     = 0x8000000000000000ull;
Global u64 ExponentF64 = 0x7FF0000000000000ull;
Global u64 MantissaF64 = 0xFFFFFFFFFFFFFull;

enum Corner
{
    Corner_Invalid = -1,
    Corner_00,
    Corner_01,
    Corner_10,
    Corner_11,
    Corner_Count
};

enum Axis2
{
    Axis2_Invalid = -1,
    Axis2_X,
    Axis2_Y,
    Axis2_Count,
};
#define Axis2_Flip(a) ((Axis2)(!(a)))


#endif