function UI_Action
UI_Text(Str8 string)
{
    UI_Wig* wig = UI_WigCreate(UI_Wig_HasText, StringLiteral(""));
    UI_WigSetDisplayString(wig, string);
    UI_Action result = UI_GetAction(wig);

    return result;
}

function UI_Action
UI_Text(char *format_str, ...)
{
    Scratch scratch = ScratchBegin(0, 0);
    va_list args;

    va_start(args, format_str);
    Str8 string = StrPushFV(scratch.arena, format_str, args);
    UI_Action result = UI_Text(string);
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

function UI_Action
UI_Button(Str8 string)
{
    UI_WigFlags flags = 
       UI_Wig_DrawBG |
       UI_Wig_HasText |
       UI_Wig_Interactable;

    UI_Wig* wig = UI_WigCreate(flags, string);
    UI_Action result = UI_GetAction(wig);

    return result;
}

function UI_Action
UI_Button(char* format_str, ...)
{
    Scratch scratch = ScratchBegin(0, 0);

    va_list args;
    va_start(args, format_str);
    Str8 string = StrPushFV(scratch.arena, format_str, args);
    UI_Action result = UI_Button(string);
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
UI_BeginColumn(char *format_str, ...)
{
    Scratch scratch = ScratchBegin(0, 0);

    va_list args;
    va_start(args, format_str);
    Str8 string = StrPushFV(scratch.arena, format_str, args);
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

//
// struct UI_CustomData ?
UI_DRAW_CUSTOM(UI_DrawCustomComponent)
{

}

function void
UI_CustomComponent()
{
    UI_Wig* wig = UI_WigCreate(UI_Wig_AllowMouse, StringLiteral("custom"));
    UI_Action action = UI_GetAction(wig);

    wig->flags |= UI_Wig_Custom;
    wig->custom =
    {
        .fn = UI_DrawCustomComponent,
        .data = (void*)1,
    };
}


