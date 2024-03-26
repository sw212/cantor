#ifndef CORE_MATH_H
#define CORE_MATH_H

template <typename T>
struct Array
{
    T*  v;
    u64 count;
};

union Vec2_f32
{
    struct { f32 x, y; };
    f32 v[2];
};

union Vec2_i16
{
    struct { i16 x, y; };
    i16 v[2];
};

union Vec2_i32
{
    struct { i32 x, y; };
    i32 v[2];
};

union Vec2_i64
{
    struct { i64 x, y; };
    i64 v[2];
};

union Vec3_f32
{
    struct { f32 x, y, z; };
    f32 v[3];
};

union Vec3_i32
{
    struct { i32 x, y, z; };
    i32 v[3];
};

union Vec3_i64
{
    struct { i64 x, y, z; };
    i64 v[3];
};

union Vec4_f32
{
    struct { f32 x, y, z, w; };
    f32 v[4];
};

union Vec4_i32
{
    struct { i32 x, y, z, w; };
    i32 v[4];
};

union Vec4_i64
{
    struct { i64 x, y, z, w; };
    i64 v[4];
};

struct Mat3x3_f32
{
    f32 v[3][3];
};

struct Mat4x4_f32
{
    f32 v[4][4];
};

struct Mat3x3_f64
{
    f64 v[3][3];
};

struct Mat4x4_f64
{
    f64 v[4][4];
};

union Rect2_f32
{
    struct { Vec2_f32 min; Vec2_f32 max; };
    Vec2_f32 v[2];
};

union Rect2_i64
{
    struct { Vec2_i64 min; Vec2_i64 max; };
    Vec2_i64 v[2];
};



// - Float
function f32 Floor(f32 a);
function f32 Ceil(f32 a);
function f32 Abs(f32 a);
function f32 Sqrt(f32 a);
function f32 Square(f32 a);


// - Vec
function f32 Length(Vec2_f32 a);
function Vec2_f32 Add(Vec2_f32 a, Vec2_f32 b);
function Vec3_f32 Add(Vec3_f32 a, Vec3_f32 b);
function Vec4_f32 Add(Vec4_f32 a, Vec4_f32 b);


// - Matrix
function Mat4x4_f32 operator*(Mat4x4_f32 a, Mat4x4_f32 b);

function Mat4x4_f32 Identity4(f32 v);
function Mat4x4_f32 Inverse(Mat4x4_f32 a);


// - Rect
function Rect2_f32 Pad(Rect2_f32 a, f32 v);
function b32 Contains(Rect2_f32 r, Vec2_f32 p);
function Rect2_f32 Intersection(Rect2_f32 a, Rect2_f32 b);
function Vec2_f32 Dimensions(Rect2_f32 r);

#endif