#ifndef _MENU_H_
#define _MENU_H_

/* defines */
#ifndef _STDIO_H_
#include <stdio.h>
#endif

#ifndef _STDLIB_H_
#include <stdlib.h>
#endif

#ifndef _WINDOWS_H_
#include <windows.h>
#endif

#ifndef _STRING_H_
#include <string.h>
#endif

#ifndef _STDARG_H_
#include <stdarg.h>
#endif

#ifndef _TIME_H_
#include <time.h>
#endif

//#ifndef _LIMITS_H_
//#include <limits.h>
//#endif

/* end */

#define RESET      "\033[0m"
#define HEADER     "\033[44m\033[37m" // dark blue
#define SUBHEADER  "\033[46m\033[30m" // light blue
#define HIGHLIGHT  "\033[47m\033[30m"  // inverted (white)
#define SUCCESS    "\033[42m\033[37m" // green
#define ERROR_COLOR    "\033[1;31m"  // red
#define GREEN_COLOR    "\033[1;32m"

typedef void (*menu_callback)(void*, void*);

struct __menu_item;
struct __menu;

typedef struct __menu_item* MENU_ITEM; 
typedef struct __menu* MENU;

// function prototypes
double tick();
int get_menu_options_amount(MENU restrict menu);
int is_menu_running(MENU menu);
int get_menu_selected_index(MENU menu);
const char* get_menu_header(MENU menu);
const char* get_menu_footer(MENU menu);
int get_menu_header_policy(MENU menu);
int get_menu_footer_policy(MENU menu);
int get_menu_width_policy(MENU menu);
MENU create_menu();
MENU_ITEM create_menu_item(const char* restrict text, menu_callback callback, void* callback_data);
int add_option(MENU used_menu, const MENU_ITEM item);
void change_header(MENU used_menu, const char* restrict text);
void change_footer(MENU used_menu, const char* restrict text);
void enable_menu(MENU used_menu);
void clear_option(MENU used_menu, MENU_ITEM option_to_clear);
void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy);
void change_width_policy(MENU menu_to_change, int new_width_policy);
void clear_menu(MENU menu_to_clear);
void clear_menus();
void clear_menus_and_exit();

#endif