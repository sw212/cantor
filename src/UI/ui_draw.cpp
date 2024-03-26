function void
UI_Draw()
{
    Sys_Hnd window_handle = UI_Window();


    for (UI_Wig* wig = UI_Root(), *next_wig = &ui_nil_wig; !UI_WigIsNil(wig); wig = next_wig)
    {
        UI_WigTraversal rec = UI_TraverseRevPreOrder(wig, &ui_nil_wig, offsetof(UI_Wig, prev), offsetof(UI_Wig, last));
        next_wig = rec.next;

        // Background
        if (wig->flags &UI_Wig_DrawBackground)
        {
            Draw_RectParams p = {.color = {1.f, 1.f, 1.f, 1.f}};
            Render_Rect2D* rect = Draw_Rect(Pad(wig->rect, 1.f), &p);
            
            // Background Color
            rect->colors[Corner_00] = wig->style->color_background;
            rect->colors[Corner_01] = wig->style->color_background;
            rect->colors[Corner_10] = wig->style->color_background;
            rect->colors[Corner_11] = wig->style->color_background;

            // TODO:
            // more styling!
        }

        if (wig->flags & UI_Wig_DrawText)
        {
            Arena* draw_arena = Draw_GetArena();
            Scratch   scratch = ScratchBegin(&draw_arena, 1);

            Vec2_f32 text_pos = UI_GetTextPos(wig);
            Str8   wig_string = UI_WigGetDisplayString(wig);

            Str8 trail = StringLiteral("...");
            b32 truncate = 0;

            if (!(wig->flags & UI_Wig_IgnoreTruncate))
            {
                f32 trail_adv = Font_GetAdvance(wig->text->font_tag, wig->text->font_size, trail);
                f32 max_size = wig->calculated_size.x - wig->text->padding * 2.f;

                if (trail_adv > max_size)
                {
                    wig_string = StringLiteral("");
                }
                else
                {
                    Str8 truncated = Font_GetTruncatedString(wig->text->font_tag,
                                                             wig->text->font_size,
                                                             wig_string,
                                                             max_size,
                                                             trail_adv);
                    truncate = (truncated.size < wig_string.size);
                    wig_string = truncated;
                }
            }

            f32 offset = Draw_Text(text_pos, wig->text->font_tag, wig->text->font_size, wig->text->color_text, wig_string);

            if (truncate)
            {
                Draw_Text(Vec2_f32{text_pos.x + offset, text_pos.y}, wig->text->font_tag, wig->text->font_size, wig->text->color_text, trail);
            }
            
            ScratchEnd(scratch);
        }

        // Clipping
        if (wig->flags & UI_Wig_Clip)
        {
            Rect2_f32 curr_clip = Draw_TopClip();
            Rect2_f32 new_clip  = wig->rect;

            Vec2_f32  clip_dims = Dimensions(curr_clip);

            if (clip_dims.x != 0.f || clip_dims.y != 0.f)
            {
                new_clip = Intersection(curr_clip, new_clip);
            }

            Draw_PushClip(new_clip);
        }

        // Pop Clipping
        if (!rec.push_count)
        {
            i32 pop_idx = 0;

            for (UI_Wig* p = wig; !UI_WigIsNil(p) && p != next_wig && pop_idx <= rec.pop_count; p = p->parent, pop_idx++)
            {
                if (p->flags &UI_Wig_Clip)
                {
                    Draw_PopClip();
                }
            }
        }
    }
}