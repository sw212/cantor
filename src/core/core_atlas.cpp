function Atlas*
AtlasInitialise(Arena* arena, Vec2_i64 size)
{
    Atlas* result = PushArray(arena, Atlas, 1);
    result->size = size;
    
    result->root = PushArray(arena, AtlasNode, 1);
    result->root->max_space[Corner_00] = Vec2_i64{size.x / 2, size.y / 2};
    result->root->max_space[Corner_01] = Vec2_i64{size.x / 2, size.y / 2};
    result->root->max_space[Corner_10] = Vec2_i64{size.x / 2, size.y / 2};
    result->root->max_space[Corner_11] = Vec2_i64{size.x / 2, size.y / 2};

    return result;
}

function Rect2_i64
AtlasAllocate(Arena* arena, Atlas* atlas, Vec2_i64 size)
{
    Vec2_i64 result_bl   = {};
    Vec2_i64 result_size = {};

    AtlasNode* node = 0;
    Vec2_i64 node_size = atlas->size;
    Corner node_corner = Corner_Invalid;

    for (AtlasNode* n = atlas->root, *next = 0; n != 0; n = next, next = 0)
    {
        if (n->flags & AtlasFlag_Occupied)
        {
            break;
        }

        b32 node_is_empty = (n->child_count == 0);

        if (node_is_empty)
        {
            result_size = node_size;
        }

        Vec2_i64 child_size = {node_size.x / 2, node_size.y / 2};

        AtlasNode* next_child = 0;

        if ((child_size.x >= size.x) && (child_size.y >= size.y))
        {
            for (Corner c = (Corner)0; c < Corner_Count; c = Corner(c + 1))
            {
                if (!n->children[c])
                {
                    n->children[c] = PushArray(arena, AtlasNode, 1);
                    n->children[c]->parent = n;

                    Vec2_i64 inner_child_size = Vec2_i64{child_size.x / 2, child_size.y / 2};
                    n->children[c]->max_space[Corner_00] = inner_child_size;
                    n->children[c]->max_space[Corner_01] = inner_child_size;
                    n->children[c]->max_space[Corner_10] = inner_child_size;
                    n->children[c]->max_space[Corner_11] = inner_child_size;
                }

                if ((n->max_space[c].x >= size.x) && (n->max_space[c].y >= size.y))
                {
                    next_child = n->children[c];
                    node_corner = c;

                    Vec2_i64 corner_dir = {c / 2, c & 1};
                    result_bl.x += corner_dir.x * child_size.x;
                    result_bl.y += corner_dir.y * child_size.y;

                    break;
                }
            }
        }

        if (node_is_empty && !next_child)
        {
            node = n;
        }
        else
        {
            next = next_child;
            node_size = child_size;
        }
    }

    if (node && (node_corner != Corner_Invalid))
    {
        node->flags |= AtlasFlag_Occupied;
        
        if (node->parent)
        {
            MemoryZeroStruct(&node->parent->max_space[node_corner]);
        }

        for (AtlasNode* p = node->parent; p != 0; p = p->parent)
        {
            p->child_count++;

            AtlasNode* parent = p->parent;

            if (parent)
            {
                // 
                // best to calc p->parent->max_space in iter for p (i.e. this iter) because on next iter
                // we will have "forgotten" which corner we came from, meaning we would need
                // to recalculate max_space for all four quadrants instead of just the one corner
                //

                Corner parent_corner = Corner_Invalid;

                if (0) {}
                else if (p == parent->children[Corner_00]) { parent_corner = Corner_00; }
                else if (p == parent->children[Corner_01]) { parent_corner = Corner_01; }
                else if (p == parent->children[Corner_10]) { parent_corner = Corner_10; }
                else if (p == parent->children[Corner_11]) { parent_corner = Corner_11; }

                Assert(parent_corner != Corner_Invalid);

                Vec2_i64* p_max = p->max_space;
                parent->max_space[parent_corner].x = Max(Max(p_max[Corner_00].x, p_max[Corner_01].x), Max(p_max[Corner_10].x, p_max[Corner_11].x));
                parent->max_space[parent_corner].y = Max(Max(p_max[Corner_00].y, p_max[Corner_01].y), Max(p_max[Corner_10].y, p_max[Corner_11].y));
            }
        }
    }

    Rect2_i64 result = {.min = result_bl, .max = {result_bl.x + result_size.x, result_bl.y + result_size.y}};
    return result;
}

function void
AtlasRelease(Atlas* atlas, Rect2_i64 region)
{
    Vec2_i64 region_size = Dimensions(region);

    AtlasNode* node = 0;
    Vec2_i64 node_bl = {};
    Vec2_i64 node_size = atlas->size;
    Corner node_corner = Corner_Invalid;

    for (AtlasNode* n = atlas->root, *next = 0; n != 0; n = next)
    {
        if ((node_bl.x <= region.min.x && region.min.x < (node_bl.x + node_size.x)) &&
            (node_bl.y <= region.min.y && region.min.y < (node_bl.y + node_size.y)))
        {
            if ((node_bl.x == region.min.x && region_size.x == node_size.x) &&
                (node_bl.y == region.min.y && region_size.y == node_size.y))
            {
                node = n;
                break;
            }
            else
            {
                Vec2_i64 region_mid = {region.min.x + (region_size.x / 2), region.min.y + (region_size.y / 2)};
                Vec2_i64 node_mid = {node_bl.x + (node_size.x / 2), node_bl.y + (node_size.y / 2)};

                Corner c = Corner_Invalid;

                // find quadrant of region relative to node
                if (0) {}
                else if ((region_mid.x <= node_mid.x) && (region_mid.y <= node_mid.y)) { c = Corner_00; }
                else if ((region_mid.x <= node_mid.x) && (region_mid.y >= node_mid.y)) { c = Corner_01; }
                else if ((region_mid.x >= node_mid.x) && (region_mid.y <= node_mid.y)) { c = Corner_10; }
                else if ((region_mid.x >= node_mid.x) && (region_mid.y >= node_mid.y)) { c = Corner_11; }

                next = n->children[c];
                node_corner = c;

                node_size.x /= 2;
                node_size.y /= 2;

                Vec2_i64 corner_dir = { c / 2, c & 1 };
                node_bl.x += corner_dir.x * node_size.x;
                node_bl.y += corner_dir.y * node_size.y;
            }
        }
        else
        {
            break;
        }
    }

    if (node && (node_corner != Corner_Invalid))
    {
        node->flags &= ~AtlasFlag_Occupied;

        if (node->parent)
        {
            node->parent->max_space[node_corner] = region_size;
        }

        for (AtlasNode* p = node->parent; p != 0; p = p->parent)
        {
            p->child_count--;

            AtlasNode* parent = p->parent;

            if (parent)
            {
                Corner parent_corner = Corner_Invalid;

                if (0) {}
                else if (p == parent->children[Corner_00]) { parent_corner = Corner_00; }
                else if (p == parent->children[Corner_01]) { parent_corner = Corner_01; }
                else if (p == parent->children[Corner_10]) { parent_corner = Corner_10; }
                else if (p == parent->children[Corner_11]) { parent_corner = Corner_11; }

                Assert(parent_corner != Corner_Invalid);

                Vec2_i64* p_max = p->max_space;
                parent->max_space[parent_corner].x = Max(Max(p_max[Corner_00].x, p_max[Corner_01].x), Max(p_max[Corner_10].x, p_max[Corner_11].x));
                parent->max_space[parent_corner].y = Max(Max(p_max[Corner_00].y, p_max[Corner_01].y), Max(p_max[Corner_10].y, p_max[Corner_11].y));
            }
        }
    }
}
