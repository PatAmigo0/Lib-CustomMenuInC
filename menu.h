#pragma once

#ifndef _MENU_H_
#define _MENU_H_

/* ============== INCLUDES ============== */
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

/* end */

/* ============== CONSTANTS & MACROS ============== */
// Color Definitions
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

// reset all styles
#define RESET_ALL_STYLES          "\x1b[0m"

// screen managment (vt)
#define RESET_MOUSE_POSITION "\x1b[H"
#define CLEAR_SCREEN "\x1b[2J"
#define CLEAR_SCROLL_BUFFER "\x1b[3J"

// values
#define MAX_RGB_LEN 45
#define DEFAULT_MOUSE_SETTING 0
#define DEFAULT_HEADER_SETTING 1
#define DEFAULT_FOOTER_SETTING 1
#define DEFAULT_WIDTH_SETTING 1
#define DEFAULT_LEGACY_SETTING 0

/* ============== LEGACY COLORS ============== */

// standard colors (dark)
#define BLACK 0
#define BLUE FOREGROUND_BLUE
#define GREEN FOREGROUND_GREEN
#define CYAN FOREGROUND_GREEN | FOREGROUND_BLUE
#define RED FOREGROUND_RED
#define MAGENTA FOREGROUND_RED | FOREGROUND_BLUE
#define YELLOW FOREGROUND_RED | FOREGROUND_GREEN
#define WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE

// bright colors (add intensity)
#define BRIGHT_BLUE FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define BRIGHT_GREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define BRIGHT_CYAN FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define BRIGHT_RED FOREGROUND_RED | FOREGROUND_INTENSITY
#define BRIGHT_MAGENTA FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define BRIGHT_YELLOW FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define BRIGHT_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY

// standard background colors
#define BG_BLACK 0
#define BG_BLUE BACKGROUND_BLUE
#define BG_GREEN BACKGROUND_GREEN
#define BG_CYAN BACKGROUND_GREEN | BACKGROUND_BLUE
#define BG_RED BACKGROUND_RED
#define BG_MAGENTA BACKGROUND_RED | BACKGROUND_BLUE
#define BG_YELLOW BACKGROUND_RED | BACKGROUND_GREEN
#define BG_WHITE BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE

// bright background colors
#define BG_BRIGHT_BLUE BACKGROUND_BLUE | BACKGROUND_INTENSITY
#define BG_BRIGHT_GREEN BACKGROUND_GREEN | BACKGROUND_INTENSITY
#define BG_BRIGHT_CYAN BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY
#define BG_BRIGHT_RED BACKGROUND_RED | BACKGROUND_INTENSITY
#define BG_BRIGHT_MAGENTA BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY
#define BG_BRIGHT_YELLOW BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY
#define BG_BRIGHT_WHITE BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY

// some useful combinations
#define ERROR_COLOR FOREGROUND_RED | FOREGROUND_INTENSITY
#define WARNING_COLOR FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define SUCCESS_COLOR FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define INFO_COLOR FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define HIGHLIGHT_COLOR FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_BLUE

// menu-related colors
#define MENU_HEADER_COLOR FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_WHITE
#define MENU_SELECTED_COLOR FOREGROUND_WHITE | FOREGROUND_INTENSITY | BACKGROUND_BLUE
#define MENU_NORMAL_COLOR FOREGROUND_WHITE
#define MENU_FOOTER_COLOR FOREGROUND_GREEN | FOREGROUND_INTENSITY

/* ============== TYPE DEFINITIONS ============== */
// main menu type
struct __menu;

// string macro
typedef char* RGB_COLOR_SEQ;

// coord struct
typedef struct
{
    float X;
    float Y;
} MENU_COORD;

// menu item struct
typedef struct __menu_item
{
    COORD boundaries;
    int text_len;
    const char* text;
    void (*callback)(struct __menu*, void*); //
    void* data_chunk;
} *MENU_ITEM;

// color settings
typedef struct __menu_color_object_property
{
    char __rgb_seq[MAX_RGB_LEN];
} COLOR_OBJECT_PROPERTY;

typedef struct __menu_color_object
{
    COLOR_OBJECT_PROPERTY headerColor;
    COLOR_OBJECT_PROPERTY footerColor;
    COLOR_OBJECT_PROPERTY optionColor;
} MENU_COLOR;

typedef struct __legacy_menu_color_object
{
    WORD headerColor;
    WORD footerColor;
    WORD optionColor;
} LEGACY_MENU_COLOR;

// menu settings
typedef struct __menu_settings
{
    int mouse_enabled;
    int header_enabled;
    int footer_enabled;
    int double_width_enabled;
    int force_legacy_mode;
    MENU_COORD menu_center;
    int __garbage_collector;
} MENU_SETTINGS;

// menu render
typedef struct __menu_render_unit
{
    const char* text;
    DWORD unit_type; // for the future (like heade = 0x1, footer and etc)
    void* extra_data;
} MENU_RENDER_UNIT;

typedef struct __menu_render_unit_types
{
    DWORD header;
    DWORD footer;
    DWORD selectable;
} MENU_RENDER_UNIT_TYPES;

typedef MENU_RENDER_UNIT* PMENU_RENDER_UNIT;

// RGB color
typedef struct
{
    short r, g, b;
} MENU_RGB_COLOR;

// main menu struct
typedef struct __menu
{
    // meta
    unsigned long long __ID;
    WORD count;
    MENU_ITEM* options;
    int active_buffer;

    // boolean
    int running;
    int need_redraw;

    // handles
    HANDLE hBuffer[2];
    short int selected_index;

    // render
    COORD menu_size;
    COORD current_size;
    COORD halt_size;

    const char* footer;
    const char* header;

    // objects
    MENU_SETTINGS menu_settings;
    MENU_COLOR color_object;
    LEGACY_MENU_COLOR legacy_color_object;
} *MENU;

// callback func
typedef void* dpointer;
typedef void (*__menu_callback)(MENU, dpointer);

/* ============== FUNCTION DECLARATIONS ============== */

/* ----- Core Functions ----- */
MENU create_menu();
MENU_ITEM create_menu_item(const char* text, __menu_callback callback, void* callback_data);
void enable_menu(MENU used_menu);
void clear_menu(MENU menu_to_clear);
void clear_menus();
void clear_menus_and_exit();

/* ----- Configuration Functions ----- */
void set_menu_settings(MENU menu, MENU_SETTINGS new_settings);
void set_color_object(MENU menu, MENU_COLOR color_object);
void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy);
void toggle_mouse(MENU menu_to_change);
void change_header(MENU used_menu, const char* text);
void change_footer(MENU used_menu, const char* text);

/* ----- Item Management ----- */
void add_option(MENU used_menu, const MENU_ITEM item);
void clear_option(MENU used_menu, MENU_ITEM option_to_clear);

/* ----- Color Functions ----- */
MENU_RGB_COLOR mrgb(short r, short g, short b);
COLOR_OBJECT_PROPERTY new_rgb_color(int text_color, MENU_RGB_COLOR color);
COLOR_OBJECT_PROPERTY new_full_rgb_color(MENU_RGB_COLOR fg, MENU_RGB_COLOR bg);

/* ----- Settings Management ----- */
MENU_SETTINGS create_new_settings();
MENU_COLOR create_color_object();
LEGACY_MENU_COLOR create_legacy_color_object();
void set_default_menu_settings(MENU_SETTINGS new_settings);
void set_default_color_object(MENU_COLOR color_object);
void set_default_legacy_color_object(LEGACY_MENU_COLOR color_object);

/* ----- Utility Functions ----- */
double tick();

// Global struct that defines types for render units
static MENU_RENDER_UNIT_TYPES mrut;

#endif