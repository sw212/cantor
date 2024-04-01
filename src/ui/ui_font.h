#ifndef UI_FONT_EMBED_H
#define UI_FONT_EMBED_H

extern u8 ui_font_buffer[108168];
read_only Str8 ui_font_data = {ui_font_buffer, sizeof(ui_font_buffer)};
read_only u128 ui_font_hash = {0x722bd4ddab2e3f81, 0x5823f44d6d645746};

#endif