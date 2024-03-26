#ifndef UI_WIDGET_H
#define UI_WIDGET_H

function UI_Signal UI_Text(Str8 string);
function UI_Signal UI_Text(char* fmt, ...);

function void UI_Space(UI_Size size);
#define UI_Padding(size) ScopedContextBlock(UI_Space(size), UI_Space(size))

function UI_Signal UI_Button(Str8 string);
function UI_Signal UI_Button(char* fmt, ...);

function void UI_BeginColumn(Str8 string);
function void UI_BeginColumn(char* fmt, ...);
function void UI_BeginColumn();
function void UI_EndColumn();
#define UI_ColNamed(s)    ScopedContextBlock(UI_BeginColumn(s), UI_EndColumn())
#define UI_ColNamedF(...) ScopedContextBlock(UI_BeginColumn(__VA_ARGS__), UI_EndColumn())
#define UI_Col            ScopedContextBlock(UI_BeginColumn(), UI_EndColumn())

function void UI_BeginRow(Str8 string);
function void UI_BeginRow(char* fmt, ...);
function void UI_BeginRow();
function void UI_RowEnd();
#define UI_RowNamed(s)    ScopedContextBlock(UI_BeginRow(s), UI_RowEnd())
#define UI_RowNamedF(...) ScopedContextBlock(UI_BeginRow(__VA_ARGS__), UI_RowEnd())
#define UI_Row            ScopedContextBlock(UI_BeginRow(), UI_RowEnd())

#endif