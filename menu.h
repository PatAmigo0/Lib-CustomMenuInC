#ifndef _MENU_H_
#define _MENU_H_

/* includes */
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

#ifndef _PSAPI_H_
#include <psapi.h>
#endif
/* end */

/* COLOR DEFINITIONS */
// text Colors (Foreground)
#define BLACK_TEXT          "\033[30m"
#define RED_TEXT            "\033[31m"
#define GREEN_TEXT          "\033[32m"
#define YELLOW_TEXT         "\033[33m"
#define BLUE_TEXT           "\033[34m"
#define MAGENTA_TEXT        "\033[35m"
#define CYAN_TEXT           "\033[36m"
#define WHITE_TEXT          "\033[37m"

#define BRIGHT_BLACK_TEXT   "\033[90m"
#define BRIGHT_RED_TEXT     "\033[91m"
#define BRIGHT_GREEN_TEXT   "\033[92m"
#define BRIGHT_YELLOW_TEXT  "\033[93m"
#define BRIGHT_BLUE_TEXT    "\033[94m"
#define BRIGHT_MAGENTA_TEXT "\033[95m"
#define BRIGHT_CYAN_TEXT    "\033[96m"
#define BRIGHT_WHITE_TEXT   "\033[97m"

// background Colors
#define BLACK_BG           "\033[40m"
#define RED_BG             "\033[41m"
#define GREEN_BG           "\033[42m"
#define YELLOW_BG          "\033[43m"
#define BLUE_BG            "\033[44m"
#define MAGENTA_BG         "\033[45m"
#define CYAN_BG            "\033[46m"
#define WHITE_BG           "\033[47m"

#define BRIGHT_BLACK_BG    "\033[100m"
#define BRIGHT_RED_BG      "\033[101m"
#define BRIGHT_GREEN_BG    "\033[102m"
#define BRIGHT_YELLOW_BG   "\033[103m"
#define BRIGHT_BLUE_BG     "\033[104m"
#define BRIGHT_MAGENTA_BG  "\033[105m"
#define BRIGHT_CYAN_BG     "\033[106m"
#define BRIGHT_WHITE_BG    "\033[107m"

// text Styles
#define BOLD_TEXT          "\033[1m"
#define DIM_TEXT           "\033[2m"
#define ITALIC_TEXT        "\033[3m"
#define UNDERLINE_TEXT     "\033[4m"
#define BLINK_TEXT         "\033[5m"
#define REVERSE_TEXT       "\033[7m"  // swap foreground and background
#define HIDDEN_TEXT        "\033[8m"
#define STRIKETHROUGH_TEXT "\033[9m"

// reset all styles
#define RESET_ALL_STYLES          "\033[0m"

// combinations
#define DARK_BLUE_BG_WHITE_TEXT     "\033[44;37m"
#define CYAN_BG_BLACK_TEXT          "\033[46;30m"
#define WHITE_BG_BLACK_TEXT         "\033[47;30m"
#define GREEN_BG_WHITE_TEXT         "\033[42;37m"
#define BRIGHT_RED_TEXT_ONLY        "\033[1;31m"
#define BRIGHT_GREEN_TEXT_ONLY      "\033[1;32m"
#define RED_BG_WHITE_TEXT           "\033[41;37m"
#define YELLOW_BG_BLACK_TEXT        "\033[43;30m"
#define MAGENTA_BG_WHITE_TEXT       "\033[45;37m"
#define BRIGHT_RED_BG_WHITE_TEXT    "\033[101;97m"
#define BRIGHT_YELLOW_BG_BLACK_TEXT "\033[103;30m"

// screen managment (vt)
#define RESET_MOUSE_POSITION "\033[H"
#define CLEAR_SCREEN "\x1b[2J"
#define CLEAR_SCROLL_BUFFER "\x1b[3J"

// values
#define MAX_RGB_LEN 45
#define DEFAULT_MOUSE_SETTING 0
#define DEFAULT_HEADER_SETTING 1
#define DEFAULT_FOOTER_SETTING 1
#define DEFAULT_WIDTH_SETTING 1

// types
typedef char* RGB_COLOR_SEQ;

struct __menu_item;
struct __menu;
struct __menu_settings
{
    BYTE mouse_enabled;
    BYTE header_enabled;
    BYTE footer_enabled;
    BYTE double_width_enabled;
    BYTE __garbage_collector;
};

typedef struct __menu_color_object
{
    char headerColor[MAX_RGB_LEN];
    char footerColor[MAX_RGB_LEN];
} MENU_COLOR;

typedef struct __menu_item* MENU_ITEM;
typedef struct __menu* MENU;

typedef void (*__menu_callback)(MENU, void*);
typedef void* dpointer;
typedef struct __menu_settings MENU_SETTINGS;
typedef struct __menu_color_object MENU_COLOR;

typedef struct
{
    short r, g, b;
} MENU_RGB_COLOR;

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
MENU_ITEM create_menu_item(const char* restrict text, __menu_callback callback, void* callback_data);
MENU_SETTINGS create_new_settings();
MENU_COLOR create_color_object();
MENU_RGB_COLOR rgb(short r, short g, short b);
void new_rgb_color(int text_color, MENU_RGB_COLOR color, char output[MAX_RGB_LEN]);
void new_full_rgb_color(MENU_RGB_COLOR _color_foreground, MENU_RGB_COLOR _color_background, char output[MAX_RGB_LEN]);
int add_option(MENU used_menu, const MENU_ITEM item);
void change_header(MENU used_menu, const char* restrict text);
void change_footer(MENU used_menu, const char* restrict text);
void set_color_object(MENU menu, MENU_COLOR color_object);
void toggle_mouse(MENU restrict menu_to_change);
void set_default_menu_settings(MENU_SETTINGS new_settings);
void set_default_color_object(MENU_COLOR color_object);
void enable_menu(MENU used_menu);
void clear_option(MENU used_menu, MENU_ITEM option_to_clear);
void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy);
void clear_menu(MENU menu_to_clear);
void clear_menus();
void clear_menus_and_exit();

#endif