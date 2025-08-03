# Windows Console Menu Library

This lightweight library provides a simple, easy-to-use menu system for Windows console applications. It features customizable menus, keyboard and mouse navigation, and a clean abstraction layer.

## Version
```0.9.3 BETA```

## Features

- **Windows-only implementation** (uses Windows API)
- Customizable headers and footers texts
- Colorful menu options with highlighting
- Keyboard navigation (arrow keys + Enter)
- Mouse navigation (toggleable)
- Advanced color customization with macros / rgb colors
- Default settings configuration
- Double buffering for smooth rendering
- Callback functions with data passing
- Dynamically centers the menu in the console
- Clear abstraction layer

## Installation

1. Add `menu.h` and `menu.c` to your project
2. Include the header in your code:
```c
#include "menu.h"
```

## Requirements

- Windows operating system
- C99 compatible compiler
- Standard Windows libraries
- Console supporting ANSI escape codes (Windows 10+)

## Included Libraries in Implementation
The library implementation (`menu.c`) includes the following essential headers:

```c
#include <stdio.h>    // standard I/O operations
#include <stdlib.h>   // memory allocation, exit()
#include <windows.h>  // windows API functions
#include <string.h>   // string manipulation
#include <stdarg.h>   // variable argument handling
#include <time.h>     // timing functions
```

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

## Core Functions Reference

### Menu Creation & Management
1. **`MENU create_menu()`**  
   Creates and returns a new menu object.

2. **`void enable_menu(MENU menu)`**  
   Activates and displays the menu.

3. **`void clear_menu(MENU menu)`**  
   **FIXED:** Now properly frees all resources (menu struct, items, and buffers)

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
   **FIXED:** Now handles memory reallocation correctly after removal

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
    
15. **`void set_menu_settings(MENU menu, MENU_SETTINGS settings)`** 
    Applies custom settings to a specific menu.  
    *Example:*
    ```c
    MENU_SETTINGS custom = create_new_settings();
    custom.mouse_enabled = 1;
    custom.header_enabled = 0;
    custom.footer_enabled = 0;
    set_menu_settings(my_menu, custom);
    ```

15. **`void set_default_menu_settings(MENU_SETTINGS settings)`**  
    Sets default settings for new menus.

### Color Management
16. **`MENU_COLOR create_color_object()`**  
    Creates a new color object with default colors.

17. **`void set_color_object(MENU menu, MENU_COLOR color_object)`**  
    Applies color scheme to a specific menu.

18. **`void set_default_color_object(MENU_COLOR color_object)`**  
    Sets default color scheme for new menus.

### RGB Color Functions
19. **`MENU_RGB_COLOR rgb(short r, short g, short b)`**  
    Creates an RGB color object with specified components (0-255).

20. **`void new_rgb_color(int text_color, MENU_RGB_COLOR color, char output[MAX_RGB_LEN])`**  
    Generates ANSI escape sequence for a single RGB color.  
    - `text_color`: 1 for foreground, 0 for background  
    - `color`: RGB color to convert  
    - `output`: Buffer to store the sequence (MAX_RGB_LEN = 45)

21. **`void new_full_rgb_color(MENU_RGB_COLOR fg, MENU_RGB_COLOR bg, char output[MAX_RGB_LEN])`**  
    Generates ANSI sequence for both foreground and background RGB colors.  
    - `fg`: Foreground RGB color  
    - `bg`: Background RGB color  
    - `output`: Buffer to store the sequence (MAX_RGB_LEN = 45)

## Color Customization

### RGB Color Functions
Create custom colors using RGB values (0-255):

```c
// create RGB colors
MENU_RGB_COLOR custom_fg = rgb(255, 200, 0);  // gold text
MENU_RGB_COLOR custom_bg = rgb(30, 30, 100);  // dark blue background

// generate sequences
char full_seq[MAX_RGB_LEN];

// create combined sequence
new_full_rgb_color(custom_fg, custom_bg, full_seq);

// use it after via strcpy(dest, src)
// ...
```

### Predefined Color Macros
## Example with Colors and Custom Settings

```c
#include "menu.h"

typedef struct
{
    int id;
    const char* name;
} MenuData;

void file_callback(MENU menu, void* data)
{
    MenuData* file_data = (MenuData*)data;
    printf("Selected file option: %s (ID: %d)\n", 
           file_data->name, file_data->id);
    getchar();
}

int main()
{
    // create custom settings
    MENU_SETTINGS custom_settings = create_new_settings();
    custom_settings.mouse_enabled = 1;
    set_default_menu_settings(custom_settings);

    // create custom colors
    MENU_COLOR custom_colors = create_color_object();
    
    // use new_full_rgb_color for custom RGB
    new_full_rgb_color(rgb(45, 230, 0), rgb(143, 32, 255), custom_colors.footerColor);
    
    set_default_color_object(custom_colors);

    MENU main_menu = create_menu();
    
    // add menu items
    MenuData open_data = {1, "Open File"};
    add_option(main_menu, create_menu_item("Open", file_callback, &open_data));
    
    enable_menu(main_menu);
    return 0;
}
```

## Building

Compile with your project:
```bash
gcc <your_app.c> menu.c -o your_app
```

## Key Updates

### v0.9.3 BETA
- Implemented RGB color support for the menu
- Fixed memory management bugs
- Addressed window resizing edge cases
- Optimized mouse event processing

## Limitations
- Windows-only implementation
- Requires console supporting ANSI escape codes (Windows 10+)
- Console resize handling has minimum size requirements
- Limited to vertical menus

*Planned for future versions: Horizontal menus, improved resize handling*

## Important Notes

1. **Memory Management**  
   Always use `clear_menu()` to destroy menus - direct `free()` calls will cause memory leaks

2. **Item Removal**  
   When removing items with `clear_option()`, the selected index is automatically adjusted to prevent invalid positions

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

> **Critical Restriction**: Never create a persistent pointer to a `MENU_ITEM` created via `create_menu_item()`. These objects are managed internally by the library and direct access may cause memory corruption.
