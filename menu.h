#ifndef MENU_H
#define MENU_H

#include <windows.h>
#define RESET      "\033[0m"
#define HEADER     "\033[44m\033[37m" // dark blue
#define SUBHEADER  "\033[46m\033[30m" // light blue
#define HIGHLIGHT  "\033[47m\033[30m"  // inverted (white)
#define SUCCESS    "\033[42m\033[37m" // green
#define ERROR_COLOR    "\033[1;31m"  // red
#define GREEN_COLOR    "\033[1;32m"

typedef void (*menu_callback)(void* m);

typedef struct menu_item 
{
    const char* text;
    menu_callback callback;
} menu_item;

typedef struct __menu 
{
	int __ID;
    int count;
    menu_item* options;
    int running;
    int need_redraw;
    int active_buffer;
    HANDLE hBuffer[2];
    int selected_index;
    
    int footer_policy;
    int header_policy;
    
    const char* footer;
    const char* header;
    
    COORD menu_size;
} __menu;

typedef __menu* MENU;

// function prototypes
double tick();
void init_menu_system();
MENU create_menu();
int add_option(MENU used_menu, menu_item* item);
void change_header(MENU used_menu, const char* text);
void change_footer(MENU used_menu, const char* text);
void enable_menu(MENU used_menu);
void clear_option(MENU used_menu, menu_item* option_to_clear);
void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy);
void clear_menu(MENU menu_to_clear);
void clear_menus();

#endif