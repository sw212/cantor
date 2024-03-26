function UI_Signal
UI_Text(Str8 string)
{
    UI_Wig* wig = UI_WigCreate(UI_Wig_DrawText, StringLiteral(""));
    UI_WigSetDisplayString(wig, string);
    UI_Signal result = UI_GetSignal(wig);

    return result;
}

function UI_Signal
UI_Text(char *fmt, ...)
{
    Scratch scratch = ScratchBegin(0, 0);
    va_list args;

    va_start(args, fmt);
    Str8 string = StrPushFV(scratch.arena, fmt, args);
    UI_Signal result = UI_Text(string);
    va_end(args);

    ScratchEnd(scratch);

    return result;
}

function void
UI_Space(UI_Size size)
{
    UI_Wig* parent = UI_GetParent();
    UI_SetNextDesiredSize(parent->layout_direction, size);
    UI_SetNextDesiredSize(Axis2_Flip(parent->layout_direction), UI_Pixels(0.f, 0.f));
    UI_Wig* wig = UI_WigCreate(0, StringLiteral(""));
}

function UI_Signal
UI_Button(Str8 string)
{
    UI_WigFlags flags = 
       UI_Wig_DrawBackground |
       UI_Wig_DrawText |
       UI_Wig_Clickable;

    UI_Wig* wig = UI_WigCreate(flags, string);
    UI_Signal result = UI_GetSignal(wig);

    return result;
}

function UI_Signal
UI_Button(char* fmt, ...)
{
    Scratch scratch = ScratchBegin(0, 0);

    va_list args;
    va_start(args, fmt);
    Str8 string = StrPushFV(scratch.arena, fmt, args);
    UI_Signal result = UI_Button(string);
    va_end(args);

    ScratchEnd(scratch);

    return result;
}

function void
UI_BeginColumn()
{
    UI_BeginColumn(StringLiteral(""));
}
function void
UI_BeginColumn(Str8 string)
{
    UI_SetNextLayoutDirection(Axis2_Y);
    UI_Wig* wig = UI_WigCreate(0, string);
    UI_SetParent(wig);
}
function void
UI_BeginColumn(char *fmt, ...)
{
    Scratch scratch = ScratchBegin(0, 0);

    va_list args;
    va_start(args, fmt);
    Str8 string = StrPushFV(scratch.arena, fmt, args);
    UI_BeginColumn(string);
    va_end(args);

    ScratchEnd(scratch);
}

function void
UI_EndColumn()
{
    UI_PopParent();
}

function void
UI_BeginRow()
{
    UI_BeginRow(StringLiteral(""));
}
function void
UI_BeginRow(Str8 string)
{
    UI_SetNextLayoutDirection(Axis2_X);
    UI_Wig* wig = UI_WigCreate(0, string);
    UI_SetParent(wig);
}
function void
UI_BeginRow(char *fmt, ...)
{
    Scratch scratch = ScratchBegin(0, 0);

    va_list args;
    va_start(args, fmt);
    Str8 string = StrPushFV(scratch.arena, fmt, args);
    UI_BeginRow(string);
    va_end(args);

    ScratchEnd(scratch);
}

function void
UI_RowEnd()
{
    UI_PopParent();
}
