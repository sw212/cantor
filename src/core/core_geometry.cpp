function VertexIndexArray
Geometry_Icosphere(Arena* arena)
{
    VertexIndexArray result = {};

    static read_only Vec3_f32 vertex_array[] =
    {
        #define size 1.618034f
        {-1.f, +size, +0.f},
        {+1.f, +size, +0.f},
        {-1.f, -size, +0.f},
        {+1.f, -size, +0.f},
        {+0.f, -1.f, +size},
        {+0.f, +1.f, +size},
        {+0.f, -1.f, -size},
        {+0.f, +1.f, -size},
        {+size, +0.f, -1.f},
        {+size, +0.f, +1.f},
        {-size, +0.f, -1.f},
        {-size, +0.f, +1.f},
        #undef size
    };

    static i32 index_array[] =
    {
        0, 11, 5, 0, 5, 1, 0, 1, 7,
        0, 7, 10, 0, 10, 11, 11, 10, 2,
        5, 11, 4, 1, 5, 9, 7, 1, 8,
        10, 7, 6, 3, 9, 4, 3, 4, 2,
        3, 2, 6, 3, 6, 8, 3, 8, 9,
        9, 8, 1, 4, 9, 5, 2, 4, 11,
        6, 2, 10, 8, 6, 7
    };

    u64 vertex_count = ArrayCount(vertex_array);
    u64 index_count  = ArrayCount(index_array);

    result.vertices.v = PushArrayNZ(arena, Vec3_f32, vertex_count);
    result.indices.v  = PushArrayNZ(arena, i32     , index_count);

    result.vertices.count = vertex_count;
    result.indices.count  = index_count;

    MemoryCopy(result.vertices.v, vertex_array, sizeof(vertex_array));
    MemoryCopy(result.indices.v , index_array , sizeof(index_array));

    return result;
}
