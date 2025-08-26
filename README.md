# Windows Console Menu Library

This lightweight library provides a simple, easy-to-use menu system for Windows console applications. It features customizable menus, keyboard and mouse navigation, and a clean abstraction layer.

## Version
```0.9.4 BETA```

## NEW IN VERSION 0.9.4
- **Legacy Console Support**: Partitial compatibility with Windows 7/8 consoles
- **Automatic VT100 Detection**: Automatically detects and uses appropriate rendering mode
- **Force Legacy Mode**: Manual override for legacy color systems
- **Enhanced Error Handling**: Better error messages for both VT100 and legacy modes
- **Memory Optimizations**: Reduced memory footprint and improved performance

## Features

- **Windows-only implementation** (uses Windows API)
- Customizable headers and footers texts
- Colorful menu options with highlighting
- Keyboard navigation (arrow keys + Enter)
- Mouse navigation (toggleable)
- Advanced color customization with macros / rgb colors
- Default settings configuration
- Double buffering for smooth rendering
- Selectable place for the menu
- Callback functions with data passing
- Dynamically centers the menu in the console
- Clear abstraction layer
- **NEW**: Legacy console support (Windows 7/8 compatibility) (automatic VT100 detection)

## Installation

1. Add `menu.h` and `menu.c` to your project
2. Include the header in your code:
```c
#include "menu.h"
```

## Requirements

- Windows operating system (Windows 7+)
- C99 compatible compiler
- Standard Windows libraries

## Usage

### Basic Setup

```c
#include "menu.h"

void menu_callback(MENU menu, dpointer data)
{
    char* message = (char*)data;
    printf("%s\n", message);
    getchar();
}

int main()
{
    MENU main_menu = create_menu();

    // create menu items with associated data
    add_option(main_menu, create_menu_item("Option 1", menu_callback, "First option selected"));
    add_option(main_menu, create_menu_item("Option 2", menu_callback, "Second option selected"));

    change_header(main_menu, "MAIN MENU");
    change_footer(main_menu, "Navigate with arrow keys or mouse");

    // enable mouse support
    toggle_mouse(main_menu);

    enable_menu(main_menu);
}
```

### NEW: Force Legacy Mode Example

```c
#include "menu.h"

void callback(MENU menu, void* data)
{
    printf(":)");
    getchar();
}

int main()
{
    // force legacy mode if you like it!
    MENU_SETTINGS settings = create_new_settings();
    settings.force_legacy_mode = 1; // Force legacy color system
    set_default_menu_settings(settings);

    // this won't do anything in legacy mode
    MENU_COLOR new_color = create_color_object();
    strcpy(new_color.headerColor, MAGENTA_BG WHITE_TEXT);
    new_full_rgb_color(rgb(45, 230, 0), rgb(143, 32, 255), new_color.footerColor);
    set_default_color_object(new_color);

    // this won't do anything in non legacy mode
    LEGACY_MENU_COLOR legacy_colors = create_legacy_color_object();
    legacy_colors.headerColor = BRIGHT_MAGENTA;
    legacy_colors.footerColor = BRIGHT_MAGENTA;
    legacy_colors.optionColor = HIGHLIGHT_COLOR;
    set_default_legacy_color_object(legacy_colors);
    
    MENU menu = create_menu();
    add_option(menu, create_menu_item("Options", callback, NULL));
    add_option(menu, create_menu_item("Exit", callback, NULL));
    
    enable_menu(menu);
    return 0;
}
```

## Core Functions Reference

### Menu Creation & Management
1. **`MENU create_menu()`**  
   Creates and returns a new menu object.

2. **`void enable_menu(MENU menu)`**  
   Activates and displays the menu.

3. **`void clear_menu(MENU menu)`**  
   Frees all resources (menu struct, items, and buffers)

4. **`void clear_menus()`**  
   Destroys all created menus.

5. **`void clear_menus_and_exit()`**  
   Destroys all menus and exits the program.

### Menu Item Management
6. **`MENU_ITEM create_menu_item(const char* text, __menu_callback callback, void* data)`**  
   Creates a menu item with text, callback, and associated data.

7. **`int add_option(MENU menu, const MENU_ITEM item)`**  
   Adds a menu item to a menu.

8. **`void clear_option(MENU menu, MENU_ITEM option)`**  
   Removes a menu item from a menu.

### Appearance Customization
9. **`void change_header(MENU menu, const char* text)`**  
   Sets the menu header text.

10. **`void change_footer(MENU menu, const char* text)`**  
    Sets the menu footer text.

11. **`void change_menu_policy(MENU menu, int header_policy, int footer_policy)`**  
    Controls header/footer visibility (1 = show, 0 = hide).

### Input Settings
12. **`void toggle_mouse(MENU menu)`**  
    Toggles mouse input support.

### Configuration
13. **`MENU_SETTINGS create_new_settings()`**  
    Creates a new settings object with default values.
    
14. **`void set_menu_settings(MENU menu, MENU_SETTINGS settings)`** 
    Applies custom settings to a specific menu.

15. **`void set_default_menu_settings(MENU_SETTINGS settings)`**  
    Sets default settings for new menus.

### Color Management
16. **`MENU_COLOR create_color_object()`**  
    Creates a new color object with default colors.

17. **`void set_color_object(MENU menu, MENU_COLOR color_object)`**  
    Applies color scheme to a specific menu.

18. **`void set_default_color_object(MENU_COLOR color_object)`**  
    Sets default color scheme for new menus.

### NEW: Legacy Color Management
19. **`LEGACY_MENU_COLOR create_legacy_color_object()`**  
    Creates a new legacy color object with default colors.

20. **`void set_default_legacy_color_object(LEGACY_MENU_COLOR color_object)`**  
    Sets default legacy color scheme for new menus.

### RGB Color Functions
21. **`MENU_RGB_COLOR rgb(short r, short g, short b)`**  
    Creates an RGB color object with specified components (0-255).

22. **`void new_rgb_color(int text_color, MENU_RGB_COLOR color, char output[MAX_RGB_LEN])`**  
    Generates ANSI escape sequence for a single RGB color.

23. **`void new_full_rgb_color(MENU_RGB_COLOR fg, MENU_RGB_COLOR bg, char output[MAX_RGB_LEN])`**  
    Generates ANSI sequence for both foreground and background RGB colors.

## Building

Compile with your project:
```bash
gcc <your_app.c> menu.c -o your_app
```

## IMPORTANT NOTES

1. **Memory Management**  
   - Always use `clear_menu()` to properly free menu resources
   - Never hold persistent pointers to menu items - they're managed internally
   - Call `clear_menus_and_exit()` for proper cleanup on program termination

2. **Legacy Mode Considerations**  
   - Legacy mode uses Windows console attributes instead of ANSI codes
   - Color customization is more limited in legacy mode
   - Legacy colors are defined using Windows `WORD` values, not RGB

3. **Performance**  
   - Menus with many options may render slower on non legacy systems
   - Mouse input is well optimized

4. **Compatibility**  
   - VT100 mode requires Windows 10 or newer
   - Legacy mode works on Windows 7 and newer
   - Test both modes if targeting multiple Windows versions
   - It's always better to have 2 styles for your menu

5. **Error Handling**  
   - The library will automatically handle most console compatibility issues
   - Error messages adapt to the current rendering mode (VT100 or legacy)
   - Size validation ensures menus fit the console window

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.