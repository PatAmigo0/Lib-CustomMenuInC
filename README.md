# Windows Console Menu Library

This lightweight library provides a simple, easy-to-use menu system for Windows console applications. It features light customizable menu, keyboard navigation, and a clean abstraction layer.

## Features

- **Windows-only implementation** (uses Windows API)
- Customizable headers and footers
- Colorful menu options with highlighting

- Keyboard navigation (arrow keys + Enter)
- Double buffering for smooth rendering
- Callback functions for menu selections
- Dynamically changes the size and location of the menu in the center of the console

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

void menu_callback_example()
{
    printf("Option selected!\n");
}

int main()
{
    MENU main_menu = create_menu();
    
    menu_item option1 = {"Option 1", menu_callback_example};
    menu_item option2 = {"Option 2", menu_callback_example};
    
    add_option(main_menu, &option1);
    add_option(main_menu, &option2);
    
    change_header(main_menu, "MAIN MENU");
    change_footer(main_menu, "Navigate with arrow keys");
    
    enable_menu(main_menu);

    clear_menus();
}
```

### Core Functions

1. **`MENU create_menu()`**  
   Creates and returns a new menu object.

2. **`int add_option(MENU your_menu, menu_item* your_item)`**  
   Adds an option to your menu.  
   - `menu_item` struct:
     ```c
     typedef struct
     {
         char* text;                   // display text
         void (*callback)(void* menu); // function to call when selected
     } menu_item;
     ```

3. **`void change_header(MENU your_menu, const char* text)`**  
   Sets the menu header text.

4. **`void change_footer(MENU your_menu, const char* text)`**  
   Sets the menu footer text.

5. **`void enable_menu(MENU your_menu)`**  
   Activates and displays the menu.

6. **`void change_menu_policy(MENU menu, int header_policy, int footer_policy)`**  
   Controls header/footer visibility (1 = show, 0 = hide).

7. **`void clear_menu(MENU menu)`**  
   Destroys a menu and frees its resources.

8. **`void clear_menus()`**
   Destroys all the menus that you had in the program, and then shuts down the program.

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
  // hide header and footer:
  change_menu_policy(my_menu, 0, 0);
  ```

- **Callback Functions**:
  ```c
  void custom_callback(void* menu)
  {
      MENU m = (MENU)menu;
      printf("Selected from menu with %d options\n", m->count);
  }
  ```

## Building

Compile with your project if using static lib (example using GCC):
```bash
gcc your_app.c menu.c -o your_app
```

## Example Program

```c
#include "menu.h"

void file_action()
{
    printf("\nFile action selected!\n");
    system("pause");
}

void edit_action()
{
    printf("\nEdit action selected!\n");
    system("pause");
}

void exit_action(void* m)
{
    MENU menu = (MENU)m;
    printf("\nExiting...\n");
    clear_menus();
    exit(0);
}

int main()
{
    init_menu_system();
    
    MENU main_menu = create_menu();
    
    menu_item options[] =
    {
        {"Open File", file_action},
        {"Save File", file_action},
        {"Edit Content", edit_action},
        {"Exit Program", exit_action}
    };
    
    for(int i = 0; i < 4; i++)
    {
        add_option(main_menu, &options[i]);
    }
    
    change_header(main_menu, "TEXT EDITOR V2.4");
    change_footer(main_menu, "Use ↑↓ to navigate, Enter to select, Esc to exit");
    
    enable_menu(main_menu);
    
    return 0;
}
```

## Graphical example
<img width="3600" height="3399" alt="how menu works" src="https://github.com/user-attachments/assets/11fe3e66-bf14-4ddf-b7c7-992e154b651b" />

## Limitations

- Windows-only implementation
- Console resize handling has minimum size requirements
- Limited to vertical menus
- No mouse support (will be added in future)
- Some window resizing visual bugs

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

> **Note**: This library is designed specifically for Windows console applications and uses Windows API functions extensively.
