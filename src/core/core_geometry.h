#ifndef CORE_GEOMETRY_H
#define CORE_GEOMETRY_H

    struct VertexIndexArray
    {
        Array<Vec3_f32> vertices;
        Array<i32>      indices;
    };


    function VertexIndexArray GeometryIcosphere(Arena* arena);
#endif