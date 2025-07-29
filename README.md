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
- Safe memory management
- Easy to use!

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
   Destroys a menu and frees its resources.

4. **`void clear_menus()`**  
   Destroys all created menus.

5. **`void clear_menus_and_exit()`**  
   Destroys all menus and exits the program.

### Menu Item Management
6. **`MENU_ITEM create_menu_item(const char* text, menu_callback callback, dpointer data)`**  
   Creates a menu item with text, callback, and associated data.

7. **`int add_option(MENU menu, MENU_ITEM item)`**  
   Adds a menu item to a menu.

8. **`void clear_option(MENU menu, MENU_ITEM option)`**  
   Removes a specific option from a menu.

### Appearance Customization
9. **`void change_header(MENU menu, const char* text)`**  
   Sets the menu header text.

10. **`void change_footer(MENU menu, const char* text)`**  
    Sets the menu footer text.

11. **`void change_menu_policy(MENU menu, int header_policy, int footer_policy)`**  
    Controls header/footer visibility (1 = show, 0 = hide).

12. **`void change_width_policy(MENU menu, int width_policy)`**  
    Toggles between normal and double-width menu (1 = double width).

### Input Settings
13. **`void toggle_mouse(MENU menu)`**  
    Toggles mouse input support.

### Configuration
14. **`MENU_SETTINGS create_new_settings()`**  
    Creates a new settings object with default values.

15. **`void set_default_menu_settings(MENU_SETTINGS settings)`**  
    Sets default settings for new menus.

### Color Management
16. **`MENU_COLOR create_color_object()`**  
    Creates a new color object with default colors.

17. **`void set_color_object(MENU menu, MENU_COLOR color_object)`**  
    Applies color scheme to a specific menu.

18. **`void set_default_color_object(MENU_COLOR color_object)`**  
    Sets default color scheme for new menus.

### RGB Color Functions (New in v0.9.3 BETA)
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

### Information Getters
22. **`int get_menu_options_amount(MENU menu)`**  
    Returns number of options in menu.

23. **`int is_menu_running(MENU menu)`**  
    Returns 1 if menu is active, 0 otherwise.

24. **`int get_menu_selected_index(MENU menu)`**  
    Returns currently selected index.

25. **`const char* get_menu_header(MENU menu)`**  
    Returns current header text.

26. **`const char* get_menu_footer(MENU menu)`**  
    Returns current footer text.

27. **`int get_menu_header_policy(MENU menu)`**  
    Returns header visibility status.

28. **`int get_menu_footer_policy(MENU menu)`**  
    Returns footer visibility status.

29. **`int get_menu_width_policy(MENU menu)`**  
    Returns width policy status.

## Color Customization

The library supports extensive color customization through ANSI escape codes, including both predefined macros and RGB color creation.

### RGB Color Functions (New in v0.9.3 BETA)
Create custom colors using RGB values (0-255):

```c
// create RGB colors
MENU_RGB_COLOR custom_fg = rgb(255, 200, 0);  // gold text
MENU_RGB_COLOR custom_bg = rgb(30, 30, 100);  // dark blue background

// generate sequences
char fg_seq[MAX_RGB_SEQ_LEN];
char bg_seq[MAX_RGB_SEQ_LEN];
char full_seq[MAX_RGB_DOUBLE_SEQ_LEN];

// create individual sequences
new_rgb_color(1, custom_fg, fg_seq);   // text color
new_rgb_color(0, custom_bg, bg_seq);   // bg color

// create combined sequence
new_full_rgb_color(custom_fg, custom_bg, full_seq);
```

### Basic Color Macros
The library provides predefined color macros:

#### Text Colors (Foreground)
- `BLACK_TEXT`
- `RED_TEXT`
- `GREEN_TEXT`
- `YELLOW_TEXT`
- `BLUE_TEXT`
- `MAGENTA_TEXT`
- `CYAN_TEXT`
- `WHITE_TEXT`
- `BRIGHT_BLACK_TEXT`
- `BRIGHT_RED_TEXT`
- `BRIGHT_GREEN_TEXT`
- `BRIGHT_YELLOW_TEXT`
- `BRIGHT_BLUE_TEXT`
- `BRIGHT_MAGENTA_TEXT`
- `BRIGHT_CYAN_TEXT`
- `BRIGHT_WHITE_TEXT`

#### Background Colors
- `BLACK_BG`
- `RED_BG`
- `GREEN_BG`
- `YELLOW_BG`
- `BLUE_BG`
- `MAGENTA_BG`
- `CYAN_BG`
- `WHITE_BG`
- `BRIGHT_BLACK_BG`
- `BRIGHT_RED_BG`
- `BRIGHT_GREEN_BG`
- `BRIGHT_YELLOW_BG`
- `BRIGHT_BLUE_BG`
- `BRIGHT_MAGENTA_BG`
- `BRIGHT_CYAN_BG`
- `BRIGHT_WHITE_BG`

### Combining Colors
You can combine foreground and background colors by concatenating the macros. The general format is:

```
strcpy(destination, BACKGROUND FOREGROUND);
```

Example:

```c
// underlined text on green background
strcpy(custom_colors.headerColor, GREEN_BG WHITE_TEXT UNDERLINE_TEXT);

// bold yellow text on blue background
strcpy(custom_colors.footerColor, BLUE_BG BOLD_TEXT YELLOW_TEXT);
```

Available text styles:
- `BOLD_TEXT`
- `DIM_TEXT`
- `ITALIC_TEXT`
- `UNDERLINE_TEXT`
- `BLINK_TEXT`
- `REVERSE_TEXT` (swap foreground and background)
- `HIDDEN_TEXT`
- `STRIKETHROUGH_TEXT`

### Predefined Combinations
The library includes some predefined color combinations:

- `DARK_BLUE_BG_WHITE_TEXT`  
  Dark Blue BG with White Text

- `CYAN_BG_BLACK_TEXT`  
  Cyan BG with Black Text

- `WHITE_BG_BLACK_TEXT`  
  White BG with Black Text

- `GREEN_BG_WHITE_TEXT`  
  Green BG with White Text

- `RED_BG_WHITE_TEXT`  
  Red BG with White Text

- `YELLOW_BG_BLACK_TEXT`  
  Yellow BG with Black Text

- `BRIGHT_RED_BG_WHITE_TEXT`  
  Bright Red BG with White Text

## Example with Colors and Custom Settings

```c
#include "menu.h"

typedef struct
{
    int id;
    const char* name;
} MenuData;

void file_callback(MENU menu, dpointer data)
{
    MenuData* file_data = (MenuData*)data;
    printf("Selected file option: %s (ID: %d)\n", 
           file_data->name, file_data->id);
    getchar();
}

// ... other callbacks ...

int main()
{
    // create custom settings
    MENU_SETTINGS custom_settings = create_new_settings();
    custom_settings.mouse_enabled = 1;
    set_default_menu_settings(custom_settings);

    // create custom colors
    MENU_COLOR custom_colors = create_color_object();
    strcpy(custom_colors.headerColor, MAGENTA_BG WHITE_TEXT); // for pre-defined styles use this
    new_full_rgb_color(rgb(45, 230, 0), rgb(143, 32, 255), custom_colors.footerColor); // for rgb use this
    set_default_color_object(custom_colors);

    MENU main_menu = create_menu();
    
    // add menu items
    MenuData open_data = {1, "Open File"};
    add_option(main_menu, create_menu_item("Open", file_callback, &open_data));
    
    // ... other options ...
    
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
- Fixed critical memory management bugs
- Most implementations have been changed to use stack memory instead of heap
- Improved error handling robustness
- Addressed window resizing edge cases
- Optimized mouse event processing
- Fixed callback execution context issues
- Overall optimizations

### v0.9.2 BETA
- Added advanced color customization system
- Implemented default settings configuration
- Improved buffer management
- Optimized error handling
- Fixed switching to the console 

### v0.9.1 BETA
- Reduced resource consumption by 5 times
- Improved performance (20x faster rendering)
- Fixed mouse input handling issues
- Overall buug fixes

## Limitations
- Windows-only implementation
- Requires console supporting ANSI escape codes (Windows 10+)
- Console resize handling has minimum size requirements
- Limited to vertical menus
- Some window resizing visual bugs

*Planned for future versions: Horizontal menus, improved resize handling*

## Important Notes

1. **Memory Management**  
   The MENU is a dynamically allocated memory. Always use `clear_menu()` to destroy menus - direct `free()` calls will cause memory leaks

2. **Thread Safety**  
   This library is not thread-safe. Call all menu functions from the main thread (subject to change in the future if there are any requests)

3. **Item Lifetime**  
   Never create pointers to `MENU_ITEM` objects (RESTRICTED PARAM)

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

> **Critical Restriction**: Never create a persistent pointer to a `MENU_ITEM` created via `create_menu_item()`. These objects are managed internally by the library and direct access may cause memory corruption.
