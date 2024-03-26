#define StringLiteralInit(str) {(u8*)str, sizeof(str) - 1}

Str8 sys_key_table[Sys_Key_Count] =
{
    StringLiteralInit("Null"),
    StringLiteralInit("Escape"),
    StringLiteralInit("F1"),
    StringLiteralInit("F2"),
    StringLiteralInit("F3"),
    StringLiteralInit("F4"),
    StringLiteralInit("F5"),
    StringLiteralInit("F6"),
    StringLiteralInit("F7"),
    StringLiteralInit("F8"),
    StringLiteralInit("F9"),
    StringLiteralInit("F10"),
    StringLiteralInit("F11"),
    StringLiteralInit("F12"),
    StringLiteralInit("F13"),
    StringLiteralInit("F14"),
    StringLiteralInit("F15"),
    StringLiteralInit("F16"),
    StringLiteralInit("F17"),
    StringLiteralInit("F18"),
    StringLiteralInit("F19"),
    StringLiteralInit("F20"),
    StringLiteralInit("F21"),
    StringLiteralInit("F22"),
    StringLiteralInit("F23"),
    StringLiteralInit("F24"),
    StringLiteralInit("Grave Accent"),
    StringLiteralInit("0"),
    StringLiteralInit("1"),
    StringLiteralInit("2"),
    StringLiteralInit("3"),
    StringLiteralInit("4"),
    StringLiteralInit("5"),
    StringLiteralInit("6"),
    StringLiteralInit("7"),
    StringLiteralInit("8"),
    StringLiteralInit("9"),
    StringLiteralInit("Minus"),
    StringLiteralInit("Equal"),
    StringLiteralInit("Backspace"),
    StringLiteralInit("Delete"),
    StringLiteralInit("Tab"),
    StringLiteralInit("A"),
    StringLiteralInit("B"),
    StringLiteralInit("C"),
    StringLiteralInit("D"),
    StringLiteralInit("E"),
    StringLiteralInit("F"),
    StringLiteralInit("G"),
    StringLiteralInit("H"),
    StringLiteralInit("I"),
    StringLiteralInit("J"),
    StringLiteralInit("K"),
    StringLiteralInit("L"),
    StringLiteralInit("M"),
    StringLiteralInit("N"),
    StringLiteralInit("O"),
    StringLiteralInit("P"),
    StringLiteralInit("Q"),
    StringLiteralInit("R"),
    StringLiteralInit("S"),
    StringLiteralInit("T"),
    StringLiteralInit("U"),
    StringLiteralInit("V"),
    StringLiteralInit("W"),
    StringLiteralInit("X"),
    StringLiteralInit("Y"),
    StringLiteralInit("Z"),
    StringLiteralInit("Space"),
    StringLiteralInit("Enter"),
    StringLiteralInit("Ctrl"),
    StringLiteralInit("Shift"),
    StringLiteralInit("Alt"),
    StringLiteralInit("Up"),
    StringLiteralInit("Left"),
    StringLiteralInit("Down"),
    StringLiteralInit("Right"),
    StringLiteralInit("Page Up"),
    StringLiteralInit("Page Down"),
    StringLiteralInit("Home"),
    StringLiteralInit("End"),
    StringLiteralInit("Forward Slash"),
    StringLiteralInit("Period"),
    StringLiteralInit("Comma"),
    StringLiteralInit("Quote"),
    StringLiteralInit("Left Bracket"),
    StringLiteralInit("Right Bracket"),
    StringLiteralInit("Insert"),
    StringLiteralInit("Left Mouse Button"),
    StringLiteralInit("Middle Mouse Button"),
    StringLiteralInit("Right Mouse Button"),
    StringLiteralInit("Semicolon"),
};

#undef StringLiteralInit