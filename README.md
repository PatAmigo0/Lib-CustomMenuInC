# Windows Console Menu Library

This lightweight library provides a simple, easy-to-use menu system for Windows console applications. It features light customizable menu, keyboard navigation, and a clean abstraction layer.

## Version
```0.9.0 BETA```

## Features

- **Windows-only implementation** (uses Windows API)
- Customizable headers and footers
- Colorful menu options with highlighting
- Keyboard navigation (arrow keys + Enter)
- Double buffering for smooth rendering
- Callback functions for menu selections
- Dynamically changes the size and location of the menu in the center of the console
- Mouse tracking
- Clear abstraction layer
- Safe mmemory managment
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

void menu_callback_example(MENU menu, dpointer data_block)
{
    printf("Option selected!\n");
}

int main()
{
    MENU main_menu = create_menu();
    
    // create menu items using create_menu_item
    add_option(main_menu, create_menu_item("Option 1", menu_callback_example, NULL));
    add_option(main_menu, create_menu_item("Option 2", menu_callback_example, NULL));
    
    change_header(main_menu, "MAIN MENU");
    change_footer(main_menu, "Navigate with arrow keys");
    
    enable_menu(main_menu);
}
```

### Core Functions

1. **`MENU create_menu()`**  
   Creates and returns a new menu object.

2. **`MENU_ITEM create_menu_item(const char* restrict text, void (*callback)(MENU, dpointer), dpointer data_to_send)`**  
   Creates a menu item with text and callback function.  
   *This is the recommended way to create menu items*.

3. **`int add_option(MENU your_menu, menu_item* restrict your_item)`**  
   Adds an option to your menu.  

4. **`void change_header(MENU your_menu, const char* restrict text)`**  
   Sets the menu header text.

5. **`void change_footer(MENU your_menu, const char* restrict text)`**  
   Sets the menu footer text.

6. **`void enable_menu(MENU your_menu)`**  
   Activates and displays the menu.

7. **`void change_menu_policy(MENU menu, int header_policy, int footer_policy)`**  
   Controls header/footer visibility (1 = show, 0 = hide).

8. **`void toggle_mouse(MENU menu)`**
   Toggles mouse input.

9. **`void clear_menu(MENU menu)`**  
   Destroys a menu and frees its resources.

10. **`void clear_menus()`**
   Destroys all the menus that you had in the program, and then shuts down the program.

### Creating Menu Items

Use `create_menu_item()` for memory-safe menu item creation:
```c
MENU_ITEM item = create_menu_item("Click Me", my_callback, NULL);

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
  void custom_callback(MENU menu)
  {
      printf("Selected from menu with %d options\n", get_menu_options_amount(menu));
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

void file_action(MENU unused)
{
    printf("\nFile action selected!\n");
    system("pause");
}

void edit_action(MENU unused)
{
    printf("\nEdit action selected!\n");
    system("pause");
}

void exit_action(MENU menu)
{
    printf("\nExiting...\n");
    clear_menu(menu);
}

int main()
{
    MENU main_menu = create_menu();
    toggle_mouse(main_menu);

    // create menu items with create_menu_item
    add_option(main_menu, create_menu_item("Open File", file_action, NULL));
    add_option(main_menu, create_menu_item("Save File", file_action, NULL));
    add_option(main_menu, create_menu_item("Edit Content", edit_action, NULL));
    add_option(main_menu, create_menu_item("Exit Program", exit_action, NULL));
    
    change_header(main_menu, "TEXT EDITOR V2.4");
    change_footer(main_menu, "Use ↑↓ to navigate, Enter to select, Esc to exit");
    
    enable_menu(main_menu);
    
    return 0;
}
```

## Graphical example
<img width="3600" height="3399" alt="how menu works" src="https://github.com/user-attachments/assets/11fe3e66-bf14-4ddf-b7c7-992e154b651b" />

## Key Updates

- Version 0.8.8 -> 0.9.0 BETA
- Mouse support
- You can pass any value to your callback via void pointer (dpointer) to the data_block
- More efficent memory management
- Provides better type safety and error prevention

## Limitations

- Windows-only implementation
- Console resize handling has minimum size requirements
- Limited to vertical menus
- Some window resizing visual bugs

  `All this is going to be added in the future.`

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

> **Note**: Never make a pointer to the MENU_ITEM which was created via ```create_menu_item()``` (RESTRICTED PARAM).
