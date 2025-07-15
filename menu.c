#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include "menu.h"

#define DEBUG 0

#define NUM_J 90
#define NUM_I 314

#define BUFFER_CAPACITY 256
#define UPDATE_FREQUENCE 20 // ms

#define DEFAULT_HEADER_TEXT "MENU"
#define DEFAULT_FOOTER_TEXT "Use arrows to navigate, Enter to select"
#define DEFAULT_MENU_TEXT "Unnamed Option"

int menus_amount = 0, depth_forbeind = 0, _initialized = 0;
MENU* menus_array = NULL;  // Changed to MENU*

HANDLE hConsole, hConsoleError, hCurrent;

// restricted functions prototypes
HANDLE _createConsoleScreenBuffer(void);
void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text, ...);
void _write_string(HANDLE hDestination, const char* text, ...);
void _vwrite_string(HANDLE hDestination, const char* text, va_list args);
void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate);
void _getMenuSize(MENU menu, COORD* minValues);  // Changed parameter type
void _initWindow(SMALL_RECT* window, COORD* size);
void _clear_buffer(HANDLE hBuffer);
HANDLE _block_input(DWORD* oldMode);
void _renderMenu(MENU used_menu);  // Changed parameter type

double tick()
{
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / freq.QuadPart;
}

void init_menu_system()
{
    if (_initialized ^ 1)
    {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        hConsoleError = GetStdHandle(STD_ERROR_HANDLE);
        hCurrent = hConsole;
        _initialized = 1;
    }
}

void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy)
{
    menu_to_change->header_policy = new_header_policy;
    menu_to_change->footer_policy = new_footer_policy;
}

MENU create_menu()
{
    if (_initialized ^ 1) init_menu_system();
    MENU new_menu = (MENU)malloc(sizeof(__menu));

    new_menu->count = 0;
    new_menu->options = NULL;
    new_menu->running = 0;
    new_menu->active_buffer = 0;
    new_menu->selected_index = 0;
    new_menu->footer_policy = 1;
    new_menu->header_policy = 1;

    new_menu->hBuffer[0] = _createConsoleScreenBuffer();
    new_menu->hBuffer[1] = _createConsoleScreenBuffer();

    new_menu->header = DEFAULT_HEADER_TEXT;
    new_menu->footer = DEFAULT_FOOTER_TEXT;

    menus_array = (MENU*)realloc(menus_array, (menus_amount + 1) * sizeof(MENU));  // Changed to MENU*
    menus_array[menus_amount++] = new_menu;

    return new_menu;
}

int add_option(MENU used_menu, menu_item* item)
{
    if (!used_menu) return 0;

    size_t new_size = used_menu->count + 1;
    menu_item* new_options = (menu_item*)realloc(used_menu->options, new_size * sizeof(menu_item));
    if (!new_options) return 0;

    used_menu->options = new_options;
    menu_item* new_item = &used_menu->options[used_menu->count];

    new_item->text = item->text ? strdup(item->text) : strdup(DEFAULT_MENU_TEXT);
    new_item->callback = item->callback;

    used_menu->count = new_size;
    return 1;
}



void change_header(MENU used_menu, const char* text)
{
    used_menu->header = text;
}

void change_footer(MENU used_menu, const char* text)
{
    used_menu->footer = text;
}

void enable_menu(MENU used_menu)
{
    if (!used_menu || used_menu->count == 0)
    {
        _write_string(hConsoleError, "Error: you can\'t enable the menu when it has no options. Add options using the \'add_option\' function!");
        exit(1);
    }

    if (depth_forbeind)
        for (int i = 0; i < menus_amount; i++)
            menus_array[i]->running = 0;

    used_menu->running = 1;
    used_menu->selected_index = 0;

    _setConsoleActiveScreenBuffer(used_menu->hBuffer[used_menu->active_buffer]);
    _renderMenu(used_menu);
}

void clear_option(MENU used_menu, menu_item* option_to_clear)
{
    menu_item* o = used_menu->options;
    for (int i = 0; i < used_menu->count; i++)
        if (&o[i] == option_to_clear)
        {
            for (int k = i + 1; k < used_menu->count; k++)
                o[k - 1] = o[k];
            used_menu->count--;
            if (used_menu->count <= 0)
            {
                free(used_menu->options);
                used_menu->options = NULL;
            }
            else o = (menu_item*)realloc(o, used_menu->count * sizeof(menu_item));
            break;
        }
    used_menu->selected_index = 0;

    if (used_menu->count <= 0) clear_menu(used_menu);
}

void clear_menu(MENU menu_to_clear)
{
    for (int i = 0; i < menus_amount; i++)
        if (menus_array[i] == menu_to_clear)
        {
            MENU m = menus_array[i];
            for (int j = 0; j < m->count; j++)
                if (m->options[j].text) free(m->options[j].text);

            if (m->options != NULL)
                free(m->options);
            if (m->hBuffer[0] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[0]);
            if (m->hBuffer[1] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[1]);
            m->running = 0;

            free(m);
            for (int k = i + 1; k < menus_amount; k++)
                menus_array[k-1] = menus_array[k];

            menus_amount--;
            if (menus_amount <= 0)
            {
                free(menus_array);
                menus_array = NULL;
                exit(1);
            }
            else
            {
                menus_array = (MENU*)realloc(menus_array, menus_amount * sizeof(MENU));
                menus_array[0]->running = 1;
            }
            break;
        }
}

void clear_menus()
{
    for (int i = 0; i < menus_amount; i++)
        clear_menu(menus_array[i]);

    free(menus_array);
    menus_array = NULL;
    menus_amount = 0;
    exit(0);
}

/*
    RECTRICTED ACCESS
*/
HANDLE _createConsoleScreenBuffer(void)
{
    return CreateConsoleScreenBuffer(
               GENERIC_READ | GENERIC_WRITE,
               FILE_SHARE_READ | FILE_SHARE_WRITE,
               NULL,
               CONSOLE_TEXTMODE_BUFFER,
               NULL
           );
}

void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text, ...)
{
    va_list args;
    va_start(args, text);
    SetConsoleCursorPosition(hDestination, (COORD){ x, y });
    _vwrite_string(hDestination, text, args);
    va_end(args);
}

void _write_string(HANDLE hDestination, const char* text, ...)
{
    va_list args;
    va_start(args, text);
    _vwrite_string(hDestination, text, args);
    va_end(args);
}

void _vwrite_string(HANDLE hDestination, const char* text, va_list args)
{
    char buffer[BUFFER_CAPACITY];
    DWORD written;

    vsnprintf(buffer, BUFFER_CAPACITY, text, args);
    WriteConsoleA(hDestination, buffer, (DWORD)strlen(buffer), &written, NULL);
}

void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate)
{
    SetConsoleActiveScreenBuffer(hBufferToActivate);
    hCurrent = hBufferToActivate;
}

void _getMenuSize(MENU menu, COORD* minValues)
{
    int max_width = 0;
    for (int i = 0; i < menu->count; i++)
    {
        int current_width = strlen(menu->options[i].text);
        if (current_width > max_width) max_width = current_width;
    }

    minValues->X = max_width + 4;
    minValues->Y = menu->count * 2 + 6;
}

void _initWindow(SMALL_RECT* window, COORD* size)
{
    window->Top = window->Left = 0;
    window->Right = size->X - 1;
    window->Bottom = size->Y - 1;
}

void _clear_buffer(HANDLE hBuffer)
{
    SetConsoleCursorPosition(hBuffer, (COORD){0, 0});
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hBuffer, &csbi))
    {
        COORD bufferSize = csbi.dwSize;
        DWORD written;
        COORD coord = {0, 0};
        DWORD size = bufferSize.X * bufferSize.Y;

        FillConsoleOutputCharacter(hBuffer, ' ', size, coord, &written);
        FillConsoleOutputAttribute(hBuffer,
                                   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
                                   size, coord, &written);
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

void _renderMenu(MENU used_menu)
{
    if (!used_menu) return;

    int current_width, current_height, old_width, old_height;
    int start_x, start_y;

    HANDLE hBackBuffer;
    CONSOLE_SCREEN_BUFFER_INFO active_csbi, csbi;
    SMALL_RECT active_window, back_window, old_window;
    COORD menu_size, visible_size;
    DWORD old_mode, written, numEvents;
    INPUT_RECORD inputRecords[8];
    const char* error_message =
        ERROR_COLOR
        "Error: Console window size is too small!\n"
        "Required size: %d x %d\n"
        "Current size: %d x %d\n"
        "Make window bigger."
        RESET;

    GetConsoleScreenBufferInfo(used_menu->hBuffer[used_menu->active_buffer], &csbi);
    old_window = csbi.srWindow;
    old_width = old_window.Right - old_window.Left + 1;
    old_height = old_window.Bottom - old_window.Top + 1;

    used_menu->need_redraw = 1;

    _getMenuSize(used_menu, &menu_size);
    HANDLE hStdin = _block_input(&old_mode);

    while (used_menu->running)
    {
        if (GetConsoleScreenBufferInfo(used_menu->hBuffer[used_menu->active_buffer], &active_csbi))
        {
            active_window = active_csbi.srWindow;
            current_width = active_window.Right - active_window.Left + 1;
            current_height = active_window.Bottom - active_window.Top + 1;
        }
        else
            current_width = current_height = 0;

        if (old_width != current_width || old_height != current_height)
        {
            COORD newSize = { current_width, current_height };
            SetConsoleScreenBufferSize(used_menu->hBuffer[0], newSize);
            SetConsoleScreenBufferSize(used_menu->hBuffer[1], newSize);
            used_menu->need_redraw = 1;
        }

        old_width = current_width, old_height = current_height;

        if (used_menu->need_redraw)
        {
            hBackBuffer = used_menu->hBuffer[used_menu->active_buffer ^ 1];
            _initWindow(&back_window, &visible_size);
            SetConsoleWindowInfo(hBackBuffer, TRUE, &back_window);

            _clear_buffer(hBackBuffer);
            BYTE size_check = (current_width < menu_size.X) | (current_height < menu_size.Y);

            if (size_check)
            {
                SetConsoleCursorPosition(hBackBuffer, (COORD){0, 0});
                FillConsoleOutputAttribute(hBackBuffer,
                                           FOREGROUND_RED | FOREGROUND_INTENSITY,
                                           current_width * 4,
                                           (COORD){0, 0},
                                           &written);
                _write_string(hBackBuffer, error_message, menu_size.X, menu_size.Y, current_width, current_height);
            }
            else
            {
                start_x = (current_width - menu_size.X) / 2;
                start_y = (current_height - menu_size.Y) / 2;

                // header
                if (used_menu->header_policy)
                {
                    char header[BUFFER_CAPACITY];
                    int spaces = (menu_size.X - 6) / 2;
                    snprintf(header, sizeof(header), HEADER "%*s%s%*s" RESET,
                             spaces, "", used_menu->header, spaces, "");
                    _draw_at_position(hBackBuffer, start_x, start_y, header);
                }
                else start_y -= 2;

                // options
                int y = start_y + 2;
                int x = start_x + 2;
                for (int i = 0; i < used_menu->count; i++)
                {
                    const char* format = (i == used_menu->selected_index) ?
                                         HIGHLIGHT "%s" RESET : "%s";
                    _draw_at_position(hBackBuffer, x, y, format, used_menu->options[i].text);
                    y++;
                }

                // footer
                if (used_menu->footer_policy)
                {
                    char footer[BUFFER_CAPACITY];
                    snprintf(footer, sizeof(footer), SUBHEADER " %-*s " RESET,
                             menu_size.X - 4, used_menu->footer);
                    _draw_at_position(hBackBuffer, start_x, y + 1, footer);
                }
            }

            _setConsoleActiveScreenBuffer(hBackBuffer);
            used_menu->active_buffer ^= 1;
            used_menu->need_redraw = 0;
        }

        if (hStdin != INVALID_HANDLE_VALUE &&
            GetNumberOfConsoleInputEvents(hStdin, &numEvents) &&
            numEvents > 0)
        {
            ReadConsoleInput(hStdin, inputRecords, 8, &numEvents);
            for (DWORD i = 0; i < numEvents; i++)
            {
                if (inputRecords[i].EventType == KEY_EVENT &&
                    inputRecords[i].Event.KeyEvent.bKeyDown)
                {
                    WORD vk = inputRecords[i].Event.KeyEvent.wVirtualKeyCode;
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
                            if (used_menu->options[used_menu->selected_index].callback)
                            {
                                _clear_buffer(hConsole);
                                _setConsoleActiveScreenBuffer(hConsole);
                                SetConsoleMode(hStdin, old_mode);
                                used_menu->options[used_menu->selected_index].callback(used_menu);
                                menus_array[i]->running = 1;
                            }
                            _setConsoleActiveScreenBuffer(hBackBuffer);
                            _block_input(&old_mode);
                            break;

                        case VK_ESCAPE:
                            used_menu->running = 0;
                            break;
                        case VK_DELETE:
                            if (DEBUG)
                                clear_option(used_menu, &(used_menu->options[used_menu->selected_index]));
                            break;
                    }
                }
            }
        }
        else Sleep(UPDATE_FREQUENCE);
    }

    // cleanup
    SetConsoleMode(hStdin, old_mode);
    _setConsoleActiveScreenBuffer(hConsole);
}