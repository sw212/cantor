// - Float

function f32
Floor(f32 a)
{
    f32 result = floorf(a);
    return result;
}

function f32
Ceil(f32 a)
{
    f32 result = ceilf(a);
    return result;
}

function f32
Abs(f32 a)
{
    union { u32 u; f32 f; } result;
    result.f = a;
    result.u = result.u & ~SignF32;
    return result.f;
}
function i64
Abs(i64 a)
{
    i64 result = llabs(a);
    return result;
}

function f32
Sqrt(f32 a)
{
    f32 result = sqrtf(a);
    return result;
}

function f32
Square(f32 a)
{
    f32 result = a*a;
    return result;
}


// - Vec

function f32
Length(Vec2_f32 a)
{
    f32 result = Sqrt(Square(a.x) + Square(a.y));
    return result;
}

function Vec2_f32
Add(Vec2_f32 a, Vec2_f32 b)
{
    Vec2_f32 result = {a.x + b.x, a.y + b.y};
    return result;
}
function Vec3_f32
Add(Vec3_f32 a, Vec3_f32 b)
{
    Vec3_f32 result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}
function Vec4_f32
Add(Vec4_f32 a, Vec4_f32 b)
{
    Vec4_f32 result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    return result;
}


// - Matrix

function Mat4x4_f32
operator*(Mat4x4_f32 a, Mat4x4_f32 b)
{
    Mat4x4_f32 result = {};

    for (i32 r = 0; r < 4; r++)
    {
        for (i32 c = 0; c < 4; c++)
        {
            for (i32 k = 0; k < 4; k++)
            {
                result.v[r][c] = (a.v[r][k] * b.v[k][c]);
            }
        }
    }

    return result;
}

function Mat4x4_f32
Identity4(f32 v = 1.f)
{
    Mat4x4_f32 result = 
    {
        v, 0.f, 0.f, 0.f,
        0.f, v, 0.f, 0.f,
        0.f, 0.f, v, 0.f,
        0.f, 0.f, 0.f, v,
    };
    return result;
}

function Mat4x4_f32
Inverse(Mat4x4_f32 a)
{
    Mat4x4_f32 result = {};

    result.v[0][0] =  a.v[1][1] * a.v[2][2] * a.v[3][3] - a.v[1][1] * a.v[2][3] * a.v[3][2] - a.v[2][1] * a.v[1][2] * a.v[3][3] + a.v[2][1] * a.v[1][3] * a.v[3][2] + a.v[3][1] * a.v[1][2] * a.v[2][3] - a.v[3][1] * a.v[1][3] * a.v[2][2];
    result.v[1][0] = -a.v[1][0] * a.v[2][2] * a.v[3][3] + a.v[1][0] * a.v[2][3] * a.v[3][2] + a.v[2][0] * a.v[1][2] * a.v[3][3] - a.v[2][0] * a.v[1][3] * a.v[3][2] - a.v[3][0] * a.v[1][2] * a.v[2][3] + a.v[3][0] * a.v[1][3] * a.v[2][2];
    result.v[2][0] =  a.v[1][0] * a.v[2][1]  * a.v[3][3] - a.v[1][0] * a.v[2][3] * a.v[3][1] - a.v[2][0] * a.v[1][1] * a.v[3][3] + a.v[2][0] * a.v[1][3] * a.v[3][1] + a.v[3][0] * a.v[1][1] * a.v[2][3] - a.v[3][0] * a.v[1][3] * a.v[2][1];
    result.v[3][0] = -a.v[1][0] * a.v[2][1]  * a.v[3][2] + a.v[1][0] * a.v[2][2] * a.v[3][1] + a.v[2][0] * a.v[1][1] * a.v[3][2] - a.v[2][0] * a.v[1][2] * a.v[3][1] - a.v[3][0] * a.v[1][1] * a.v[2][2] + a.v[3][0] * a.v[1][2] * a.v[2][1];
    result.v[0][1] = -a.v[0][1] * a.v[2][2] * a.v[3][3] + a.v[0][1] * a.v[2][3] * a.v[3][2] + a.v[2][1] * a.v[0][2] * a.v[3][3] - a.v[2][1] * a.v[0][3] * a.v[3][2] - a.v[3][1] * a.v[0][2] * a.v[2][3] + a.v[3][1] * a.v[0][3] * a.v[2][2];
    result.v[1][1] =  a.v[0][0] * a.v[2][2] * a.v[3][3] - a.v[0][0] * a.v[2][3] * a.v[3][2] - a.v[2][0] * a.v[0][2] * a.v[3][3] + a.v[2][0] * a.v[0][3] * a.v[3][2] + a.v[3][0] * a.v[0][2] * a.v[2][3] - a.v[3][0] * a.v[0][3] * a.v[2][2];
    result.v[2][1] = -a.v[0][0] * a.v[2][1]  * a.v[3][3] + a.v[0][0] * a.v[2][3] * a.v[3][1] + a.v[2][0] * a.v[0][1] * a.v[3][3] - a.v[2][0] * a.v[0][3] * a.v[3][1] - a.v[3][0] * a.v[0][1] * a.v[2][3] + a.v[3][0] * a.v[0][3] * a.v[2][1];
    result.v[3][1] =  a.v[0][0] * a.v[2][1]  * a.v[3][2] - a.v[0][0] * a.v[2][2] * a.v[3][1] - a.v[2][0] * a.v[0][1] * a.v[3][2] + a.v[2][0] * a.v[0][2] * a.v[3][1] + a.v[3][0] * a.v[0][1] * a.v[2][2] - a.v[3][0] * a.v[0][2] * a.v[2][1];
    result.v[0][2] =  a.v[0][1] * a.v[1][2]  * a.v[3][3] - a.v[0][1] * a.v[1][3]  * a.v[3][2] - a.v[1][1] * a.v[0][2] * a.v[3][3] + a.v[1][1] * a.v[0][3] * a.v[3][2] + a.v[3][1] * a.v[0][2] * a.v[1][3]  - a.v[3][1] * a.v[0][3] * a.v[1][2];
    result.v[1][2] = -a.v[0][0] * a.v[1][2]  * a.v[3][3] + a.v[0][0] * a.v[1][3]  * a.v[3][2] + a.v[1][0] * a.v[0][2] * a.v[3][3] - a.v[1][0] * a.v[0][3] * a.v[3][2] - a.v[3][0] * a.v[0][2] * a.v[1][3]  + a.v[3][0] * a.v[0][3] * a.v[1][2];
    result.v[2][2] =  a.v[0][0] * a.v[1][1]  * a.v[3][3] - a.v[0][0] * a.v[1][3]  * a.v[3][1] - a.v[1][0] * a.v[0][1] * a.v[3][3] + a.v[1][0] * a.v[0][3] * a.v[3][1] + a.v[3][0] * a.v[0][1] * a.v[1][3]  - a.v[3][0] * a.v[0][3] * a.v[1][1];
    result.v[3][2] = -a.v[0][0] * a.v[1][1]  * a.v[3][2] + a.v[0][0] * a.v[1][2]  * a.v[3][1] + a.v[1][0] * a.v[0][1] * a.v[3][2] - a.v[1][0] * a.v[0][2] * a.v[3][1] - a.v[3][0] * a.v[0][1] * a.v[1][2]  + a.v[3][0] * a.v[0][2] * a.v[1][1];
    result.v[0][3] = -a.v[0][1] * a.v[1][2]  * a.v[2][3] + a.v[0][1] * a.v[1][3]  * a.v[2][2] + a.v[1][1] * a.v[0][2] * a.v[2][3] - a.v[1][1] * a.v[0][3] * a.v[2][2] - a.v[2][1]  * a.v[0][2] * a.v[1][3]  + a.v[2][1]  * a.v[0][3] * a.v[1][2];
    result.v[1][3] =  a.v[0][0] * a.v[1][2]  * a.v[2][3] - a.v[0][0] * a.v[1][3]  * a.v[2][2] - a.v[1][0] * a.v[0][2] * a.v[2][3] + a.v[1][0] * a.v[0][3] * a.v[2][2] + a.v[2][0]  * a.v[0][2] * a.v[1][3]  - a.v[2][0]  * a.v[0][3] * a.v[1][2];
    result.v[2][3] = -a.v[0][0] * a.v[1][1]  * a.v[2][3] + a.v[0][0] * a.v[1][3]  * a.v[2][1]  + a.v[1][0] * a.v[0][1] * a.v[2][3] - a.v[1][0] * a.v[0][3] * a.v[2][1]  - a.v[2][0]  * a.v[0][1] * a.v[1][3]  + a.v[2][0]  * a.v[0][3] * a.v[1][1];
    result.v[3][3] =  a.v[0][0] * a.v[1][1]  * a.v[2][2] - a.v[0][0] * a.v[1][2]  * a.v[2][1]  - a.v[1][0] * a.v[0][1] * a.v[2][2] + a.v[1][0] * a.v[0][2] * a.v[2][1]  + a.v[2][0]  * a.v[0][1] * a.v[1][2]  - a.v[2][0]  * a.v[0][2] * a.v[1][1];

    f32 det = a.v[0][0] * result.v[0][0] + a.v[0][1] * result.v[1][0] + a.v[0][2] * result.v[2][0] + a.v[0][3] * result.v[3][0];
    f32 inv_det = 1.0f / det;

    for (i32 r = 0; r < 16; r++)
    {
        for (i32 c = 0; c < 16; c++)
        {
            result.v[r][c] *= inv_det;
        }
    }

    return result;
}


// - Rect

function Rect2_f32
Pad(Rect2_f32 a, f32 v)
{
    Rect2_f32 result = 
    {
        .min = {a.min.x - v, a.min.y - v},
        .max = {a.max.x + v, a.max.y + v}
    };
    return result;
}

function b32
Contains(Rect2_f32 r, Vec2_f32 p)
{
    b32 result = ((r.min.x <= p.x) && (p.x <= r.max.x)) &&
                 ((r.min.y <= p.y) && (p.y <= r.max.y));
    return result;
}

function Rect2_f32
Intersection(Rect2_f32 a, Rect2_f32 b)
{
    Rect2_f32 result = {
        .min = {Max(a.min.x, b.min.x), Max(a.min.y, b.min.y)},
        .max = {Min(a.max.x, b.max.x), Min(a.max.y, b.max.y)},
    };
    return result;
}

function Vec2_f32
Dimensions(Rect2_f32 r)
{
    Vec2_f32 result = {Abs(r.max.x - r.min.x), Abs(r.max.y - r.min.y)};
    return result;
}
function Vec2_i64
Dimensions(Rect2_i64 r)
{
    Vec2_i64 result = {Abs(r.max.x - r.min.x), Abs(r.max.y - r.min.y)};
    return result;
}