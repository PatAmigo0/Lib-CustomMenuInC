#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "menu.h"

#define DEBUG 0

#define BUFFER_CAPACITY 128
#define UPDATE_FREQUENCE 20 // ms
#define EVENT_MAX_RECORDS 8

#define FIX_VALUE 2

#define DEFAULT_HEADER_TEXT "MENU"
#define DEFAULT_FOOTER_TEXT "Use arrows to navigate, Enter to select"
#define DEFAULT_MENU_TEXT "Unnamed Option"

#define LOG_FILE_NAME "menu_log.txt"

static COORD zero_point, saved_buffer_size, bufferSize;
static DWORD written = 0, saved_size;
static MENU* menus_array = NULL;
static int menus_amount = 0;

static HANDLE hConsole, hConsoleError, hCurrent, _hError;

static CONSOLE_SCREEN_BUFFER_INFO hBack_csbi;

const char* error_message =
    ERROR_COLOR
    "Error: Console window size is too small!\n"
    "Required size: %d x %d\n"
    "Current size: %d x %d\n"
    "Make window bigger."
    RESET;

struct __menu_item
{
    const char* text;
    menu_callback callback;
}; // protecting menu_item

struct __menu
{
    unsigned long long __ID;
    int count;
    MENU_ITEM* options;
    int running;
    int need_redraw;
    int active_buffer;
    int width_policy;
    HANDLE hBuffer[2];
    int selected_index;

    int footer_policy;
    int header_policy;

    const char* footer;
    const char* header;

    COORD menu_size;
}; // protecting menu

// restricted functions prototypes
void _init_menu_system();
void _init_hError();
HANDLE _createConsoleScreenBuffer(void);
void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text, ...);
void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text);
void _write_string(HANDLE hDestination, const char* restrict text, ...);
void _vwrite_string(HANDLE hDestination, const char* restrict text, va_list args);
void _lwrite_string(HANDLE hDestination, const char* restrict text);
void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate);
void _getMenuSize(MENU menu);
BYTE _size_check(MENU menu, BYTE show_error, int extra_value);
void _initWindow(SMALL_RECT* restrict window, COORD size);
void _clear_buffer(HANDLE hBuffer);
int _check_menu(unsigned long long saved_id);
HANDLE _block_input(DWORD* oldMode);
void _renderMenu(const MENU used_menu);
void _show_error_and_wait(const char* restrict message, ...);

double tick()
{
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    static LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
}

void change_menu_policy(MENU restrict menu_to_change, int new_header_policy, int new_footer_policy)
{
    menu_to_change->header_policy = (new_header_policy && 1) || 0;
    menu_to_change->footer_policy = (new_footer_policy && 1) || 0;
}

void change_width_policy(MENU restrict menu_to_change, int new_width_policy)
{
    menu_to_change->width_policy = (new_width_policy && 1) || 0;
}

int get_menu_options_amount(MENU menu)
{
    return menu->count;
}

int is_menu_running(MENU menu)
{
    return menu->running;
}

int get_menu_selected_index(MENU menu)
{
    return menu->selected_index;
}

const char* get_menu_header(MENU menu)
{
    return menu->header;
}

const char* get_menu_footer(MENU menu)
{
    return menu->footer;
}

int get_menu_header_policy(MENU menu)
{
    return menu->header_policy;
}

int get_menu_footer_policy(MENU menu)
{
    return menu->footer_policy;
}

int get_menu_width_policy(MENU menu)
{
    return menu->width_policy;
}

MENU create_menu()
{
    static int _initialized = 0;
    if (_initialized ^ 1)
        {
            _init_menu_system();
            _initialized = 1;
        }
    MENU new_menu = (MENU)malloc(sizeof(struct __menu));

    new_menu->count = 0;
    new_menu->options = NULL;
    new_menu->running = 0;
    new_menu->active_buffer = 0;
    new_menu->selected_index = 0;
    new_menu->footer_policy = 1;
    new_menu->header_policy = 1;
    new_menu->width_policy = 1;
    new_menu->__ID = rand() * rand();

    new_menu->hBuffer[0] = _createConsoleScreenBuffer();
    new_menu->hBuffer[1] = _createConsoleScreenBuffer();

    new_menu->menu_size = zero_point;

    new_menu->header = DEFAULT_HEADER_TEXT;
    new_menu->footer = DEFAULT_FOOTER_TEXT;

    _getMenuSize(new_menu);

    menus_array = (MENU*)realloc(menus_array, (menus_amount + 1) * sizeof(MENU));
    menus_array[menus_amount++] = new_menu;

    return new_menu;
}

MENU_ITEM create_menu_item(const char* restrict text, menu_callback callback)
{
    MENU_ITEM item = (MENU_ITEM)malloc(sizeof(struct __menu_item));
    if (!item) return NULL;
    item->text = (text != NULL) ? strdup(text) : strdup(DEFAULT_MENU_TEXT);
    item->callback = (callback != NULL) ? callback : NULL;
    return item;
}

int add_option(MENU used_menu, const MENU_ITEM item)
{
    if (!used_menu) return 0;

    size_t new_size = used_menu->count + 1;
    MENU_ITEM* new_options = (MENU_ITEM*)realloc(used_menu->options, new_size * sizeof(MENU_ITEM));
    if (!new_options) return 0;

    used_menu->options = new_options;
    used_menu->options[used_menu->count] = item;
    used_menu->count = new_size;
    _getMenuSize(used_menu);

    return 1;
}

void change_header(MENU used_menu, const char* restrict text)
{
    used_menu->header = text;
}

void change_footer(MENU used_menu, const char* restrict text)
{
    used_menu->footer = text;
}

void enable_menu(MENU used_menu)
{
    if (!used_menu || used_menu->count == 0)
        {
            _lwrite_string(hConsoleError, "Error: you can\'t enable the menu when it has no options. Add options using the \'add_option\' function!");
            exit(1);
        }

    if (_size_check(used_menu, TRUE, 0))
        {
            clear_menu(used_menu);
            return;
        }

    used_menu->running = 1;
    used_menu->selected_index = 0;

    _setConsoleActiveScreenBuffer(used_menu->hBuffer[used_menu->active_buffer]);
    _renderMenu(used_menu);
}

void clear_option(MENU used_menu, MENU_ITEM option_to_clear)
{
    MENU_ITEM* o = used_menu->options;
    for (int i = 0; i < used_menu->count; i++)
        if (o[i] == option_to_clear)
            {
                free((void*)o[i]->text);
                free(o[i]);
                for (int k = i + 1; k < used_menu->count; k++)
                    o[k-1] = o[k];
                used_menu->count--;

                used_menu->selected_index = 0;
                _getMenuSize(used_menu);

                if (used_menu->count <= 0) clear_menu(used_menu);
                else
                    {
                        MENU_ITEM* temp = (MENU_ITEM*)realloc(used_menu->options, used_menu->count * sizeof(MENU_ITEM));
                        if (temp) used_menu->options = temp;
                    }
                break;
            }
}

void clear_menu(MENU menu_to_clear)
{
    for (int i = 0; i < menus_amount; i++)
        if (menus_array[i] == menu_to_clear)
            {
                MENU m = menus_array[i];
                if (m->options != NULL && m->count > 0)
                    {
                        for (int j = 0; j < m->count; j++)
                            {
                                free((void*)m->options[j]->text);
                                free(m->options[j]);
                            }
                        free(m->options);
                        m->options = NULL;
                    }
                if (m->hBuffer[0] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[0]);
                if (m->hBuffer[1] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[1]);
                m->running = FALSE;

                menus_array[i] = NULL;

                for (int k = i + 1; k < menus_amount; k++) menus_array[k-1] = menus_array[k];
                menus_amount--;
                if (menus_amount <= 0)
                    {
                        free(menus_array);
                        menus_array = NULL;
                        _setConsoleActiveScreenBuffer(hConsole);
                    }
                else
                    {
                        menus_array = (MENU*)realloc(menus_array, menus_amount * sizeof(MENU));
                        menus_array[0]->running = TRUE;	// in case so we always have something to hold our back
                    }
                break;
            }
}

void clear_menus()
{
    while(menus_amount > 0) clear_menu(menus_array[0]);
}

void clear_menus_and_exit()
{
    while(menus_amount > 0) clear_menu(menus_array[0]);
    exit(0);
}

/*
    RESTRICTED ACCESS
*/
void _init_menu_system()
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hConsoleError = GetStdHandle(STD_ERROR_HANDLE);
    hCurrent = hConsole;
    zero_point = saved_buffer_size = bufferSize = (COORD) {0, 0},
    _init_hError();
    srand(time(NULL));
}

void _init_hError()
{
    _hError = _createConsoleScreenBuffer();
}

HANDLE _createConsoleScreenBuffer()
{
    return CreateConsoleScreenBuffer(
               GENERIC_READ | GENERIC_WRITE,
               FILE_SHARE_READ | FILE_SHARE_WRITE,
               NULL,
               CONSOLE_TEXTMODE_BUFFER,
               NULL
           );
}

void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text)
{
    SetConsoleCursorPosition(hDestination, (COORD)
    {
        x, y
    });
    _lwrite_string(hDestination, text);
}

void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text, ...)
{
    va_list args;
    va_start(args, text);

    SetConsoleCursorPosition(hDestination, (COORD)
    {
        x, y
    });

    _vwrite_string(hDestination, text, args);
    va_end(args);
}

void _write_string(HANDLE hDestination, const char* restrict text, ...)
{
    va_list args;
    va_start(args, text);
    _vwrite_string(hDestination, text, args);
    va_end(args);
}

void _vwrite_string(HANDLE hDestination, const char* restrict text, va_list args)
{
    char buffer[BUFFER_CAPACITY];
    vsnprintf(buffer, BUFFER_CAPACITY, text, args);
    _lwrite_string(hDestination, buffer);
}

void _lwrite_string(HANDLE hDestination, const char* restrict text)
{
    WriteConsoleA(hDestination, (const void*)text, (DWORD)strlen(text), &written, NULL);
}

void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate)
{
    SetConsoleActiveScreenBuffer(hBufferToActivate);
    hCurrent = hBufferToActivate;
}

void _getMenuSize(MENU menu)
{
    int max_width = 1, current_width;
    for (int i = 0; i < menu->count; i++)
        {
            current_width = strlen(menu->options[i]->text);
            if (current_width > max_width) max_width = current_width;
        }

    menu->menu_size.X = (max_width + 4) * ((menu->width_policy) ? 2 : 1);
    menu->menu_size.Y = menu->count * 2 + 6;
}

BYTE _size_check(MENU menu, BYTE show_error, int extra_value)
{
    GetConsoleScreenBufferInfo(hCurrent, &hBack_csbi);
    int screen_width = hBack_csbi.srWindow.Right - hBack_csbi.srWindow.Left + 1;
    int screen_height = hBack_csbi.srWindow.Bottom - hBack_csbi.srWindow.Top + 1;
    BYTE size_error = (screen_width < menu->menu_size.X + extra_value) | (screen_height < menu->menu_size.Y + extra_value);

    if (size_error && show_error)
        _show_error_and_wait(error_message, menu->menu_size.X, menu->menu_size.Y, screen_width, screen_height);
    return size_error;

}

void _initWindow(SMALL_RECT* restrict window, COORD size)
{
    window->Top = window->Left = 0;
    window->Right = size.X - 1;
    window->Bottom = size.Y - 1;
}

HANDLE _find_first_active_menu_buffer()
{
    for (int i = menus_amount - 1; i >= 0; i--)
        if (menus_array[i]->running) return menus_array[i]->hBuffer[menus_array[i]->active_buffer]; // returns current active hBuffer
    exit(1); // we should NEVER get here
}

void _clear_buffer(HANDLE hBuffer)
{
    SetConsoleCursorPosition(hBuffer, zero_point);
    if (GetConsoleScreenBufferInfo(hBuffer, &hBack_csbi))
        {
            bufferSize = hBack_csbi.dwSize;
            if (saved_buffer_size.X != bufferSize.X || saved_buffer_size.Y != bufferSize.Y)
                saved_size = bufferSize.X * bufferSize.Y;

            FillConsoleOutputCharacter(hBuffer, ' ', saved_size, zero_point, &written);
            FillConsoleOutputAttribute(hBuffer,
                                       FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
                                       saved_size, zero_point, &written);
        }
}

HANDLE _block_input(DWORD* oldMode)
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin != INVALID_HANDLE_VALUE)
        {
            GetConsoleMode(hStdin, oldMode);
            DWORD newMode = *oldMode & ~ENABLE_QUICK_EDIT_MODE;
            newMode |= ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
            SetConsoleMode(hStdin, newMode);
        }
    return hStdin;
}

int _check_menu(unsigned long long saved_id)
{
    int menu_still_exists = 0;
    for (int i = 0; i < menus_amount; i++)
        if (menus_array[i] && menus_array[i]->__ID == saved_id && menus_array[i]->running == TRUE)
            {
                menu_still_exists = 1;
                break;
            }

    return menu_still_exists;
}

void _show_error_and_wait(const char* restrict message, ...)
{
    _clear_buffer(_hError);
    _setConsoleActiveScreenBuffer(_hError);
    va_list args;
    va_start(args, message);
    _vwrite_string(_hError, message, args);
    va_end(args);

    _lwrite_string(_hError, "\n\nPress Enter to continue...");
    getchar();
    _clear_buffer(_hError);
}

void _renderMenu(const MENU used_menu)
{
    if (!used_menu) return;
    register int current_width, current_height, old_width, old_height;
    int start_x, start_y;
    int y, x, i;
    unsigned long long saved_id;

    BYTE size_check = 0, size_error = 0;

    HANDLE hBackBuffer = used_menu->hBuffer[0];
    CONSOLE_SCREEN_BUFFER_INFO active_csbi, csbi;
    SMALL_RECT active_window, old_window, new_window;
    COORD menu_size, newSize;
    DWORD old_mode, numEvents, event;
    WORD vk;
    INPUT_RECORD inputRecords[EVENT_MAX_RECORDS];
    const char* format;

    GetConsoleScreenBufferInfo(used_menu->hBuffer[used_menu->active_buffer], &csbi);
    old_window = csbi.srWindow;
    old_width = old_window.Right - old_window.Left + 1;
    old_height = old_window.Bottom - old_window.Top + 1;

    menu_size = used_menu->menu_size;
    saved_id = used_menu->__ID;

    used_menu->need_redraw = 1;

    int spaces = (menu_size.X - 6) / 2;
    HANDLE hStdin = _block_input(&old_mode);

    char header[BUFFER_CAPACITY];
    snprintf(header, sizeof(header), HEADER "%*s%s%*s" RESET,
             spaces, "", used_menu->header, spaces, "");

    char footer[BUFFER_CAPACITY];
    snprintf(footer, sizeof(footer), SUBHEADER " %-*s " RESET,
             menu_size.X - 4, used_menu->footer);

    while (used_menu->running)
        {
            if (size_error) GetConsoleScreenBufferInfo(_hError, &active_csbi);
            else GetConsoleScreenBufferInfo(used_menu->hBuffer[used_menu->active_buffer], &active_csbi);

            active_window = active_csbi.srWindow;
            current_width = active_window.Right - active_window.Left + 1;
            current_height = active_window.Bottom - active_window.Top + 1;

            if ((old_width != current_width) | (old_height != current_height))
                {
                    newSize.X = current_width;
                    newSize.Y = current_height;
                    old_width = current_width;
                    old_height = current_height;

                    size_check = (current_width < menu_size.X) | (current_height < menu_size.Y);

                    _initWindow(&new_window, newSize);

                    if (size_check)
                        {
                            size_error = 1;
                            SetConsoleScreenBufferSize(_hError, newSize);
                            SetConsoleWindowInfo(_hError, TRUE, &new_window);
                            _clear_buffer(_hError);
                            _draw_at_position(_hError, 0, 0, error_message, menu_size.X, menu_size.Y, current_width, current_height);
                            _setConsoleActiveScreenBuffer(_hError);
                            continue;
                        }
                    else if (size_error) size_error = 0;

                    SetConsoleScreenBufferSize(used_menu->hBuffer[0], newSize);
                    SetConsoleScreenBufferSize(used_menu->hBuffer[1], newSize);
                    SetConsoleWindowInfo(used_menu->hBuffer[0], TRUE, &new_window);
                    SetConsoleWindowInfo(used_menu->hBuffer[1], TRUE, &new_window);
                    used_menu->need_redraw = 1;
                }

            if (used_menu->need_redraw)
                {
                    hBackBuffer = used_menu->hBuffer[used_menu->active_buffer ^ 1];
                    _clear_buffer(hBackBuffer);
                    //_draw_at_position(hBackBuffer, 0, 0, "__ID: %llu", used_menu->__ID);

                    start_x = (current_width - menu_size.X) / 2;
                    start_y = (current_height - menu_size.Y) / 2;

                    // header
                    if (used_menu->header_policy)
                        _ldraw_at_position(hBackBuffer, start_x, start_y, header);
                    else start_y -= 2;

                    // options
                    y = start_y + 2;
                    x = start_x + 2;

                    for (i = 0; i < used_menu->count; i++)
                        {
                            format = (i == used_menu->selected_index) ?
                                     HIGHLIGHT "%s" RESET : "%s";
                            _draw_at_position(hBackBuffer, x, y, format, used_menu->options[i]->text);
                            y++;
                        }

                    // footer
                    if (used_menu->footer_policy)
                        _ldraw_at_position(hBackBuffer, start_x, y + 1, footer);


                    _setConsoleActiveScreenBuffer(hBackBuffer);
                    used_menu->active_buffer ^= 1;
                    used_menu->need_redraw = 0;
                }
            if (size_error ^ 1 && GetNumberOfConsoleInputEvents(hStdin, &numEvents) && numEvents > 0)
                {
                    BYTE is_native_key = 0;
                    ReadConsoleInput(hStdin, inputRecords, EVENT_MAX_RECORDS, &numEvents);
                    for (event = 0; event < numEvents && is_native_key ^ 1; event++)
                        switch(inputRecords[event].EventType)
                            {
                                case KEY_EVENT:
                                    if (inputRecords[event].Event.KeyEvent.bKeyDown)
                                        {
                                            vk = inputRecords[event].Event.KeyEvent.wVirtualKeyCode;
                                            is_native_key = (vk == VK_UP) || (vk == VK_DOWN) || (vk == VK_RETURN) || (vk == VK_ESCAPE) || (vk == VK_DELETE);
                                            if (is_native_key)
                                                {
                                                    used_menu->need_redraw = 1;
                                                    switch (vk)
                                                        {
                                                            case VK_UP:
                                                                used_menu->selected_index =
                                                                    (used_menu->selected_index - 1 + used_menu->count) % used_menu->count;
                                                                break;

                                                            case VK_DOWN:
                                                                used_menu->selected_index =
                                                                    (used_menu->selected_index + 1) % used_menu->count;
                                                                break;

                                                            case VK_RETURN: // ENTER
                                                                if (used_menu->options[used_menu->selected_index]->callback)
                                                                    {
                                                                        _clear_buffer(hConsole);
                                                                        _setConsoleActiveScreenBuffer(hConsole);
                                                                        SetConsoleMode(hStdin, old_mode);

                                                                        used_menu->options[used_menu->selected_index]->callback((void*)used_menu);

                                                                        if (_check_menu(saved_id))
                                                                            {
                                                                                if (_size_check(used_menu, FALSE, 0)) exit(1);
                                                                                else
                                                                                    {
                                                                                        _setConsoleActiveScreenBuffer(hBackBuffer);
                                                                                        _block_input(&old_mode);
                                                                                        used_menu->need_redraw = 1;
                                                                                    }
                                                                            }
                                                                        else goto end_render_loop;
                                                                    }
                                                                break;

                                                            case VK_ESCAPE:
                                                                clear_menu(used_menu);
                                                                break;
                                                            case VK_DELETE:
                                                                if (DEBUG)
                                                                    clear_option(used_menu, used_menu->options[used_menu->selected_index]);
                                                                break;
                                                        }
                                                }
                                        }
                                    break;
                            }
                }
            Sleep(UPDATE_FREQUENCE);
        }

end_render_loop:; // anchor

    // cleanup
    if (menus_amount == 0)
        {
            SetConsoleMode(hStdin, old_mode);
            _setConsoleActiveScreenBuffer(hConsole);
        }
    else
        _setConsoleActiveScreenBuffer(_find_first_active_menu_buffer());
    free(used_menu);
}