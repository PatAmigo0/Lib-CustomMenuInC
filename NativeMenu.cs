using System;
using System.Runtime.InteropServices;

namespace MenuLibrary
{
    /// <summary>
    /// Хранит в себе координаты центра меню
    /// </summary>
    /// <remarks>
    /// <para>Этот тип является реализацией для C# структуры из C</para>
    /// <para>Пример на C#: <c>[StructLayout(LayoutKind.Sequential)]</c></para>
    /// <para>Оригинальная структура на C: <c>typedef struct { float X; float Y; } MENU_COORD;</c></para>
    /// </remarks>
    [StructLayout(LayoutKind.Sequential)]
    public struct MenuCoord
    {
        public float X;
        public float Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct MenuSettings
    {
        public int mouse_enabled;
        public int header_enabled;
        public int footer_enabled;
        public int double_width_enabled;
        public int force_legacy_mode;
        public MenuCoord menu_center;
        public int __garbage_collector;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct MenuRGBColor
    {
        public short r, g, b;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct ColorObjectProperty
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 45)]
        public string RgbSeq;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct MenuColor
    {
        public ColorObjectProperty headerColor;
        public ColorObjectProperty footerColor;
        public ColorObjectProperty optionColor;
    }
    public static class NativeMenu
    {
        private const string DllName = "menu.dll";

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void MenuCallback(IntPtr menu, IntPtr data);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr create_menu();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern IntPtr create_menu_item(string text, MenuCallback callback, IntPtr callback_data);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int add_option(IntPtr menu, IntPtr item);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void enable_menu(IntPtr menu);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void clear_menu(IntPtr menu);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void change_header(IntPtr menu, string text);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void toggle_mouse(IntPtr menu);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void set_menu_settings(IntPtr menu, MenuSettings settings);
    }
}