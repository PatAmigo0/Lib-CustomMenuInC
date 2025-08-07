#include "menu.h"

/* ============== DEFINES AND MACROS ============== */
// #define DEBUG

#ifdef DEBUG
#include <psapi.h>
#endif

#define DISABLED -1
#define BUFFER_CAPACITY 128
#define UPDATE_FREQUENCE 2147483647 // ms
#define ERROR_UPDATE_FREQUENCE 20 // ms
#define EVENT_MAX_RECORDS 4
#define OFFSET_VALUE 2

#define DEFAULT_HEADER_TEXT "MENU"
#define DEFAULT_FOOTER_TEXT "Use arrows to navigate, Enter to select"
#define DEFAULT_MENU_TEXT "Unnamed Option"

#define LOG_FILE_NAME "menu_log.txt"
#define RGB_COLOR_SEQUENCE "\x1b[%d;2;%d;%d;%dm"
#define RGB_COLOR_DOUBLE_SEQUENCE "\x1b[38;2;%hd;%hd;%hdm\x1b[48;2;%hd;%hd;%hdm"
#define ERROR_MESSAGE1 "\033[31mError: Console window size is too small!\n""Required size: %d x %d\n"
#define ERROR_MESSAGE2 "Current size: %d x %d\nMake window bigger.\033[0m"

// error codes
#define BAD_CALLOC 138
#define BAD_REALLOC 134
#define BAD_MENU 253
#define BAD_HANDLE 258

/* ============== GLOBAL VARIABLES ============== */
static COORD zero_point = {0, 0};
static DWORD written = 0;
static COORD cached_size = {0, 0};
static HANDLE hConsole, hConsoleError, hCurrent, _hError, hStdin;
static CONSOLE_SCREEN_BUFFER_INFO hBack_csbi;

static BYTE menu_settings_initialized = FALSE, menu_color_initialized = FALSE;
static MENU_SETTINGS MENU_DEFAULT_SETTINGS;
static MENU_COLOR MENU_DEFAULT_COLOR;

static INPUT input = {0};
static BYTE holding = FALSE;

static MENU* menus_array = NULL;
static int menus_amount = 0;

/* ============== FORWARD DECLARATIONS ============== */
#ifdef DEBUG
static void _print_memory_info(HANDLE hBuffer);
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
static void _vwrite_string(HANDLE hDestination, const char* restrict text, va_list args);
static void _lwrite_string(HANDLE hDestination, const char* restrict text);

static void _clamp_center_coord(COORD* coord);
static HANDLE _find_first_active_menu_buffer();

static void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate);
static void _getMenuSize(MENU menu);
static BYTE _size_check(MENU menu);
static void _initWindow(SMALL_RECT* restrict window, COORD size);
static void _clear_buffer(HANDLE hBuffer);
static void _reset_mouse_state();
static int _check_menu(unsigned long long saved_id);
static void _block_input(DWORD* oldMode);

static void _renderMenu(const MENU used_menu);
static void _show_error_and_wait_extended(MENU menu);
static COORD _calculate_start_coordinates(MENU menu, COORD current_size);

/* ============== PUBLIC FUNCTION IMPLEMENTATIONS ============== */

/* ----- timing Functions ----- */
double tick()
{
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    static LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
}

/* ----- Menu Policy Functions ----- */
void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy)
{
    menu_to_change->_menu_settings.header_enabled = (new_header_policy && 1) || 0;
    menu_to_change->_menu_settings.footer_enabled = (new_footer_policy && 1) || 0;
}

void toggle_mouse(MENU menu_to_change)
{
    menu_to_change->_menu_settings.mouse_enabled = menu_to_change->_menu_settings.mouse_enabled  ^ 1;
}

/* ----- Configuration Functions ----- */
void set_menu_settings(MENU menu, MENU_SETTINGS new_settings)
{
    memcpy((void*)&(menu->_menu_settings), (void*)&new_settings, sizeof(MENU_SETTINGS));
    _clamp_center_coord(&(menu->_menu_settings.menu_center));
}

void set_default_menu_settings(MENU_SETTINGS new_settings)
{
    if (menu_settings_initialized ^ TRUE) menu_settings_initialized = TRUE;
    memcpy((void*)&MENU_DEFAULT_SETTINGS, (void*)&new_settings, sizeof(MENU_SETTINGS));
}

void set_color_object(MENU menu, MENU_COLOR color_object)
{
    memcpy((void*)&(menu->_color_object), (void*)&color_object, sizeof(MENU_COLOR));
}

void set_default_color_object(MENU_COLOR color_object)
{
    if (menu_color_initialized ^ TRUE) menu_color_initialized = TRUE;
    memcpy((void*)&MENU_DEFAULT_COLOR, (void*)&color_object, sizeof(MENU_COLOR));
}

/* ----- Menu Creation Functions ----- */
MENU create_menu()
{
    static int _initialized = FALSE;
    if (!_initialized)
        {
            _init_menu_system();
            _initialized = TRUE;
        }

    MENU new_menu = (MENU)_safe_malloc(sizeof(struct __menu));
    memset(new_menu, FALSE, sizeof(struct __menu));

    new_menu->count = 0;
    new_menu->active_buffer = 0;
    new_menu->selected_index = 0;
    new_menu->options = NULL;
    new_menu->running = FALSE;
    new_menu->__ID = _random_uint64_t();
    new_menu->_menu_settings = create_new_settings();
    new_menu->_color_object = create_color_object();
    new_menu->hBuffer[0] = _createConsoleScreenBuffer();
    new_menu->hBuffer[1] = _createConsoleScreenBuffer();
    new_menu->menu_size = zero_point;
    new_menu->header = DEFAULT_HEADER_TEXT;
    new_menu->footer = DEFAULT_FOOTER_TEXT;

    _getMenuSize(new_menu);

    menus_array = (MENU*)_safe_realloc(menus_array, (menus_amount + 1) * sizeof(MENU));
    menus_array[menus_amount++] = new_menu;

    return new_menu;
}

MENU_ITEM create_menu_item(const char* restrict text, __menu_callback callback, void* callback_data)
{
    MENU_ITEM item = (MENU_ITEM)_safe_malloc(sizeof(struct __menu_item));
    item->text = text ? text : DEFAULT_MENU_TEXT;
    item->text_len = strlen(item->text);
    item->boundaries = (COORD)
    {
        item->text_len - 1, 0
    };
    item->callback = callback; //
    item->data_chunk = callback_data;
    return item;
}

MENU_SETTINGS create_new_settings()
{
    if (!menu_settings_initialized) _init_menu_system();
    MENU_SETTINGS new_settings;
    memcpy(&new_settings, &MENU_DEFAULT_SETTINGS, sizeof(MENU_SETTINGS));
    return new_settings;
}

MENU_COLOR create_color_object()
{
    if (!menu_color_initialized) _init_menu_system();
    MENU_COLOR new_color_object;
    memcpy(&new_color_object, &MENU_DEFAULT_COLOR, sizeof(MENU_COLOR));
    return new_color_object;
}

/* ----- Color Functions ----- */
MENU_RGB_COLOR rgb(short r, short g, short b)
{
    return (MENU_RGB_COLOR)
    {
        r, g, b
    };
}

void new_rgb_color(int text_color, MENU_RGB_COLOR color, char output[MAX_RGB_LEN])
{
    sprintf(output, RGB_COLOR_SEQUENCE, text_color ? 38 : 48, color.r, color.g, color.b);
}

void new_full_rgb_color(MENU_RGB_COLOR _color_foreground, MENU_RGB_COLOR _color_background, char output[MAX_RGB_LEN])
{
    sprintf(output, RGB_COLOR_DOUBLE_SEQUENCE,
            _color_foreground.r, _color_foreground.g, _color_foreground.b,
            _color_background.r, _color_background.g, _color_background.b);
}

/* ----- Menu Operations ----- */
void add_option(MENU used_menu, const MENU_ITEM item)
{
    size_t new_size = used_menu->count + 1;
    MENU_ITEM* new_options = (MENU_ITEM*)_safe_realloc(used_menu->options, new_size * sizeof(MENU_ITEM));

    used_menu->options = new_options;
    used_menu->options[used_menu->count] = item;
    used_menu->count = new_size;
    _getMenuSize(used_menu);
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
            _lwrite_string(hConsoleError, "Error: Menu has no options. Use add_option first!");
            exit(BAD_MENU);
        }

    if (_size_check(used_menu))
        {
            _show_error_and_wait_extended(used_menu);
        }

    used_menu->running = 1;
    used_menu->selected_index = 0;
    _setConsoleActiveScreenBuffer(used_menu->hBuffer[used_menu->active_buffer]);
    _renderMenu(used_menu);
}

/* ----- Cleanup Functions ----- */
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
    while(menus_amount > 0)
        clear_menu(menus_array[0]);
}

void clear_menus_and_exit()
{
    clear_menus();
    exit(0);
}

/* ============== INTERNAL FUNCTIONS ============== */

/* ----- Memory Management ----- */
#ifdef DEBUG
static void _print_memory_info(HANDLE hBuffer)
{
    static size_t _s_t = 1024 * 1024;
    PROCESS_MEMORY_COUNTERS pmc;

    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            DWORD handleCount = 0;
            GetProcessHandleCount(GetCurrentProcess(), &handleCount);

            _draw_at_position(hBuffer, 0, 16, "PageFaultCount: %lu", pmc.PageFaultCount);
            _draw_at_position(hBuffer, 0, 18, "WorkingSetSize: %.5lf MB", (double)pmc.WorkingSetSize / _s_t);
            _draw_at_position(hBuffer, 0, 20, "PeakWorkingSetSize: %.5lf MB", (double)pmc.PeakWorkingSetSize / _s_t);
            _draw_at_position(hBuffer, 0, 22, "Pagefile Usage: %.5lf MB", (double)pmc.PagefileUsage / _s_t);
            _draw_at_position(hBuffer, 0, 24, "PeakPagefile: %.5lf MB", (double)pmc.PeakPagefileUsage / _s_t);
            _draw_at_position(hBuffer, 0, 26, "PagedPool: %.5lf MB", (double)pmc.QuotaPagedPoolUsage / _s_t);
            _draw_at_position(hBuffer, 0, 28, "NonPagedPool: %.5lf MB", (double)pmc.QuotaNonPagedPoolUsage / _s_t);
            _draw_at_position(hBuffer, 0, 30, "Handles: %lu", handleCount);
        }
}
#endif

static void* _safe_malloc(size_t _size)
{
    void* new_ptr = calloc(1, _size);
    if (!new_ptr && _size > 0)
        {
            _lwrite_string(hConsoleError, "Fatal: Memory allocation failed\n");
            exit(BAD_CALLOC);
        }
    return new_ptr;
}

static void* _safe_realloc(void* ptr, size_t _size)
{
    if (_size == 0)
        {
            free(ptr);
            return NULL;
        }

    void* new_ptr = realloc(ptr, _size);
    if (!new_ptr && _size > 0)
        {
            _lwrite_string(hConsoleError, "Fatal: Memory reallocation failed\n");
            exit(BAD_REALLOC);
        }
    return new_ptr;
}

/* ----- Initialization ----- */
static MENU_COLOR _create_default_color()
{
    return (MENU_COLOR)
    {
        DARK_BLUE_BG_WHITE_TEXT, CYAN_BG_BLACK_TEXT
    };
}

static MENU_SETTINGS _create_default_settings()
{
    MENU_SETTINGS settings;
    memset(&settings, FALSE, sizeof(MENU_SETTINGS));
    settings.mouse_enabled = DEFAULT_MOUSE_SETTING;
    settings.header_enabled = DEFAULT_HEADER_SETTING;
    settings.footer_enabled = DEFAULT_FOOTER_SETTING;
    settings.double_width_enabled = DEFAULT_WIDTH_SETTING;
    settings.menu_center = (MENU_COORD)
    {
        0, 0
    };
    settings.__garbage_collector = TRUE;
    return settings;
}

static HANDLE _createConsoleScreenBuffer()
{
    HANDLE hBuffer = CreateConsoleScreenBuffer(
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         CONSOLE_TEXTMODE_BUFFER,
                         NULL
                     );
    if (hBuffer == INVALID_HANDLE_VALUE)
        {
            _draw_at_position(hConsoleError, 0, 0, "Fatal: error creating console buffer: %lu\n", GetLastError());
            exit(BAD_HANDLE);
        }
    return hBuffer;
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

    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

    CONSOLE_SCREEN_BUFFER_INFO icsbi;
    GetConsoleScreenBufferInfo(hConsole, &icsbi);

    cached_size = (COORD)
    {
        icsbi.srWindow.Right - icsbi.srWindow.Left + 1,
                             icsbi.srWindow.Bottom - icsbi.srWindow.Top + 1
    };

    if (!menu_settings_initialized)
        set_default_menu_settings(_create_default_settings());
    if (!menu_color_initialized)
        set_default_color_object(_create_default_color());

    srand(time(NULL));
}

static void _init_hError()
{
    _hError = _createConsoleScreenBuffer();
}

/* ----- Rendering Utilities ----- */
static void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text)
{
    SetConsoleCursorPosition(hDestination, (COORD)
    {
        x, y
    });
    _lwrite_string(hDestination, text);
}

static void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* text, ...)
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

static void _vwrite_string(HANDLE hDestination, const char* text, va_list args)
{
    char buffer[BUFFER_CAPACITY];
    vsnprintf(buffer, BUFFER_CAPACITY, text, args);
    _lwrite_string(hDestination, buffer);
}

static void _lwrite_string(HANDLE hDestination, const char* text)
{
    WriteConsoleA(hDestination, text, (DWORD)strlen(text), &written, NULL);
}

/* ---- Other Utilities ---- */
static void _clamp_center_coord(COORD* coord)
{
	// x
	if (coord->X > 1) coord->X = 1;
	else if (coord->X < -1) coord->X = -1;
	
	// y
	if (coord->Y > 1) coord->Y = 1;
	else if (coord->Y < -1) coord->Y = -1; 
}

/* ----- Console Management ----- */
static void _setConsoleActiveScreenBuffer(HANDLE hBuffer)
{
    SetConsoleActiveScreenBuffer(hBuffer);
    hCurrent = hBuffer;
}

static void _getMenuSize(MENU menu)
{
    int max_width = 1;
    for (int i = 0; i < menu->count; i++)
        {
            int current_width = strlen(menu->options[i]->text);
            if (current_width > max_width) max_width = current_width;
        }

    menu->menu_size.X = (max_width + 4) * (menu->_menu_settings.double_width_enabled ? 2 : 1);
    menu->menu_size.Y = menu->count * 2 + 6;
}

static BYTE _size_check(MENU menu)
{
    GetConsoleScreenBufferInfo(hConsole, &hBack_csbi);
    int screen_width = hBack_csbi.srWindow.Right - hBack_csbi.srWindow.Left + 1;
    int screen_height = hBack_csbi.srWindow.Bottom - hBack_csbi.srWindow.Top + 1;

    cached_size = (COORD)
    {
        screen_width, screen_height
    };
    return (screen_width < menu->menu_size.X) | (screen_height < menu->menu_size.Y);
}

static void _initWindow(SMALL_RECT* window, COORD size)
{
    window->Top = window->Left = 0;
    window->Right = size.X - 1;
    window->Bottom = size.Y - 1;
}

static HANDLE _find_first_active_menu_buffer()
{
    for (int i = menus_amount - 1; i >= 0; i--)
        {
            if (menus_array[i]->running)
                return menus_array[i]->hBuffer[menus_array[i]->active_buffer];
        }
    exit(BAD_MENU);
}

static void _clear_buffer(HANDLE hBuffer)
{
    _lwrite_string(hBuffer, CLEAR_SCREEN);
    _lwrite_string(hBuffer, CLEAR_SCROLL_BUFFER);
    _lwrite_string(hBuffer, RESET_MOUSE_POSITION);
}

/* ----- Input Handling ----- */
static void _block_input(DWORD* oldMode)
{
    GetConsoleMode(hStdin, oldMode);
    DWORD newMode = *oldMode;
    newMode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    newMode |= (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleMode(hStdin, newMode);
}

static void _reset_mouse_state()
{
    SendInput(1, &input, sizeof(INPUT));
}

static int _check_menu(unsigned long long saved_id)
{
    for (int i = 0; i < menus_amount; i++)
        if (menus_array[i] &&
                menus_array[i]->__ID == saved_id &&
                menus_array[i]->running) return TRUE;
    return FALSE;
}

/* ----- Error Handling ----- */
static void _show_error_and_wait_extended(MENU menu)
{
    menu->need_redraw = TRUE;

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

    // error message intialization
    char error_message[BUFFER_CAPACITY];
    sprintf(error_message, ERROR_MESSAGE1, menu_size.X, menu_size.Y);
    strcat(error_message, ERROR_MESSAGE2);

    // pre-start calls
    _block_input(&oldMode);
    _draw_at_position(_hError, 0, 0, error_message, current_size.X, current_size.Y);
    _setConsoleActiveScreenBuffer(_hError);
    FlushConsoleInputBuffer(hStdin);
    SetConsoleScreenBufferSize(_hError, menu_size);

    running = TRUE;
    while (running)
        {
        error_wait_start:
            ;
            objectWait = WaitForSingleObject(hStdin, UPDATE_FREQUENCE);
            if (objectWait == WAIT_OBJECT_0)
                {
                    GetNumberOfConsoleInputEvents(hStdin, &numEvents);
                    ReadConsoleInput(hStdin, inputRecords, min(EVENT_MAX_RECORDS, numEvents), &numEvents);
                    event_running = TRUE;
                    for (k = 0; k < numEvents && event_running; k++)
                        switch(inputRecords[k].EventType)
                            {
                                case WINDOW_BUFFER_SIZE_EVENT:
                                    current_size = inputRecords[k].Event.WindowBufferSizeEvent.dwSize;
                                    event_running = FALSE;
                                    break;
                                case MOUSE_EVENT:
                                case KEY_EVENT:
                                    goto error_wait_start;
                            }
                    if (current_size.X >= menu_size.X && current_size.Y >= menu_size.Y) running = FALSE;
                    else
                        {
                            _clear_buffer(_hError);
                            _draw_at_position(_hError, 0, 0, error_message, current_size.X, current_size.Y);
                        }
                }
        }
    _clear_buffer(_hError);
    FlushConsoleInputBuffer(hStdin);
    _reset_mouse_state();
    _setConsoleActiveScreenBuffer(menu->hBuffer[menu->active_buffer]);
    cached_size = current_size;
}

/* ----- Main Rendering Loop ----- */
static COORD _calculate_start_coordinates(MENU menu, COORD current_size)
{
    MENU_COORD normcoord = menu->_menu_settings.menu_center;
    COORD menu_size = menu->menu_size;
    COORD newcoord;

    // main calculations
    newcoord.X = (float)(normcoord.X + 1.0f) * (float)(current_size.X - 1.0f) / 2.0f;
    newcoord.Y = (float)(1.0f - normcoord.Y) * (float)(current_size.Y - 1.0f) / 2.0f;
    
    newcoord.X -= menu_size.X / 2;
    newcoord.Y -= menu_size.Y / 2 - OFFSET_VALUE;
    
    // clamping time
    if (newcoord.X + menu_size.X > current_size.X) newcoord.X -= menu_size.X / 2;
	if (newcoord.Y + menu_size.Y > current_size.Y) newcoord.Y -= OFFSET_VALUE;
    
    
    if (newcoord.X < 0) newcoord.X = 0;
    if (newcoord.Y < 0) newcoord.Y = 0;
    
    return newcoord;
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

    last_selected_index = DISABLED;

    menu_size = used_menu->menu_size;
    saved_id = used_menu->__ID;
    mouse_input_enabled = used_menu->_menu_settings.mouse_enabled;
    used_menu->need_redraw = TRUE;

    selected_index = mouse_input_enabled ? DISABLED : 0;
    used_menu->selected_index = selected_index;

    spaces = (menu_size.X - 6) / 2;

    snprintf(header, sizeof(header), "%s%*s%s%*s" RESET_ALL_STYLES, used_menu->_color_object.headerColor,
             spaces, "", used_menu->header, spaces, "");
    snprintf(footer, sizeof(footer), "%s %-*s " RESET_ALL_STYLES, used_menu->_color_object.footerColor,
             menu_size.X - 4, used_menu->footer);

    // debug values
#ifdef DEBUG
    tick_count = 0;
    debug_mouse_pos = (COORD)
    {
        0, 0
    };
    mouse_status = FALSE;
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
                    start = _calculate_start_coordinates(used_menu, current_size);
#ifdef DEBUG
                    _draw_at_position(hBackBuffer, 0, 0, "__ID: %llu", used_menu->__ID);
                    _draw_at_position(hBackBuffer, 0, 2, "SELECTED: %d", something_is_selected);
                    _draw_at_position(hBackBuffer, 0, 4, "tick: %llu", tick_count);
                    _draw_at_position(hBackBuffer, 0, 6, "WINDOW SIZE: %d %d ", current_size.X, current_size.Y);
                    _draw_at_position(hBackBuffer, 0, 8, "need redraw: %d", used_menu->need_redraw);
                    _draw_at_position(hBackBuffer, 0, 10, "MOUSE POS: %d %d   ", debug_mouse_pos.X, debug_mouse_pos.Y);
                    _draw_at_position(hBackBuffer, 0, 12, "mouse status: %d", mouse_status);
                    _draw_at_position(hBackBuffer, 0, 14, "menus_amount: %d", menus_amount);
                    _print_memory_info(hBackBuffer); // -> 30
                    _draw_at_position(hBackBuffer, 0, 32, "menus start coord: %d %d", start.X, start.Y);
#endif

                    // header
                    if (used_menu->_menu_settings.header_enabled)
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
                    if (used_menu->_menu_settings.footer_enabled)
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
                            BYTE is_native_key = FALSE;
                            BYTE mouse_option_selected = FALSE;
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

                                                                                if (_check_menu(saved_id)) // if exists after the callback (may be deleted)
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
                                                    if (mouse_option_selected ^ TRUE)
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
                                                    if (mouse_option_selected ^ TRUE && something_is_selected)
                                                        {
                                                            something_is_selected = FALSE;
                                                            used_menu->need_redraw = TRUE;
                                                            used_menu->selected_index = DISABLED;
                                                            last_selected_index = DISABLED;
                                                        }
                                                    if (inputRecords[event].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED && holding ^ 1)
                                                        {
                                                            holding = TRUE;
                                                            if (something_is_selected) goto input_handler; // handling the input
                                                        }
                                                    else if (inputRecords[event].Event.MouseEvent.dwButtonState == 0) holding = FALSE;
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
}