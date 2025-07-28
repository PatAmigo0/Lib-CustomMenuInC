#include "menu.h"

// #define DEBUG

#define DISABLED -1

#define BUFFER_CAPACITY 128
#define UPDATE_FREQUENCE 2147483647 // ms
#define ERROR_UPDATE_FREQUENCE 20 // ms
#define EVENT_MAX_RECORDS 6

#define FIX_VALUE 2

#define DEFAULT_HEADER_TEXT "MENU"
#define DEFAULT_FOOTER_TEXT "Use arrows to navigate, Enter to select"
#define DEFAULT_MENU_TEXT "Unnamed Option"

#define LOG_FILE_NAME "menu_log.txt"

// cached values
static COORD zero_point = {0, 0};
static DWORD written = 0;
static COORD cached_size = {0, 0}; // saved screen size after the last _size_check function call
static HANDLE hConsole, hConsoleError, hCurrent, _hError, hStdin; // cached handlers
static CONSOLE_SCREEN_BUFFER_INFO hBack_csbi; // cached csbi

BYTE menu_settings_initialized = 0, menu_color_initialized = 0;

MENU_SETTINGS MENU_DEFAULT_SETTINGS;
MENU_COLOR MENU_DEFAULT_COLOR;

// dummy values
static INPUT input = {0};

// mouse flag
static int holding = 0;

// menu values
static MENU* menus_array = NULL;
static int menus_amount = 0;

const char* error_message =
    BRIGHT_RED_TEXT
    "Error: Console window size is too small!\n"
    "Required size: %d x %d\n"
    "Current size: %d x %d\n"
    "Make window bigger."
    RESET_ALL_STYLES;

struct __menu_item
{
    COORD boundaries;
    int text_len;
    const char* text;
    menu_callback callback;
    dpointer data_chunk;
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
    int mouse_enabled;
    HANDLE hBuffer[2];
    int selected_index;

    COORD current_size;

    MENU_COLOR color_object;

    int footer_policy;
    int header_policy;

    const char* footer;
    const char* header;

    COORD menu_size;
}; // protecting menu

// restricted functions prototypes
static MENU_SETTINGS _create_default_settings();
static MENU_COLOR _create_default_color();
static unsigned long long _random_uint64_t();
static void _init_menu_system();
static void _init_hError();
static HANDLE _createConsoleScreenBuffer(void);
static void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text, ...);
static void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text);
static void _write_string(HANDLE hDestination, const char* restrict text, ...);
static void _vwrite_string(HANDLE hDestination, const char* restrict text, va_list args);
static void _lwrite_string(HANDLE hDestination, const char* restrict text);
static void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate);
static void _getMenuSize(MENU menu);
static BYTE _size_check(MENU menu, BYTE show_error, int extra_value);
static void _initWindow(SMALL_RECT* restrict window, COORD size);
static void _clear_buffer(HANDLE hBuffer);
static void _reset_mouse_state();
static int _check_menu(unsigned long long saved_id);
static void _block_input(DWORD* oldMode);
static void _renderMenu(const MENU used_menu);
static void _show_error_and_wait(const char* restrict message, ...);
static void _show_error_and_wait_extended(MENU menu);
static HANDLE _find_first_active_menu_buffer();  // Add this declaration if missing

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

void toggle_mouse(MENU restrict menu_to_change)
{
    menu_to_change->mouse_enabled = menu_to_change->mouse_enabled ^ 1;
}

void set_default_menu_settings(MENU_SETTINGS new_settings)
{
    if (menu_settings_initialized) free(MENU_DEFAULT_SETTINGS);
    else menu_settings_initialized = 1;
    MENU_DEFAULT_SETTINGS = new_settings;
}

void set_color_object(MENU menu, MENU_COLOR color_object)
{
    free(menu->color_object);
    menu->color_object = color_object;
}

void set_default_color_object(MENU_COLOR color_object)
{
    if (menu_color_initialized) free(MENU_DEFAULT_COLOR);
    else menu_color_initialized = 1;
    MENU_DEFAULT_COLOR = color_object;
}

MENU create_menu()
{
    static int _initialized = 0;
    if (_initialized ^ 1)
        {
            _init_menu_system();
            _initialized = 1;
        }
    MENU new_menu = (MENU)calloc(1, sizeof(struct __menu));

    memset(new_menu, 0, sizeof(struct __menu));
    new_menu->count = 0;
    new_menu->mouse_enabled = MENU_DEFAULT_SETTINGS->mouse_enabled;
    new_menu->options = NULL;
    new_menu->running = 0;
    new_menu->active_buffer = 0;
    new_menu->selected_index = 0;
    new_menu->footer_policy = 1;
    new_menu->header_policy = 1;
    new_menu->width_policy = 1;
    new_menu->__ID = _random_uint64_t();

    new_menu->color_object = create_color_object();

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

MENU_ITEM create_menu_item(const char* restrict text, menu_callback callback, void* callback_data)
{
    MENU_ITEM item = (MENU_ITEM)malloc(sizeof(struct __menu_item));
    if (!item) return NULL;
    item->text = (text != NULL) ? text : DEFAULT_MENU_TEXT;
    item->text_len = strlen(item->text);
    item->boundaries = (COORD)
    {
        item->text_len - 1, 0
    };
    item->callback = callback;
    item->data_chunk = callback_data;
    return item;
}

MENU_SETTINGS create_new_settings()
{
    MENU_SETTINGS new_settings = (MENU_SETTINGS)calloc(1, sizeof(struct __menu_settings));
    memset(new_settings, 0, sizeof(struct __menu_settings));
    return new_settings;
}

MENU_COLOR create_color_object()
{
    if (menu_color_initialized ^ 1) _init_menu_system();

    MENU_COLOR new_color_object = (MENU_COLOR)calloc(1, sizeof(struct __menu_color_object));

    new_color_object->headerColor = MENU_DEFAULT_COLOR->headerColor;
    new_color_object->footerColor = MENU_DEFAULT_COLOR->footerColor;

    return new_color_object;
}

int add_option(MENU used_menu, const MENU_ITEM restrict item)
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

    if (_size_check(used_menu, FALSE, 0))
        {
            _show_error_and_wait_extended(used_menu);
            //clear_menu(used_menu);
            //return;
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
                free(o[i]);
                size_t elements_to_move = used_menu->count - i - 1;
                if (elements_to_move > 0) memmove(&o[i], &o[i+1], elements_to_move * sizeof(MENU_ITEM));
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
                            free(m->options[j]);
                        free(m->options);
                        m->options = NULL;
                    }

                free(m->color_object);

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
    free(MENU_DEFAULT_SETTINGS);
    free(MENU_DEFAULT_COLOR);
    exit(0);
}

/*
    RESTRICTED ACCESS
*/
static MENU_COLOR _create_default_color()
{
    MENU_COLOR new_color_object = (MENU_COLOR)calloc(1, sizeof(struct __menu_color_object));

    /* defaults */
    new_color_object->headerColor = DARK_BLUE_BG_WHITE_TEXT;
    new_color_object->footerColor = CYAN_BG_BLACK_TEXT;
    /*          */

    return new_color_object;
}

static MENU_SETTINGS _create_default_settings()
{
    MENU_SETTINGS new_settings = create_new_settings();
    return new_settings;
}

static unsigned long long _random_uint64_t()
{
    return ((unsigned long long)rand() << 32) | rand();
}

static void _init_menu_system()
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hConsoleError = GetStdHandle(STD_ERROR_HANDLE);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hCurrent = hConsole;
    _init_hError();

    // dummy value initialization
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

    if (menu_settings_initialized ^ 1) set_default_menu_settings(_create_default_settings());

    if (menu_color_initialized ^ 1) set_default_color_object(_create_default_color());

    srand(time(NULL));
}

static void _init_hError()
{
    _hError = _createConsoleScreenBuffer();
}

static HANDLE _createConsoleScreenBuffer()
{
    return CreateConsoleScreenBuffer(
               GENERIC_READ | GENERIC_WRITE,
               FILE_SHARE_READ | FILE_SHARE_WRITE,
               NULL,
               CONSOLE_TEXTMODE_BUFFER,
               NULL
           );
}

static void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text)
{
    SetConsoleCursorPosition(hDestination, (COORD)
    {
        x, y
    });
    _lwrite_string(hDestination, text);
}

static void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text, ...)
{
    va_list args;
    va_start(args, text);

    SetConsoleCursorPosition(hDestination, (COORD)
    {
        x, y
    });

    //_write_string(hDestination, "\x1b[%d;%dH", y+1, x+1);
    _vwrite_string(hDestination, text, args);
    va_end(args);
}

static void _write_string(HANDLE hDestination, const char* restrict text, ...)
{
    va_list args;
    va_start(args, text);
    _vwrite_string(hDestination, text, args);
    va_end(args);
}


static void _vwrite_string(HANDLE hDestination, const char* restrict text, va_list args)
{
    char buffer[BUFFER_CAPACITY];
    vsnprintf(buffer, BUFFER_CAPACITY, text, args);
    _lwrite_string(hDestination, buffer);
}

static void _lwrite_string(HANDLE hDestination, const char* restrict text)
{
    WriteConsoleA(hDestination, (const void*)text, (DWORD)strlen(text), &written, NULL);
}

static void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate)
{
    SetConsoleActiveScreenBuffer(hBufferToActivate);
    hCurrent = hBufferToActivate;
}

static void _getMenuSize(MENU menu)
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

static BYTE _size_check(MENU menu, BYTE show_error, int extra_value)
{
    GetConsoleScreenBufferInfo(hCurrent, &hBack_csbi);
    int screen_width = hBack_csbi.srWindow.Right - hBack_csbi.srWindow.Left + 1;
    int screen_height = hBack_csbi.srWindow.Bottom - hBack_csbi.srWindow.Top + 1;
    BYTE size_error = (screen_width < menu->menu_size.X + extra_value) | (screen_height < menu->menu_size.Y + extra_value);

    if (size_error && show_error)
        _show_error_and_wait(error_message, menu->menu_size.X, menu->menu_size.Y, screen_width, screen_height);
    cached_size = (COORD) // caching
    {
        screen_width, screen_height
    };
    return size_error;

}

static void _initWindow(SMALL_RECT* restrict window, COORD size)
{
    window->Top = window->Left = 0;
    window->Right = size.X - 1;
    window->Bottom = size.Y - 1;
}

static HANDLE _find_first_active_menu_buffer()
{
    for (int i = menus_amount - 1; i >= 0; i--)
        if (menus_array[i]->running) return menus_array[i]->hBuffer[menus_array[i]->active_buffer]; // returns current active hBuffer
    exit(69); // we should NEVER get there
}

static void _clear_buffer(HANDLE hBuffer)
{
    _lwrite_string(hBuffer, CLEAR_SCREEN); // clear visible screen
    _lwrite_string(hBuffer, CLEAR_SCROLL_BUFFER); // clear scrollback
}

static void _block_input(DWORD* oldMode)
{
    GetConsoleMode(hStdin, oldMode);

    DWORD newMode = *oldMode;
    newMode &= ~(ENABLE_QUICK_EDIT_MODE);
    newMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    newMode |= ENABLE_WINDOW_INPUT;
    newMode |= ENABLE_MOUSE_INPUT;
    newMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    SetConsoleMode(hStdin, newMode);
}

static void _reset_mouse_state()
{
    SendInput(1, &input, sizeof(INPUT));
}

static int _check_menu(unsigned long long saved_id)
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

static void _show_error_and_wait(const char* restrict message, ...)
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

static void _show_error_and_wait_extended(MENU menu)
{
    COORD menu_size = menu->menu_size;
    SetConsoleScreenBufferSize(_hError, menu_size);
    GetConsoleScreenBufferInfo(menu->hBuffer[menu->active_buffer], &hBack_csbi);
    SMALL_RECT cr_window = hBack_csbi.srWindow;
    COORD current_size = (COORD)
    {
        cr_window.Right - cr_window.Left + 1, cr_window.Bottom - cr_window.Top + 1
    };

    _draw_at_position(_hError, 0, 0, error_message, menu_size.X, menu_size.Y, current_size.X, current_size.Y);
    _setConsoleActiveScreenBuffer(_hError);
    FlushConsoleInputBuffer(hStdin);
    DWORD numEvents;

    INPUT_RECORD inputRecords[EVENT_MAX_RECORDS];

    BYTE running = 1;
    while (running)
        {
            BYTE event_running = 1;
        error_wait_start:
            ;
            DWORD objectWait = WaitForSingleObject(hStdin, UPDATE_FREQUENCE);

            if (objectWait == WAIT_OBJECT_0)
                {
                    GetNumberOfConsoleInputEvents(hStdin, &numEvents);
                    ReadConsoleInput(hStdin, inputRecords, min(EVENT_MAX_RECORDS, numEvents), &numEvents);
                    for (DWORD k = 0; k < numEvents && event_running; k++)
                        switch(inputRecords[k].EventType)
                            {
                                case WINDOW_BUFFER_SIZE_EVENT:
                                    current_size = inputRecords[k].Event.WindowBufferSizeEvent.dwSize;
                                    event_running = 0;
                                    break;
                                case MOUSE_EVENT:
                                case KEY_EVENT:
                                    goto error_wait_start;
                            }
                    if (current_size.X >= menu_size.X && current_size.Y >= menu_size.Y) running = 0;
                    else
                        {
                            _clear_buffer(_hError);
                            _draw_at_position(_hError, 0, 0, error_message, menu_size.X, menu_size.Y, current_size.X, current_size.Y);
                        }
                }

        }
    _clear_buffer(_hError);
    FlushConsoleInputBuffer(hStdin); // makes sure that no input will affect the menu after the error gets off
    _setConsoleActiveScreenBuffer(menu->hBuffer[menu->active_buffer]);
}

static void _renderMenu(const MENU used_menu)
{
    if (!used_menu) return;
    register COORD current_size, old_size, start;
    int y = 0, x = 0, i;
    unsigned long long saved_id;

    int last_selected_index = DISABLED;
    register int y_max = 0, y_min = 0, x_max = -1;

    register BYTE size_check = FALSE;

    static BYTE something_is_selected = 0;

    HANDLE hBackBuffer = used_menu->hBuffer[0];
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT old_window, new_window;
    COORD menu_size;
    DWORD old_mode, event;
    DWORD numEvents;
    WORD vk;
    INPUT_RECORD inputRecords[EVENT_MAX_RECORDS];
    const char* format;

    GetConsoleScreenBufferInfo(hCurrent, &csbi);
    old_window = csbi.srWindow;
    old_size.X = old_window.Right - old_window.Left + 1;
    old_size.Y = old_window.Bottom - old_window.Top + 1;
    current_size = old_size;

    menu_size = used_menu->menu_size;
    saved_id = used_menu->__ID;

    register BYTE mouse_input_enabled = used_menu->mouse_enabled;

    used_menu->need_redraw = 1;

    int spaces = (menu_size.X - 6) / 2;
    _block_input(&old_mode);

    char header[BUFFER_CAPACITY];
    snprintf(header, sizeof(header), "%s%*s%s%*s" RESET_ALL_STYLES, used_menu->color_object->headerColor,
             spaces, "", used_menu->header, spaces, "");

    char footer[BUFFER_CAPACITY];
    snprintf(footer, sizeof(footer), "%s %-*s " RESET_ALL_STYLES, used_menu->color_object->footerColor,
             menu_size.X - 4, used_menu->footer);

    // pre-render checks
    if (mouse_input_enabled) used_menu->selected_index = DISABLED;

    // debug values
#ifdef DEBUG
    int test_count = 0;
#endif

    // pre-render calls
    _reset_mouse_state(); // resetting the mouse state because Windows is stupid
    while (used_menu->running)
        {
#ifdef DEBUG
            _draw_at_position(hCurrent, 0, 0, "%d   ", ++test_count);
#endif

            if ((old_size.X != current_size.X) || (old_size.Y != current_size.Y))
                {
                    old_size = current_size;

                    size_check = (current_size.X < menu_size.X) || (current_size.Y < menu_size.Y);

                    _initWindow(&new_window, current_size);

                    if (size_check)
                        {
                            _show_error_and_wait_extended(used_menu);
                            continue;
                        }

                    SetConsoleScreenBufferSize(used_menu->hBuffer[0], current_size);
                    SetConsoleScreenBufferSize(used_menu->hBuffer[1], current_size);
                    SetConsoleWindowInfo(used_menu->hBuffer[0], TRUE, &new_window);
                    SetConsoleWindowInfo(used_menu->hBuffer[1], TRUE, &new_window);
                    FlushConsoleInputBuffer(hStdin); // so we wont have events recursion -.-
                    used_menu->need_redraw = 1;
                }

            if (used_menu->need_redraw)
                {
                    hBackBuffer = used_menu->hBuffer[used_menu->active_buffer ^ 1];
                    _clear_buffer(hBackBuffer);

#ifdef DEBUG
                    _draw_at_position(hBackBuffer, 0, 5, "__ID: %llu", used_menu->__ID);
                    _draw_at_position(hBackBuffer, 10, 13, "SELECTED: %d ", something_is_selected);
                    _draw_at_position(hBackBuffer, 0, 0, "%d   ", test_count);
#endif

                    start.X = (current_size.X - menu_size.X) / 2;
                    start.Y = (current_size.Y - menu_size.Y) / 2 + FIX_VALUE;

                    // header
                    if (used_menu->header_policy)
                        _ldraw_at_position(hBackBuffer, start.X, start.Y, header);
                    else start.Y -= 2;

                    // options
                    x = start.X + 2;
                    y = start.Y + 2;

                    x_max = 0;
                    y_min = y;
                    for (i = 0; i < used_menu->count; i++, y++)
                        {
                            format = (i == used_menu->selected_index) ?
                                     WHITE_BG_BLACK_TEXT "%s" RESET_ALL_STYLES : "%s";
                            _draw_at_position(hBackBuffer, x, y, format, used_menu->options[i]->text);
                            used_menu->options[i]->boundaries.Y = y;
                            used_menu->options[i]->boundaries.X = x + used_menu->options[i]->text_len - 1;
                            if (used_menu->options[i]->boundaries.X > x_max)
                                x_max = used_menu->options[i]->boundaries.X;
                        }
                    y_max = y;

                    // footer
                    if (used_menu->footer_policy)
                        _ldraw_at_position(hBackBuffer, start.X, y + 1, footer);

                    _setConsoleActiveScreenBuffer(hBackBuffer);
                    used_menu->active_buffer ^= 1;
                    used_menu->need_redraw = FALSE;
                }

        wait_start:
            ;
            DWORD waitResult = WaitForSingleObject(hStdin, UPDATE_FREQUENCE);
            if (waitResult == WAIT_OBJECT_0)
                {
                    if (GetNumberOfConsoleInputEvents(hStdin, &numEvents))
                        {
                            BYTE is_native_key = 0;
                            BYTE mouse_option_selected = 0;
                            ReadConsoleInput(hStdin, inputRecords, min(EVENT_MAX_RECORDS, numEvents), &numEvents);
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
                                                            used_menu->need_redraw = TRUE;
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
                                                                        if (used_menu->options[used_menu->selected_index]->callback && used_menu->options[used_menu->selected_index]->callback != NULL)
                                                                            {
                                                                            input_handler:
                                                                                ;
                                                                                _clear_buffer(hConsole);
                                                                                SetConsoleMode(hStdin, old_mode);
                                                                                _setConsoleActiveScreenBuffer(hConsole);

                                                                                MENU_ITEM current_option = used_menu->options[used_menu->selected_index];
                                                                                current_option->callback(used_menu, current_option->data_chunk);

                                                                                if (_check_menu(saved_id))
                                                                                    {
                                                                                        if (_size_check(used_menu, FALSE, 0)) _show_error_and_wait_extended(used_menu);
                                                                                        else
                                                                                            {
                                                                                                _setConsoleActiveScreenBuffer(hBackBuffer);
                                                                                                _block_input(&old_mode);
                                                                                                _reset_mouse_state(); // resetting the mouse state because Windows is stupid (or me)
                                                                                            }
                                                                                    }
                                                                                else goto end_render_loop;
                                                                            }
                                                                        break;

                                                                    case VK_ESCAPE:
                                                                        clear_menu(used_menu);
                                                                        break;
#ifdef DEBUG
                                                                    case VK_DELETE:
                                                                        clear_option(used_menu, used_menu->options[used_menu->selected_index]);
                                                                        break;
#endif
                                                                }
                                                            last_selected_index = used_menu->selected_index;
                                                            FlushConsoleInputBuffer(hStdin);
                                                        }
                                                }
                                            break;
                                        case MOUSE_EVENT:
#ifdef DEBUG
                                            _draw_at_position(hCurrent, 10, 2, "MOUSE POS: %d %d   ", inputRecords[event].Event.MouseEvent.dwMousePosition.X, inputRecords[event].Event.MouseEvent.dwMousePosition.Y);
#endif

                                            if (mouse_input_enabled)
                                                {
                                                    if (mouse_option_selected ^ 1)
                                                        {
                                                            register COORD mouse_pos = inputRecords[event].Event.MouseEvent.dwMousePosition;

                                                            // checking boundaries
                                                            if (mouse_pos.Y < y_max && mouse_pos.Y >= y_min
                                                                    && mouse_pos.X >= x && mouse_pos.X <= x_max) // check if mouse is within menu options boundaries
                                                                {
                                                                    int selected_index = mouse_pos.Y - y_min;
                                                                    if (mouse_pos.X <= used_menu->options[selected_index]->boundaries.X)
                                                                        {
                                                                            used_menu->selected_index = selected_index;
                                                                            mouse_option_selected = something_is_selected = is_native_key = TRUE;

                                                                            if (last_selected_index != selected_index)
                                                                                {
                                                                                    used_menu->need_redraw = TRUE;
                                                                                    last_selected_index = selected_index;
                                                                                }
                                                                        }
                                                                }
                                                        }

                                                    // if nothing was selected but was selected before then resetting this "before"
                                                    if (mouse_option_selected ^ 1 && something_is_selected)
                                                        {
                                                            something_is_selected = FALSE;
                                                            used_menu->need_redraw = TRUE;
                                                            used_menu->selected_index = DISABLED;
                                                            last_selected_index = DISABLED;
                                                        }

                                                    if (something_is_selected)
                                                        {
                                                            if (inputRecords[event].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED && holding ^ 1)
                                                                {
                                                                    holding = 1;
                                                                    goto input_handler; // handling the input
                                                                }
                                                            else if (inputRecords[event].Event.MouseEvent.dwButtonState == 0) holding = 0;
                                                        }
                                                }
                                            else goto wait_start;
                                            break;
                                        case WINDOW_BUFFER_SIZE_EVENT:
                                            current_size = inputRecords[event].Event.WindowBufferSizeEvent.dwSize;
                                            break;
                                    }
                        }
                    FlushConsoleInputBuffer(hStdin);
                }
        }

end_render_loop:; // anchor

    // cleanup
    FlushConsoleInputBuffer(hStdin);
    if (menus_amount == 0)
        {
            SetConsoleMode(hStdin, old_mode);
            _setConsoleActiveScreenBuffer(hConsole);
        }
    else
        _setConsoleActiveScreenBuffer(_find_first_active_menu_buffer());
    // free(used_menu); no need for now
}

/* GETTERS */
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