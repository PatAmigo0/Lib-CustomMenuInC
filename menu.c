#include "menu.h"

// #define DEBUG

#define DISABLED -1
#define BUFFER_CAPACITY 128
#define UPDATE_FREQUENCE 2147483647 // ms
#define ERROR_UPDATE_FREQUENCE 20 // ms
#define EVENT_MAX_RECORDS 4

#define FIX_VALUE 2

#define DEFAULT_HEADER_TEXT "MENU"
#define DEFAULT_FOOTER_TEXT "Use arrows to navigate, Enter to select"
#define DEFAULT_MENU_TEXT "Unnamed Option"

#define LOG_FILE_NAME "menu_log.txt"
#define RGB_COLOR_SEQUENCE "\x1b[%d;2;%d;%d;%dm"
#define RGB_COLOR_DOUBLE_SEQUENCE "\x1b[38;2;%hd;%hd;%hdm\x1b[48;2;%hd;%hd;%hdm"

// error codes defines
#define BAD_CALLOC 138
#define BAD_REALLOC 134
#define BAD_MENU 253

// cached values
static COORD zero_point = {0, 0};
static DWORD written = 0;
static COORD cached_size = {0, 0}; // saved screen size after the last _size_check function call or after the last _show_error_message_and_wait_extended call
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
    __menu_callback callback;
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
#ifdef DEBUG
void _print_memory_info(HANDLE hBuffer);
#endif
static void* _safe_malloc(size_t _size);
static void* _safe_realloc(void* _mem_to_realloc, size_t _size);
static MENU_SETTINGS _create_default_settings();
static MENU_COLOR _create_default_color();
static unsigned long long _random_uint64_t();
static void _init_menu_system();
static void _init_hError();
static HANDLE _createConsoleScreenBuffer(void);
static void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text, ...);
static void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text);
// static void _write_string(HANDLE hDestination, const char* restrict text, ...);
static void _vwrite_string(HANDLE hDestination, const char* restrict text, va_list args);
static void _lwrite_string(HANDLE hDestination, const char* restrict text);
static void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate);
static void _getMenuSize(MENU menu);
static BYTE _size_check(MENU menu);
static void _initWindow(SMALL_RECT* restrict window, COORD size);
static void _clear_buffer(HANDLE hBuffer);
static void _reset_mouse_state();
static int _check_menu(unsigned long long saved_id);
static void _block_input(DWORD* oldMode);
static void _renderMenu(const MENU used_menu);
// static void _show_error_and_wait(const char* restrict message, ...);
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
    if (menu_settings_initialized ^ 1) menu_settings_initialized = 1;
    memcpy((void*)&MENU_DEFAULT_SETTINGS, (void*)&new_settings, sizeof(MENU_SETTINGS));
}

void set_color_object(MENU menu, MENU_COLOR color_object)
{
    memcpy((void*)&(menu->color_object), (void*)&color_object, sizeof(MENU_COLOR));
}

void set_default_color_object(MENU_COLOR color_object)
{
    if (menu_color_initialized ^ 1) menu_color_initialized = 1;
    memcpy((void*)&MENU_DEFAULT_COLOR, (void*)&color_object, sizeof(MENU_COLOR));
}

MENU create_menu()
{
    static int _initialized = 0;
    if (_initialized ^ 1)
        {
            _init_menu_system();
            _initialized = 1;
        }
    MENU new_menu = (MENU)_safe_malloc(sizeof(struct __menu));

    memset(new_menu, 0, sizeof(struct __menu));
    new_menu->count = 0;
    new_menu->mouse_enabled = MENU_DEFAULT_SETTINGS.mouse_enabled;
    new_menu->options = NULL;
    new_menu->running = 0;
    new_menu->active_buffer = 0;
    new_menu->selected_index = 0;
    new_menu->footer_policy = MENU_DEFAULT_SETTINGS.footer_enabled;
    new_menu->header_policy = MENU_DEFAULT_SETTINGS.header_enabled;
    new_menu->width_policy = MENU_DEFAULT_SETTINGS.double_width_enabled;
    new_menu->__ID = _random_uint64_t();

    new_menu->color_object = create_color_object();

    new_menu->hBuffer[0] = _createConsoleScreenBuffer();
    new_menu->hBuffer[1] = _createConsoleScreenBuffer();

    new_menu->menu_size = zero_point;

    new_menu->header = DEFAULT_HEADER_TEXT;
    new_menu->footer = DEFAULT_FOOTER_TEXT;

    _getMenuSize(new_menu);

    menus_array = (MENU*)_safe_realloc((void*)menus_array, (menus_amount + 1) * sizeof(MENU));
    menus_array[menus_amount++] = new_menu;

    return new_menu;
}

MENU_ITEM create_menu_item(const char* restrict text, __menu_callback callback, void* callback_data)
{
    MENU_ITEM item = (MENU_ITEM)_safe_malloc(sizeof(struct __menu_item));
    if (!item || item == NULL) exit(BAD_REALLOC);
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
    if (menu_settings_initialized ^ 1) _init_menu_system();

    MENU_SETTINGS new_settings;
    memcpy((void*)&new_settings, (void*)&MENU_DEFAULT_SETTINGS, sizeof(MENU_SETTINGS));

    return new_settings;
}

MENU_COLOR create_color_object()
{
    if (menu_color_initialized ^ 1) _init_menu_system();

    MENU_COLOR new_color_object;
    memcpy((void*)&new_color_object, (void*)&MENU_DEFAULT_COLOR, sizeof(MENU_COLOR));

    return new_color_object;
}

MENU_RGB_COLOR rgb(short r, short g, short b)
{
    return (MENU_RGB_COLOR)
    {
        r, g, b
    };
}

void new_rgb_color(int text_color, MENU_RGB_COLOR color, char output[MAX_RGB_LEN])
{
    sprintf(output, RGB_COLOR_SEQUENCE, (text_color ? 38 : 48), color.r, color.g, color.b);
}

void new_full_rgb_color(MENU_RGB_COLOR _color_foreground, MENU_RGB_COLOR _color_background, char output[MAX_RGB_LEN])
{
    sprintf(output, RGB_COLOR_DOUBLE_SEQUENCE, _color_foreground.r, _color_foreground.g, _color_foreground.b, _color_background.r, _color_background.g, _color_background.b);
}

int add_option(MENU used_menu, const MENU_ITEM restrict item)
{
    size_t new_size = used_menu->count + 1;
    MENU_ITEM* new_options = (MENU_ITEM*)_safe_realloc((void*)used_menu->options, new_size * sizeof(MENU_ITEM));

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

    if (_size_check(used_menu))
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
                used_menu->count--;

                size_t elements_to_move = used_menu->count - i;
                if (elements_to_move > 0) memmove(&o[i], &o[i+1], elements_to_move * sizeof(MENU_ITEM));

                if (used_menu->selected_index >= used_menu->count) used_menu->selected_index--;
                _getMenuSize(used_menu);

                if (used_menu->count <= 0) clear_menu(used_menu);
                else used_menu->options = (MENU_ITEM*)_safe_realloc((void*)used_menu->options, used_menu->count * sizeof(MENU_ITEM));
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

                // free(m->color_object);

                if (m->hBuffer[0] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[0]);
                if (m->hBuffer[1] != INVALID_HANDLE_VALUE) CloseHandle(m->hBuffer[1]);
                m->running = FALSE;

                menus_array[i] = NULL;
                menus_amount--;

                size_t elements_to_move = menus_amount - i;
                if (elements_to_move > 0) memmove(&menus_array[i], &menus_array[i+1], elements_to_move * sizeof(MENU));

                if (menus_amount <= 0)
                    {
                        free(menus_array);
                        menus_array = NULL;
                        _setConsoleActiveScreenBuffer(hConsole);
                    }
                else
                    {
                        menus_array = (MENU*)_safe_realloc((void*)menus_array, menus_amount * sizeof(MENU));
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

// memory functions
#ifdef DEBUG
void _print_memory_info(HANDLE hBuffer)
{
    static size_t _s_t = 1024 * 1024;
    PROCESS_MEMORY_COUNTERS pmc;

    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            _draw_at_position(hBuffer, 0, 16, "PageFaultCount: %lu", pmc.PageFaultCount);
            _draw_at_position(hBuffer, 0, 18, "WorkingSetSize: %.5lf MB",
                              (double)pmc.WorkingSetSize / _s_t);
            _draw_at_position(hBuffer, 0, 20, "PeakWorkingSetSize: %.5lf MB",
                              (double)pmc.PeakWorkingSetSize / _s_t);
            _draw_at_position(hBuffer, 0, 22, "Pagefile Usage: %.5lf MB",
                              (double)pmc.PagefileUsage / _s_t);
            _draw_at_position(hBuffer, 0, 24, "PeakPagefile: %.5lf MB",
                              (double)pmc.PeakPagefileUsage / _s_t);
            _draw_at_position(hBuffer, 0, 26, "PagedPool: %.5lf MB",
                              (double)pmc.QuotaPagedPoolUsage / _s_t);
            _draw_at_position(hBuffer, 0, 28, "NonPagedPool: %.5lf MB",
                              (double)pmc.QuotaNonPagedPoolUsage / _s_t);
        }

    static DWORD handleCount = 0;
    if (GetProcessHandleCount(GetCurrentProcess(), &handleCount))
        _draw_at_position(hBuffer, 0, 30, "Handles: %lu", handleCount);
}
#endif

static void* _safe_malloc(size_t _size)
{
    void* _new_block_chunk = calloc(1, _size);
    if (!_new_block_chunk || _new_block_chunk == NULL) exit(BAD_CALLOC);
    return _new_block_chunk;
}

static void* _safe_realloc(void* _mem_to_realloc, size_t _size)
{
    void* _new_block_chunk = realloc(_mem_to_realloc, _size);
    if (!_new_block_chunk || _new_block_chunk == NULL) exit(BAD_REALLOC);
    return _new_block_chunk;
}

// utility
static MENU_COLOR _create_default_color()
{
    MENU_COLOR new_color_object = {DARK_BLUE_BG_WHITE_TEXT, CYAN_BG_BLACK_TEXT};
    return new_color_object;
}

static MENU_SETTINGS _create_default_settings()
{
    MENU_SETTINGS new_settings;
    memset(&new_settings, 0, sizeof(struct __menu_settings));
    new_settings.mouse_enabled = DEFAULT_MOUSE_SETTING;
    new_settings.header_enabled = DEFAULT_HEADER_SETTING;
    new_settings.footer_enabled = DEFAULT_FOOTER_SETTING;
    new_settings.double_width_enabled = DEFAULT_WIDTH_SETTING;
    new_settings.__garbage_collector = TRUE;
    return new_settings;
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

static unsigned long long _random_uint64_t()
{
    return ((unsigned long long)rand() << 32) | rand();
}

// initializations
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

// write functions
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

/* static void _write_string(HANDLE hDestination, const char* restrict text, ...)
{
    va_list args;
    va_start(args, text);
    _vwrite_string(hDestination, text, args);
    va_end(args);
}
*/

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

// other
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

static BYTE _size_check(MENU menu)
{
    GetConsoleScreenBufferInfo(hConsole, &hBack_csbi);
    int screen_width = hBack_csbi.srWindow.Right - hBack_csbi.srWindow.Left + 1;
    int screen_height = hBack_csbi.srWindow.Bottom - hBack_csbi.srWindow.Top + 1;
    BYTE size_error = (screen_width < menu->menu_size.X) | (screen_height < menu->menu_size.Y);

    cached_size = (COORD) // caching
    {
        screen_width, screen_height
    };

    // cached_mouse_pos = hBack_csbi.dwCursorPosition;

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
    exit(BAD_MENU); // we should NEVER get there
}

static void _clear_buffer(HANDLE hBuffer)
{
    _lwrite_string(hBuffer, CLEAR_SCREEN); // clear visible screen
    _lwrite_string(hBuffer, CLEAR_SCROLL_BUFFER); // clear scrollback
    _lwrite_string(hBuffer, RESET_MOUSE_POSITION); // cursor reset
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

/* static void _show_error_and_wait(const char* restrict message, ...)
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
*/

static void _show_error_and_wait_extended(MENU menu)
{
    menu->need_redraw = 1;

    // size intitialization
    GetConsoleScreenBufferInfo(menu->hBuffer[menu->active_buffer], &hBack_csbi);
    SMALL_RECT cr_window = hBack_csbi.srWindow;
    COORD current_size = (COORD)
    {
        cr_window.Right - cr_window.Left + 1, cr_window.Bottom - cr_window.Top + 1
    }, menu_size = menu->menu_size;

    // function variables pre-define
    INPUT_RECORD inputRecords[EVENT_MAX_RECORDS];
    BYTE running, event_running;
    DWORD objectWait, k, numEvents, oldMode;

    // pre-start calls
    _block_input(&oldMode);
    _draw_at_position(_hError, 0, 0, error_message, menu_size.X, menu_size.Y, current_size.X, current_size.Y);
    _setConsoleActiveScreenBuffer(_hError);
    FlushConsoleInputBuffer(hStdin);
    SetConsoleScreenBufferSize(_hError, menu_size);

    running = 1;
    while (running)
        {
            event_running = 1;
        error_wait_start:
            ;
            objectWait = WaitForSingleObject(hStdin, UPDATE_FREQUENCE);
            if (objectWait == WAIT_OBJECT_0)
                {
                    GetNumberOfConsoleInputEvents(hStdin, &numEvents);
                    ReadConsoleInput(hStdin, inputRecords, min(EVENT_MAX_RECORDS, numEvents), &numEvents);
                    for (k = 0; k < numEvents && event_running; k++)
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
                    if (current_size.X >= menu_size.X && current_size.Y >= menu_size.Y)
                        running = 0;
                    else
                        {
                            _clear_buffer(_hError);
                            _draw_at_position(_hError, 0, 0, error_message, menu_size.X, menu_size.Y, current_size.X, current_size.Y);
                        }
                }
        }
    _clear_buffer(_hError);
    FlushConsoleInputBuffer(hStdin);
    _reset_mouse_state();
    _setConsoleActiveScreenBuffer(menu->hBuffer[menu->active_buffer]);
    cached_size = current_size;
}

static void _renderMenu(const MENU used_menu)
{
    if (!used_menu) return;

    // REGISTER VARIABLES BLOCK
    register COORD current_size, // current console window dimensions (X=width, Y=height)
             old_size,    // previous console window dimensions before resize
             start;       // top-left rendering position for the menu
    register int y_max,         // maximum Y coordinate of menu options (bottom boundary)
             y_min,         // minimum Y coordinate of menu options (top boundary)
             x_max;         // maximum X coordinate of menu options (right boundary)
    register BYTE size_check,   // flag: TRUE if console size is too small for menu display
             mouse_input_enabled; // flag: TRUE if mouse input is enabled for this menu

    // BASIC VARIABLES BLOCK
    int y,                      // current Y position for rendering menu items
        x,                      // current X position for rendering menu items
        i,                      // loop counter for menu options iteration
        last_selected_index,    // previously selected menu option index
        selected_index,			// a variable for storing the current index of the option pointed to by the mouse
        spaces;                 // number of spaces for centering header/footer text

    unsigned long long saved_id; // saved menu ID to verify menu validity after callbacks
    static BYTE something_is_selected; // static flag persisting between function calls (indicates if any option is visually selected)

    HANDLE hBackBuffer;         // handle to store the back buffer for double-buffered rendering
    CONSOLE_SCREEN_BUFFER_INFO csbi; // console screen buffer information structure
    SMALL_RECT new_window;      // new window dimensions after resize operations
    COORD menu_size,            // dimensions of the menu content (not the console window)
          mouse_pos;			// a variable to store the mouse position

    DWORD old_mode,             // previous console input mode (saved for restoration)
          numEvents,            // number of console input events to process
          event,                // loop counter for processing input events
          waitResult;			// WaitForSignalObject result event value
    WORD vk;                    // virtual key code for keyboard events
    INPUT_RECORD inputRecords[EVENT_MAX_RECORDS]; // an array to store console input events

    const char* format;         // format string for rendering menu options (with/without selection highlighting)
    char header[BUFFER_CAPACITY], // buffer for rendered header string
         footer[BUFFER_CAPACITY]; // buffer for rendered footer string

#ifdef DEBUG
    int mouse_status;           // current mouse button state
    size_t tick_count;          // counter for debug rendering iterations
    COORD debug_mouse_pos;      // current mouse position
#endif

    GetConsoleScreenBufferInfo(hCurrent, &csbi);
    old_size = current_size = (COORD)
    {
        csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1
    };

    last_selected_index = selected_index = -1;

    menu_size = used_menu->menu_size;
    saved_id = used_menu->__ID;
    mouse_input_enabled = used_menu->mouse_enabled;
    used_menu->need_redraw = 1;

    spaces = (menu_size.X - 6) / 2;

    snprintf(header, sizeof(header), "%s%*s%s%*s" RESET_ALL_STYLES, used_menu->color_object.headerColor,
             spaces, "", used_menu->header, spaces, "");
    snprintf(footer, sizeof(footer), "%s %-*s " RESET_ALL_STYLES, used_menu->color_object.footerColor,
             menu_size.X - 4, used_menu->footer);

    // pre-render checks
    if (mouse_input_enabled) used_menu->selected_index = DISABLED;

    // debug values
#ifdef DEBUG
    tick_count = 0;
    debug_mouse_pos = (COORD)
    {
        0, 0
    };
    mouse_status = 0;
#endif
    // pre-render calls
    _reset_mouse_state(); // resetting the mouse state because Windows is stupid
    _block_input(&old_mode);

    while (used_menu->running)
        {
#ifdef DEBUG
            _draw_at_position(hCurrent, 0, 4, "tick: %llu", ++tick_count);
            _draw_at_position(hCurrent, 0, 8, "need redraw: %d", used_menu->need_redraw);
            _draw_at_position(hCurrent, 0, 12, "mouse status: %d", mouse_status);
#endif
            if ((old_size.X != current_size.X) || (old_size.Y != current_size.Y))
                {
                    old_size = current_size;
                    size_check = (current_size.X < menu_size.X) || (current_size.Y < menu_size.Y);
                    _initWindow(&new_window, current_size);

                    if (size_check)
                        {
                            _show_error_and_wait_extended(used_menu);
                            current_size = cached_size;
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
                    _draw_at_position(hBackBuffer, 0, 0, "__ID: %llu", used_menu->__ID);
                    _draw_at_position(hBackBuffer, 0, 2, "SELECTED: %d", something_is_selected);
                    _draw_at_position(hBackBuffer, 0, 4, "tick: %llu", tick_count);
                    _draw_at_position(hBackBuffer, 0, 6, "WINDOW SIZE: %d %d ", current_size.X, current_size.Y);
                    _draw_at_position(hBackBuffer, 0, 8, "need redraw: %d", used_menu->need_redraw);
                    _draw_at_position(hBackBuffer, 0, 10, "MOUSE POS: %d %d   ", debug_mouse_pos.X, debug_mouse_pos.Y);
                    _draw_at_position(hBackBuffer, 0, 12, "mouse status: %d", mouse_status);
                    _draw_at_position(hBackBuffer, 0, 14, "menus_amount: %d", menus_amount);
                    _print_memory_info(hBackBuffer); // -> 22
#endif
                    start.X = (current_size.X - menu_size.X) / 2;
                    start.Y = (current_size.Y - menu_size.Y) / 2 + FIX_VALUE;

                    // header
                    if (used_menu->header_policy)
                        _ldraw_at_position(hBackBuffer, start.X, start.Y, header);
                    else start.Y -= 2;

                    // options
                    x = start.X + 2;
                    y = y_min = start.Y + 2;
                    x_max = 0;

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
            waitResult = WaitForSingleObject(hStdin, UPDATE_FREQUENCE);
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
                                                                        last_selected_index = used_menu->selected_index;
                                                                        break;
                                                                    case VK_DOWN:
                                                                        used_menu->selected_index =
                                                                            (used_menu->selected_index + 1) % used_menu->count;
                                                                        last_selected_index = used_menu->selected_index;
                                                                        break;
                                                                    case VK_RETURN: // ENTER
                                                                        if (used_menu->selected_index >= 0 && used_menu->options[used_menu->selected_index]->callback && used_menu->options[used_menu->selected_index]->callback != NULL)
                                                                            {
                                                                            input_handler:
                                                                                ;
                                                                                SetConsoleMode(hStdin, old_mode);
                                                                                SetConsoleScreenBufferSize(hConsole, current_size);
                                                                                _initWindow(&new_window, current_size);
                                                                                SetConsoleWindowInfo(hConsole, TRUE, &new_window);
                                                                                fflush(stdin);
                                                                                FlushConsoleInputBuffer(hStdin);

                                                                                _clear_buffer(hConsole);
                                                                                _setConsoleActiveScreenBuffer(hConsole);

                                                                                MENU_ITEM current_option = used_menu->options[used_menu->selected_index];
                                                                                current_option->callback(used_menu, current_option->data_chunk);

                                                                                if (_check_menu(saved_id))
                                                                                    {
                                                                                        if (_size_check(used_menu)) _show_error_and_wait_extended(used_menu);

                                                                                        current_size = cached_size;
                                                                                        _block_input(&old_mode);
                                                                                        _reset_mouse_state(); // resetting the mouse state because Windows is stupid (or me)
                                                                                        _setConsoleActiveScreenBuffer(hBackBuffer);
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
                                                            FlushConsoleInputBuffer(hStdin);
                                                        }
                                                }
                                            break;
                                        case MOUSE_EVENT:
#ifdef DEBUG
                                            debug_mouse_pos = inputRecords[event].Event.MouseEvent.dwMousePosition;
                                            mouse_status = inputRecords[event].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED;
                                            _draw_at_position(hCurrent, 0, 10, "MOUSE POS: %d %d   ", debug_mouse_pos.X, debug_mouse_pos.Y);
#endif
                                            if (mouse_input_enabled)
                                                {
                                                    if (mouse_option_selected ^ 1)
                                                        {
                                                            mouse_pos = inputRecords[event].Event.MouseEvent.dwMousePosition;
                                                            // checking boundaries
                                                            if (mouse_pos.Y < y_max && mouse_pos.Y >= y_min
                                                                    && mouse_pos.X >= x && mouse_pos.X <= x_max) // check if mouse is within the menu options boundaries
                                                                {
                                                                    selected_index = mouse_pos.Y - y_min;
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
                                                    if (inputRecords[event].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED && holding ^ 1)
                                                        {
                                                            holding = 1;
                                                            if (something_is_selected) goto input_handler; // handling the input
                                                        }
                                                    else if (inputRecords[event].Event.MouseEvent.dwButtonState == 0) holding = 0;
                                                }
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
    SetConsoleMode(hStdin, old_mode);
    if (menus_amount == 0)
        _setConsoleActiveScreenBuffer(hConsole);
    else _setConsoleActiveScreenBuffer(_find_first_active_menu_buffer());
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