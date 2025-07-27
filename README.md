
# Windows Console Menu Library

This lightweight library provides a simple, easy-to-use menu system for Windows console applications. It features customizable menus, keyboard and mouse navigation, and a clean abstraction layer.

## Version
```0.9.1```

## Features

- **Windows-only implementation** (uses Windows API)
- Customizable headers and footers
- Colorful menu options with highlighting
- Keyboard navigation (arrow keys + Enter)
- Mouse support (toggle on/off)
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

## Usage

### Basic Setup

```c
#include "menu.h"

void menu_callback(MENU menu, dpointer data)
{
    char* message = (char*)data;
    printf("%s\n", message);
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
14. **`void set_new_default_settings(MENU_SETTINGS settings)`**  
    Sets default settings for new menus.

### Information Getters
15. **`int get_menu_options_amount(MENU menu)`**  
    Returns number of options in menu.

16. **`int is_menu_running(MENU menu)`**  
    Returns 1 if menu is active, 0 otherwise.

17. **`int get_menu_selected_index(MENU menu)`**  
    Returns currently selected index.

18. **`const char* get_menu_header(MENU menu)`**  
    Returns current header text.

19. **`const char* get_menu_footer(MENU menu)`**  
    Returns current footer text.

20. **`int get_menu_header_policy(MENU menu)`**  
    Returns header visibility status.

21. **`int get_menu_footer_policy(MENU menu)`**  
    Returns footer visibility status.

22. **`int get_menu_width_policy(MENU menu)`**  
    Returns width policy status.

## Advanced Example with dpointer

```c
#include <stdio.h>
#include "menu.h"

// define custom data structure
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
}

void edit_callback(MENU menu, dpointer data)
{
    int* value = (int*)data;
    printf("Edit action called with value: %d\n", *value);
}

void exit_callback(MENU menu, dpointer data)
{
    printf("Exiting program...\n");
    clear_menus_and_exit();
}

int main()
{
    // create data for callbacks
    MenuData open_data = {1, "Open File"};
    MenuData save_data = {2, "Save File"};
    int edit_value = 42;
    
    MENU main_menu = create_menu();
    toggle_mouse(main_menu);

    // create menu items with different data types
    add_option(main_menu, create_menu_item("Open", file_callback, &open_data));
    add_option(main_menu, create_menu_item("Save", file_callback, &save_data));
    add_option(main_menu, create_menu_item("Edit", edit_callback, &edit_value));
    add_option(main_menu, create_menu_item("Exit", exit_callback, NULL));
    
    change_header(main_menu, "ADVANCED EXAMPLE");
    change_footer(main_menu, "Pass different data types to callbacks");
    
    // hide footer
    change_menu_policy(main_menu, 1, 0);
    
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

- Version 0.9.0 BETA → 0.9.1
- Less recources consume
- Program works 20x faster and smoother
- A lot of bugs were fixed

## Limitations

- Windows-only implementation
- Console resize handling has minimum size requirements
- Limited to vertical menus
- Some window resizing visual bugs

*Planned for future versions: Horizontal menus, improved resize handling*

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

> **Important**: Never create a pointer to a `MENU_ITEM` created via `create_menu_item()` (RESTRICTED PARAM).
