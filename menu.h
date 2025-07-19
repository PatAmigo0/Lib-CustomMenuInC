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

struct __menu_item;

typedef struct __menu_item* MENU_ITEM; 

typedef struct __menu 
{
	int __ID;
    int count;
    MENU_ITEM* options;
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
MENU create_menu();
MENU_ITEM create_menu_item(const char* restrict text, menu_callback callback);
int add_option(MENU restrict used_menu, const MENU_ITEM restrict item);
void change_header(MENU restrict used_menu, const char* restrict text);
void change_footer(MENU restrict used_menu, const char* restrict text);
void enable_menu(MENU restrict used_menu);
void clear_option(MENU restrict used_menu, MENU_ITEM restrict option_to_clear);
void change_menu_policy(MENU restrict menu_to_change, int new_header_policy, int new_footer_policy);
void clear_menu(MENU restrict menu_to_clear);
void clear_menus();

#endif