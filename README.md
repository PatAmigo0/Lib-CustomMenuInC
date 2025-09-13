# Windows Console Menu Library

This lightweight library provides a simple, high-performance menu system for Windows console applications. It features a highly optimized rendering engine, customizable menus, keyboard and mouse navigation, and a clean abstraction layer.

## Version

`0.9.4 STABLE`

-----

## ✨ NEW IN VERSION 0.9.4 STABLE ✨

  - **Optimized Rendering Engine**: A new partial redraw system dramatically improves performance by only updating changed elements, eliminating flickering on selection changes. 🚀
  - **Efficient Memory Management**: The library now pre-allocates memory for menus and options more efficiently, reducing expensive `realloc` calls and improving speed when adding options dynamically.
  - **Robust Text Alignment**: Added support for **UTF-8** character counting, ensuring menu elements are perfectly centered and aligned, even with non-ASCII characters.
  - **Refactored Color API**: The color management API has been modernized for better type safety and clarity.
  - **Improved Input Stability**: Enhanced input handling to prevent missed or ghost inputs, especially on menu initialization.

-----

## Features

  - **Windows-only implementation** (uses Windows API)
  - Customizable headers and footers
  - Colorful menu options with highlighting (VT100 & Legacy)
  - Keyboard navigation (arrow keys + Enter)
  - Mouse navigation (toggleable)
  - Advanced color customization with macros / RGB colors
  - Double buffering for smooth, flicker-free rendering
  - **NEW**: Optimized partial screen redraws for maximum performance
  - **NEW**: UTF-8 support for accurate text alignment
  - Dynamically centers the menu in the console
  - Legacy console support (Windows 7/8 compatibility) with automatic VT100 detection

-----

## Installation

1.  Add `menu.h` and `menu.c` to your project.
2.  Include the header in your code:
    ```c
    #include "menu.h"
    ```

-----

## Requirements

  - Windows operating system (Windows 7+)
  - C99 compatible compiler
  - Standard Windows libraries

-----

## Usage

### Basic Setup

```c
#include "menu.h"

void menu_callback(MENU menu, dpointer data)
{
    char* message = (char*)data;
    printf("Selected: %s\n", message);
    getchar();
}

int main()
{
    MENU main_menu = create_menu();

    // Create menu items with associated data
    add_option(main_menu, create_menu_item("Option 1", menu_callback, "First option"));
    add_option(main_menu, create_menu_item("Option 2", menu_callback, "Second option"));
    add_option(main_menu, create_menu_item("Exit", menu_callback, "Exiting..."));

    change_header(main_menu, "MAIN MENU");
    change_footer(main_menu, "Navigate with arrow keys or mouse");

    // Enable mouse support
    toggle_mouse(main_menu);

    enable_menu(main_menu);
    
    // Use clear_menu() or clear_menus() if menus are no more need
    // clear_menus_and_exit(); 
    return 0;
}
```

### Advanced Color & Legacy Mode Example

```c
#include "menu.h"

void callback(MENU menu, void* data)
{
    printf("Callback executed!\n");
    getchar();
    // To close the menu after callback, you can call clear_menu()
    // clear_menu(menu);
}

int main()
{
    // === Global Settings Configuration ===

    // Create a new settings object to force legacy mode
    MENU_SETTINGS settings = create_new_settings();
    settings.force_legacy_mode = 1; // Force legacy color system and rendering (e.g., for Win 7 or win 10 old consoles)
    set_default_menu_settings(settings);

    // --- VT100 Color Settings (WILL BE IGNORED in legacy mode) ---
    MENU_COLOR modern_colors = create_color_object();
    modern_colors.headerColor = new_full_rgb_color(mrgb(255, 255, 255), mrgb(90, 0, 157)); // White on Purple
    modern_colors.footerColor = new_full_rgb_color(mrgb(255, 255, 255), mrgb(90, 0, 157));
    modern_colors.optionColor = new_full_rgb_color(mrgb(0, 0, 0), mrgb(200, 200, 200));   // Black on Light Gray
    set_default_color_object(modern_colors);
    
    // --- Legacy Color Settings (WILL BE USED because of force_legacy_mode) ---
    LEGACY_MENU_COLOR legacy_colors = create_legacy_color_object();
    legacy_colors.headerColor = BRIGHT_MAGENTA;
    legacy_colors.footerColor = BRIGHT_MAGENTA;
    legacy_colors.optionColor = HIGHLIGHT_COLOR; // White on Blue
    set_default_legacy_color_object(legacy_colors);

    // === Menu Creation ===
    MENU menu = create_menu();
    add_option(menu, create_menu_item("Options", callback, NULL));
    add_option(menu, create_menu_item("Exit", callback, NULL));
    
    enable_menu(menu);
    return 0;
}
```

-----

## Core Functions Reference

### Menu Creation & Management

1.  **`MENU create_menu()`** Creates and returns a new menu object.
2.  **`void enable_menu(MENU menu)`** Activates and displays the menu, entering its main loop.
3.  **`void clear_menu(MENU menu)`** Frees all resources for a specific menu.
4.  **`void clear_menus()`** Destroys all created menus.
5.  **`void clear_menus_and_exit()`** Destroys all menus and exits the program with code 0.

### Menu Item Management

6.  **`MENU_ITEM create_menu_item(const char* text, __menu_callback callback, void* data)`** Creates a menu item with text, a callback function, and associated user data.
7.  **`void add_option(MENU menu, const MENU_ITEM item)`** Adds a menu item to a menu.
8.  **`void clear_option(MENU menu, MENU_ITEM option)`** Removes and frees a specific menu item from a menu.

### Appearance Customization

9.  **`void change_header(MENU menu, const char* text)`** Sets the menu header text.
10. **`void change_footer(MENU menu, const char* text)`** Sets the menu footer text.
11. **`void change_menu_policy(MENU menu, int header_policy, int footer_policy)`** Controls header/footer visibility (1 = show, 0 = hide).

### Input Settings

12. **`void toggle_mouse(MENU menu)`** Toggles mouse input support for a specific menu.

### Configuration

13. **`MENU_SETTINGS create_new_settings()`** Creates a new settings object with default values.
14. **`void set_menu_settings(MENU menu, MENU_SETTINGS settings)`** Applies custom settings to a specific menu.
15. **`void set_default_menu_settings(MENU_SETTINGS settings)`** Sets the default settings for all newly created menus.

### VT100 / RGB Color Management

16. **`MENU_COLOR create_color_object()`** Creates a new color object with default colors.
17. **`void set_color_object(MENU menu, MENU_COLOR color_object)`** Applies a color scheme to a specific menu.
18. **`void set_default_color_object(MENU_COLOR color_object)`** Sets the default color scheme for new menus.

### Legacy Color Management

19. **`LEGACY_MENU_COLOR create_legacy_color_object()`** Creates a new legacy color object.
20. **`void set_legacy_color_object(MENU menu, LEGACY_MENU_COLOR color_object)`** Applies a legacy color scheme to a specific menu.
21. **`void set_default_legacy_color_object(LEGACY_MENU_COLOR color_object)`** Sets the default legacy color scheme for new menus.

### RGB Color Helpers

22. **`MENU_RGB_COLOR mrgb(short r, short g, short b)`** Creates an RGB color structure.
23. **`COLOR_OBJECT_PROPERTY new_rgb_color(int text_color, MENU_RGB_COLOR color)`** Returns a color property for either foreground (`text_color = 1`) or background (`text_color = 0`).
24. **`COLOR_OBJECT_PROPERTY new_full_rgb_color(MENU_RGB_COLOR fg, MENU_RGB_COLOR bg)`** Returns a color property for a complete foreground and background pair.

-----

## Building

Compile your application source file with `menu.c`:

```bash
gcc your_app.c menu.c -o your_app.exe
```

-----

## Important Notes

1.  **API Changes in 0.9.4 STABLE** The color management API has been refactored. Instead of passing char buffers, functions like `new_full_rgb_color` now return a `COLOR_OBJECT_PROPERTY` struct, which should be assigned directly (e.g., `my_colors.headerColor = new_full_rgb_color(...)`).

2.  **Memory Management** - Use `clear_menu()` to free a menu's resources if you need to close it from a callback.

      - `clear_menus_and_exit()` is a convenient way to clean up before program termination.
      - The library now manages memory for options in blocks, making `add_option` more efficient.

3.  **Performance** - The new rendering engine is extremely fast and avoids redrawing the entire screen on simple updates like selection changes.

      - Mouse input is well-optimized.

4.  **Compatibility** - **VT100 mode** (with full RGB color) requires Windows 10/11.

      - **Legacy mode** works on Windows 7 and newer.
      - The library automatically detects VT100 support but can be overridden with `settings.force_legacy_mode = 1;`.
      - It's good practice to define both a modern `MENU_COLOR` and a `LEGACY_MENU_COLOR` scheme for your application to ensure it looks great on all target systems.

5.  **Error Handling** - The library automatically handles console resizing and displays a user-friendly message if the window is too small, pausing until it's resized appropriately.

-----

## Contributing

Contributions are welcome\! Please submit pull requests or open issues on GitHub.
