# Windows Console Menu Library

This lightweight library provides a simple, easy-to-use menu system for Windows console applications. It features light customizable menu, keyboard navigation, and a clean abstraction layer.

## Version
```0.8.7 BETA```

## Features

- **Windows-only implementation** (uses Windows API)
- Customizable headers and footers
- Colorful menu options with highlighting
- Keyboard navigation (arrow keys + Enter)
- Double buffering for smooth rendering
- Callback functions for menu selections
- Dynamically changes the size and location of the menu in the center of the console
- **Memory-safe menu item creation** with `create_menu_item()`

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

void menu_callback_example(void* unused)
{
    printf("Option selected!\n");
}

int main()
{
    MENU main_menu = create_menu();
    
    // create menu items using create_menu_item
    add_option(main_menu, create_menu_item("Option 1", menu_callback_example));
    add_option(main_menu, create_menu_item("Option 2", menu_callback_example));
    
    change_header(main_menu, "MAIN MENU");
    change_footer(main_menu, "Navigate with arrow keys");
    
    enable_menu(main_menu);
}
```

### Core Functions

1. **`MENU create_menu()`**  
   Creates and returns a new menu object.

2. **`MENU_ITEM create_menu_item(const char* text, void (*callback)(void*))`**  
   Creates a menu item with text and callback function.  
   *This is the recommended way to create menu items*.

3. **`int add_option(MENU your_menu, menu_item* your_item)`**  
   Adds an option to your menu.  
   - `menu_item` struct:
     ```c
     typedef struct
     {
         char* text;                   // display text
         void (*callback)(void* menu); // function to call when selected
     } menu_item;
     // MENU_ITEM is the pointer version of menu_item, menu_item shouldn't be used.
     ```

4. **`void change_header(MENU your_menu, const char* text)`**  
   Sets the menu header text.

5. **`void change_footer(MENU your_menu, const char* text)`**  
   Sets the menu footer text.

6. **`void enable_menu(MENU your_menu)`**  
   Activates and displays the menu.

7. **`void change_menu_policy(MENU menu, int header_policy, int footer_policy)`**  
   Controls header/footer visibility (1 = show, 0 = hide).

8. **`void clear_menu(MENU menu)`**  
   Destroys a menu and frees its resources.

9. **`void clear_menus()`**
   Destroys all the menus that you had in the program, and then shuts down the program.

### Creating Menu Items

Use `create_menu_item()` for memory-safe menu item creation:
```c
MENU_ITEM item = create_menu_item("Click Me", my_callback);

// add to menu:
add_option(my_menu, item);
```

### Advanced Features

- **Color Customization** (defined in menu.h):
  ```c
  #define HEADER     "\033[44m\033[37m" // dark blue
  #define SUBHEADER  "\033[46m\033[30m" // light blue
  #define HIGHLIGHT  "\033[47m\033[30m" // inverted (white)
  #define SUCCESS    "\033[42m\033[37m" // green
  #define ERROR_COLOR "\033[1;31m"      // red
  #define RESET      "\033[0m" 
  ```

- **Menu Policies**:
  ```c
  // keep header and hide footer:
  change_menu_policy(my_menu, 1, 0);
  ```

- **Callback Functions**:
  ```c
  void custom_callback(void* menu)
  {
      MENU m = (MENU)menu;
      printf("Selected from menu with %d options\n", get_menu_options_amount(m));
      // access menu context if needed
  }
  ```

## Building

Compile with your project if using static lib (example using GCC):
```bash
gcc <your_app.c> menu.c -o your_app
```

## Example Program

```c
#include <stdio.h>
#include "menu.h"

void file_action(void* unused)
{
    printf("\nFile action selected!\n");
    system("pause");
}

void edit_action(void* unused)
{
    printf("\nEdit action selected!\n");
    system("pause");
}

void exit_action(void* m)
{
    MENU menu = (MENU)m;
    printf("\nExiting...\n");
    clear_menus_and_exit();
}

int main()
{
    MENU main_menu = create_menu();
    
    // create menu items with create_menu_item
    add_option(main_menu, create_menu_item("Open File", file_action));
    add_option(main_menu, create_menu_item("Save File", file_action));
    add_option(main_menu, create_menu_item("Edit Content", edit_action));
    add_option(main_menu, create_menu_item("Exit Program", exit_action));
    
    change_header(main_menu, "TEXT EDITOR V2.4");
    change_footer(main_menu, "Use ↑↓ to navigate, Enter to select, Esc to exit");
    
    enable_menu(main_menu);
    
    return 0;
}
```

## Graphical example
<img width="3600" height="3399" alt="how menu works" src="https://github.com/user-attachments/assets/11fe3e66-bf14-4ddf-b7c7-992e154b651b" />

## Key Updates

- Version 0.8.6 BETA -> 0.8.7 BETA
- **`create_menu_item()` is now the recommended way** to create menu items (be careful of memory leaks if you don't use this)
- All callbacks should have a `void*` parameter (can be NULL if unused)
- Memory management is handled automatically for menu items
- Provides better type safety and error prevention

## Limitations

- Windows-only implementation
- Console resize handling has minimum size requirements
- Limited to vertical menus
- No mouse support (will be added in future)
- Some window resizing visual bugs

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

> **Note**: Always use `create_menu_item()` for creating menu items to ensure proper memory management and compatibility.
