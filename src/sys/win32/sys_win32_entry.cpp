function void EntryPoint();

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR lp_cmd_line, int n_show_cmd)
{
    sys_win32_hinstance = instance;
    Main(EntryPoint, (u64)__argc, __argv);
    return 0;
}