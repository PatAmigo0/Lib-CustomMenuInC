#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <string.h>
#include <stdarg.h>
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

// restricted functions prototypes
void _init_menu_system();
void _init_hError();
HANDLE _createConsoleScreenBuffer(void);
void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text, ...);
void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text);
void _write_string(HANDLE hDestination, const char* text, ...);
void _vwrite_string(HANDLE hDestination, const char* text, va_list args);
void _lwrite_string(HANDLE hDestination, const char* text);
void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate);
void _getMenuSize(MENU menu);  // Changed parameter type
BYTE _size_check(MENU menu, BYTE show_error, int extra_value);
void _initWindow(SMALL_RECT* window, COORD* size);
void _clear_buffer(HANDLE hBuffer);
int _check_menu(int saved_id);
HANDLE _block_input(DWORD* oldMode);
void _renderMenu(MENU used_menu);  // Changed parameter type
void _show_error_and_wait(const char* message, ...);

double tick()
{
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    static LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
}

void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy)
{
    menu_to_change->header_policy = new_header_policy;
    menu_to_change->footer_policy = new_footer_policy;
}

MENU create_menu()
{
	static int _initialized = 0;
    if (_initialized ^ 1)
	{
		_init_menu_system();
		_initialized = 1;
	}
    MENU new_menu = (MENU)malloc(sizeof(__menu));

    new_menu->count = 0;
    new_menu->options = NULL;
    new_menu->running = 0;
    new_menu->active_buffer = 0;
    new_menu->selected_index = 0;
    new_menu->footer_policy = 1;
    new_menu->header_policy = 1;
    new_menu->__ID = menus_amount;

    new_menu->hBuffer[0] = _createConsoleScreenBuffer();
    new_menu->hBuffer[1] = _createConsoleScreenBuffer();

    new_menu->menu_size = (COORD)
    {
        1,1
    };

    new_menu->header = DEFAULT_HEADER_TEXT;
    new_menu->footer = DEFAULT_FOOTER_TEXT;

    _getMenuSize(new_menu);

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
    _getMenuSize(used_menu);

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
                for (int j = 0; j < m->count; j++) if (m->options[j].text) free((void*)(m->options[j].text));

                if (m->options != NULL)
                {
                	free(m->options);	
                	m->options = NULL;
				}
                if (m->hBuffer[0] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[0]);
                if (m->hBuffer[1] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[1]);
                m->running = 0;

                free(m);
                menus_array[i] = NULL;

                for (int k = i + 1; k < menus_amount; k++) menus_array[k-1] = menus_array[k];

                menus_amount--;
                if (menus_amount <= 0)
                    {
                        free(menus_array);
                        menus_array = NULL;
                    }
                else menus_array = (MENU*)realloc(menus_array, menus_amount * sizeof(MENU));
                break;
            }
}

void clear_menus()
{
    while(menus_amount > 0) clear_menu(menus_array[0]);
    exit(0);
}

/*
    RECTRICTED ACCESS
*/
void _init_menu_system()
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hConsoleError = GetStdHandle(STD_ERROR_HANDLE);
    hCurrent = hConsole;
    zero_point = saved_buffer_size = bufferSize = (COORD) {0, 0},
    _init_hError();
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

void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text)
{
    SetConsoleCursorPosition(hDestination, (COORD)
    {
        x, y
    });
    _lwrite_string(hDestination, text);
}

void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text, ...)
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
    vsnprintf(buffer, BUFFER_CAPACITY, text, args);
    _lwrite_string(hDestination, buffer);
}

void _lwrite_string(HANDLE hDestination, const char* text)
{
    WriteConsoleA(hDestination, (void*)text, (DWORD)strlen(text), &written, NULL);
}

void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate)
{
    SetConsoleActiveScreenBuffer(hBufferToActivate);
    hCurrent = hBufferToActivate;
}

void _getMenuSize(MENU menu)
{
    int max_width = 0, current_width;
    for (int i = 0; i < menu->count; i++)
        {
            current_width = strlen(menu->options[i].text);
            if (current_width > max_width) max_width = current_width;
        }

    menu->menu_size.X = (max_width + 4) * 2;
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

void _initWindow(SMALL_RECT* window, COORD* size)
{
    window->Top = window->Left = 0;
    window->Right = size->X - 1;
    window->Bottom = size->Y - 1;
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

int _check_menu(int saved_id)
{
    int menu_still_exists = 0;
    for (int i = 0; i < menus_amount; i++)
        if (menus_array[i] && menus_array[i]->__ID == saved_id)
            {
                menu_still_exists = 1;
                break;
            }

    return menu_still_exists;
}

void _show_error_and_wait(const char* message, ...)
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

void _process_input()
{
	
}

void _renderMenu(MENU used_menu)
{
    if (!used_menu) return;
    int current_width, current_height, old_width, old_height;
    int start_x, start_y;
    int y, x, i, saved_id;

    BYTE size_check = 0, size_error = 0;

    HANDLE hBackBuffer;
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

                    _initWindow(&new_window, &newSize);

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

                    {
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
                                _draw_at_position(hBackBuffer, x, y, format, used_menu->options[i].text);
                                y++;
                            }

                        // footer
                        if (used_menu->footer_policy)
                            _ldraw_at_position(hBackBuffer, start_x, y + 1, footer);

                    }

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
                                            used_menu->need_redraw = 1;
                                            is_native_key = (vk == VK_UP) || (vk == VK_DOWN) || (vk == VK_RETURN) || (vk == VK_ESCAPE) || (vk == VK_DELETE);
                                            if (is_native_key)
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

                                                                    used_menu->options[used_menu->selected_index].callback((void*)used_menu);

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
                                                                clear_option(used_menu, &(used_menu->options[used_menu->selected_index]));
                                                            break;
                                                    }
                                        }
                                    break;
                            }
                }
            Sleep(UPDATE_FREQUENCE);
        }

end_render_loop:; // anchor

    // cleanup
    SetConsoleMode(hStdin, old_mode);
    _setConsoleActiveScreenBuffer(hConsole);
}