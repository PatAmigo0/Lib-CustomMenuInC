/**
 * Made by Arseniy Kuskov
 * This file has no copyright assigned and is placed in the Public Domain.
 * No warranty is given.
 */

#include "menu.h"

/* ============== DEFINES AND MACROS ============== */
// #define DEBUG

#ifdef DEBUG
#include <psapi.h>
#endif

#define DISABLED -1
#define BUFFER_CAPACITY 256
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

// word types
#define HEADER_TYPE 0x1
#define FOOTER_TYPE 0x2
#define SELECTABLE_TYPE 0x3
#define ERROR_TYPE 0x4

/* */
enum RenderArgumentTag
{
    MENU_TYPE,
    HANDLE_TYPE
};

union RenderArgumentValue
{
    MENU menu;
    HANDLE handle;
};

typedef struct __menu_custom_render_argument
{
    enum RenderArgumentTag tag;
    union RenderArgumentValue value;
} MENU_RENDER_ARGUMENT;

/* ============== WRAPPER TYPES ============== */
typedef void (*ClearBufferFunc)(HANDLE);
typedef void (*RenderUnitDrawer)(MENU_RENDER_ARGUMENT, COORD, PMENU_RENDER_UNIT);

/* ============== GLOBAL VARIABLES ============== */
static COORD zero_point = {0, 0};
static DWORD written = 0;
static COORD cached_size = {0, 0};
static HANDLE hConsole, hConsoleError, hCurrent, _hError, hStdin;
static CONSOLE_SCREEN_BUFFER_INFO hBack_csbi;

static int menu_settings_initialized = FALSE,
           menu_color_initialized = FALSE,
           menu_legacy_color_initialized = FALSE;

// input defines
static INPUT input = {0};
static int holding = FALSE;

// settings constants
static MENU_SETTINGS MENU_DEFAULT_SETTINGS;
static MENU_COLOR MENU_DEFAULT_COLOR;
static LEGACY_MENU_COLOR MENU_LEGACY_DEFAULT_COLOR;

// menu values
static MENU* menus_array = NULL;
static int menus_amount = 0;

// other values
static WORD reset_color_attribute =
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

// global types struct
static MENU_RENDER_UNIT_TYPES mrut =
{
    HEADER_TYPE,
    FOOTER_TYPE, // Footer code,
    SELECTABLE_TYPE // Selectable code
};

// global var to store vt support value
static WORD vt100_support = 0;

// START OF STATIC WRAPPERS DEC
ClearBufferFunc _clear_buffer_func;

/* ============== FORWARD DECLARATIONS ============== */
#ifdef DEBUG
static void _print_memory_info(HANDLE hBuffer);
#endif

// NEW: Define the function pointer type for the mouse handler
typedef int (*MouseEventHandler)(MENU used_menu, const MOUSE_EVENT_RECORD* event,
                                 int y_min, int y_max, int x, int x_max,
                                 int* last_selected_index, int* something_is_selected, int* holding);

// NEW: Declare the two handler functions
static int _handle_mouse_event_enabled(MENU used_menu, const MOUSE_EVENT_RECORD* event,
                                       int y_min, int y_max, int x, int x_max,
                                       int* last_selected_index, int* something_is_selected, int* holding);
static int _handle_mouse_event_disabled(MENU used_menu, const MOUSE_EVENT_RECORD* event,
                                        int y_min, int y_max, int x, int x_max,
                                        int* last_selected_index, int* something_is_selected, int* holding);


static void* _safe_malloc(size_t _size);
static void* _safe_realloc(void* _mem_to_realloc, size_t _size);
static MENU_SETTINGS _create_default_settings();
static MENU_COLOR _create_default_color();
static MENU_RENDER_UNIT _create_render_unit(const char* text, DWORD unit_type, void* extra_data);
static MENU_RENDER_ARGUMENT _create_render_argument(enum RenderArgumentTag data_tag, void* value);
static unsigned long long _random_uint64_t();
static void _init_menu_system();
static void _init_hError();
static HANDLE _createConsoleScreenBuffer(void);

static void _draw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text, ...);
static void _ldraw_at_position(HANDLE hDestination, SHORT x, SHORT y, const char* restrict text);
static void _vwrite_string(HANDLE hDestination, const char* restrict text, va_list args);
static void _lwrite_string(HANDLE hDestination, const char* restrict text);

static size_t _count_utf8_chars(const char* s);
static void _clamp_center_coord(MENU_COORD* coord);
static WORD _check_if_supports_vt100();
static HANDLE _find_first_active_menu_buffer();

static void _setConsoleActiveScreenBuffer(HANDLE hBufferToActivate);
static void _get_menu_size(MENU menu);
static int _size_check(MENU menu);
static void _initWindow(SMALL_RECT* restrict window, COORD size);
static void _clear_buffer(HANDLE hBuffer);
static void _reset_mouse_state();
static int _check_menu(unsigned long long saved_id);
static void _block_input(DWORD* oldMode);

static void _draw_render_unit(MENU_RENDER_ARGUMENT rargument, COORD pos, PMENU_RENDER_UNIT render_unit);
static void _renderMenu(const MENU used_menu);
static void _show_error_and_wait_extended(MENU menu);
static COORD _calculate_start_coordinates(MENU menu, COORD current_size);
static void _update_formatted_strings(MENU menu);

// LEGACY FUNCTIONS
static void _clear_buffer_legacy(HANDLE hBuffer);
static void _draw_render_unit_legacy(MENU_RENDER_ARGUMENT rargument, COORD pos, PMENU_RENDER_UNIT render_unit);

/* ============== PUBLIC FUNCTION IMPLEMENTATIONS ============== */

/* ----- timing Functions ----- */
double tick()
{
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

/* ----- Menu Policy Functions ----- */
void change_menu_policy(MENU menu_to_change, int new_header_policy, int new_footer_policy)
{
    menu_to_change->menu_settings.header_enabled = (new_header_policy && 1) || 0;
    menu_to_change->menu_settings.footer_enabled = (new_footer_policy && 1) || 0;
}

void toggle_mouse(MENU menu_to_change)
{
    menu_to_change->menu_settings.mouse_enabled = menu_to_change->menu_settings.mouse_enabled  ^ 1;
}

/* ----- Configuration Functions (Setters) ----- */
void set_menu_settings(MENU menu, MENU_SETTINGS new_settings)
{
    memcpy((void*)&(menu->menu_settings), (void*)&new_settings, sizeof(MENU_SETTINGS));
    _clamp_center_coord(&(menu->menu_settings.menu_center));
}

void set_default_menu_settings(MENU_SETTINGS new_settings)
{
    if (menu_settings_initialized ^ TRUE) menu_settings_initialized = TRUE;
    memcpy((void*)&MENU_DEFAULT_SETTINGS, (void*)&new_settings, sizeof(MENU_SETTINGS));
}

void set_color_object(MENU menu, MENU_COLOR color_object)
{
    memcpy((void*)&(menu->color_object), (void*)&color_object, sizeof(MENU_COLOR));
}

void set_legacy_color_object(MENU menu, LEGACY_MENU_COLOR legacy_color_object)
{
    memcpy((void*)&(menu->legacy_color_object), (void*)&legacy_color_object, sizeof(LEGACY_MENU_COLOR));
}

void set_default_color_object(MENU_COLOR color_object)
{
    if (menu_color_initialized ^ TRUE) menu_color_initialized = TRUE;
    memcpy((void*)&MENU_DEFAULT_COLOR, (void*)&color_object, sizeof(MENU_COLOR));
}

void set_default_legacy_color_object(LEGACY_MENU_COLOR color_object)
{
    if (menu_legacy_color_initialized ^ TRUE) menu_legacy_color_initialized = TRUE;
    memcpy((void*)&MENU_LEGACY_DEFAULT_COLOR, (void*)&color_object, sizeof(LEGACY_MENU_COLOR));
}

/* ----- Creation Functions ----- */
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

    new_menu->menu_settings = create_new_settings();
    new_menu->color_object = create_color_object();
    new_menu->legacy_color_object = create_legacy_color_object();

    new_menu->hBuffer[0] = _createConsoleScreenBuffer();
    new_menu->hBuffer[1] = _createConsoleScreenBuffer();

    new_menu->menu_size = zero_point;
    new_menu->header = DEFAULT_HEADER_TEXT;
    new_menu->footer = DEFAULT_FOOTER_TEXT;

    new_menu->formatted_header = NULL;
    new_menu->formatted_header = NULL;

    _get_menu_size(new_menu);

    menus_array = (MENU*)_safe_realloc(menus_array, (menus_amount + 1) * sizeof(MENU));
    menus_array[menus_amount++] = new_menu;

    return new_menu;
}

MENU_ITEM create_menu_item(const char* restrict text, __menu_callback callback, void* callback_data)
{
    MENU_ITEM item = (MENU_ITEM)_safe_malloc(sizeof(struct __menu_item));
    item->text = text ? text : DEFAULT_MENU_TEXT;
    item->text_len = _count_utf8_chars(item->text);
    item->boundaries = (COORD)
    {
        item->text_len, 0
    };
    item->callback = callback; //
    item->data_chunk = callback_data;
    return item;
}

MENU_SETTINGS create_new_settings()
{
    if (menu_settings_initialized ^ 1) _init_menu_system();
    return MENU_DEFAULT_SETTINGS;
}

MENU_COLOR create_color_object()
{
    if (menu_color_initialized ^ 1) _init_menu_system();
    return MENU_DEFAULT_COLOR;
}

LEGACY_MENU_COLOR create_legacy_color_object()
{
    if (menu_legacy_color_initialized ^ 1) _init_menu_system();
    return MENU_LEGACY_DEFAULT_COLOR;
}

/* ----- Color Functions ----- */
MENU_RGB_COLOR mrgb(short r, short g, short b)
{
    return (MENU_RGB_COLOR)
    {
        r, g, b
    };
}

COLOR_OBJECT_PROPERTY new_rgb_color(int text_color, MENU_RGB_COLOR color)
{
    COLOR_OBJECT_PROPERTY object;
    sprintf(object.__rgb_seq, RGB_COLOR_SEQUENCE, text_color ? 38 : 48, color.r, color.g, color.b);
    return object;
}

COLOR_OBJECT_PROPERTY new_full_rgb_color(MENU_RGB_COLOR _color_foreground, MENU_RGB_COLOR _color_background)
{
    COLOR_OBJECT_PROPERTY object;
    sprintf(object.__rgb_seq, RGB_COLOR_DOUBLE_SEQUENCE,
            _color_foreground.r, _color_foreground.g, _color_foreground.b,
            _color_background.r, _color_background.g, _color_background.b);
    return object;
}

/* ----- Menu Operations ----- */
void add_option(MENU used_menu, const MENU_ITEM item)
{
    size_t new_size = used_menu->count + 1;
    MENU_ITEM* new_options = (MENU_ITEM*)_safe_realloc(used_menu->options, new_size * sizeof(MENU_ITEM));

    used_menu->options = new_options;
    used_menu->options[used_menu->count] = item;
    used_menu->count = new_size;
    _get_menu_size(used_menu);
}

void change_header(MENU used_menu, const char* restrict text)
{
    used_menu->header = text;
    _get_menu_size(used_menu);
}

void change_footer(MENU used_menu, const char* restrict text)
{
    used_menu->footer = text;
    _get_menu_size(used_menu);
}

void enable_menu(MENU used_menu)
{
    if (!used_menu || used_menu->count == 0)
        {
            _lwrite_string(hConsoleError, "Error: Menu has no options. Use add_option first!");
            exit(BAD_MENU);
        }

    if (_size_check(used_menu))
        _show_error_and_wait_extended(used_menu);

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
                _get_menu_size(used_menu);

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
                free(m->formatted_header);
                free(m->formatted_footer);

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
                        menus_array[0]->running = TRUE; // in case so we always have something to hold our back
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
        new_full_rgb_color(mrgb(255, 255, 255), mrgb(143, 32, 255)),
                           new_full_rgb_color(mrgb(255, 255, 255), mrgb(143, 32, 255)),
                           new_full_rgb_color(mrgb(0, 0, 0), mrgb(255, 255, 255))
    };
}

static LEGACY_MENU_COLOR _create_default_legacy_color()
{
    return (LEGACY_MENU_COLOR)
    {
        BRIGHT_RED,
        BRIGHT_RED,
        BG_WHITE
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
    settings.force_legacy_mode = DEFAULT_LEGACY_SETTING;
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

static WORD _check_if_supports_vt100()
{
    DWORD tdwmode;
    HANDLE tConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(tConsole, &tdwmode);

    if (tdwmode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) return 1;
    else return 0;
}

static void _init_wrapper_functions()
{
    if (vt100_support && DEFAULT_LEGACY_SETTING ^ 1)
        _clear_buffer_func = _clear_buffer;
    else
        _clear_buffer_func = _clear_buffer_legacy;
}

// this function runs once for the entire program cycle
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

    if (menu_settings_initialized ^ 1)
        set_default_menu_settings(_create_default_settings());
    if (menu_color_initialized ^ 1)
        set_default_color_object(_create_default_color());
    if (menu_legacy_color_initialized ^ 1)
        set_default_legacy_color_object(_create_default_legacy_color());

    // checking if console supports vt100
    vt100_support = _check_if_supports_vt100();

    // after check initializing all the wrapper functions
    _init_wrapper_functions();

    // random init
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
// counts the number of visible characters in a UTF-8 encoded string
static size_t _count_utf8_chars(const char* s)
{
    size_t count = 0;
    while (*s)
        {
            // in UTF-8, only the first byte of a multi-byte sequence does not start with binary '10'xxxxx
            if ((*s & 0xC0) != 0x80)
                {
                    count++;
                }
            s++;
        }
    return count;
}

static void _clamp_center_coord(MENU_COORD* coord)
{
    // x
    if (coord->X > 1.0f) coord->X = 1.0f;
    else if (coord->X < -1.0f) coord->X = -1.0f;

    // y
    if (coord->Y > 1.0f) coord->Y = 1.0f;
    else if (coord->Y < -1.0f) coord->Y = -1.0f;
}

static unsigned long long _random_uint64_t()
{
    return ((unsigned long long)rand() << 32) | rand();
}

static int _check_menu(unsigned long long saved_id)
{
    for (int i = 0; i < menus_amount; i++)
        if (menus_array[i] &&
                menus_array[i]->__ID == saved_id &&
                menus_array[i]->running) return TRUE;
    return FALSE;
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

/* ----- Console Management ----- */
static void _setConsoleActiveScreenBuffer(HANDLE hBuffer)
{
    SetConsoleActiveScreenBuffer(hBuffer);
    hCurrent = hBuffer;
}

static void _get_menu_size(MENU menu)
{
    size_t max_width = 0;

    // check width of all options
    for (int i = 0; i < menu->count; i++)
        {
            size_t current_width = menu->options[i]->text_len;
            if (current_width > max_width) max_width = current_width;
        }

    // also check header and footer width to ensure menu is wide enough
    if (menu->menu_settings.header_enabled)
        {
            size_t header_width = _count_utf8_chars(menu->header);
            if (header_width > max_width) max_width = header_width;
        }
    if (menu->menu_settings.footer_enabled)
        {
            size_t footer_width = _count_utf8_chars(menu->footer);
            if (footer_width > max_width) max_width = footer_width;
        }

    menu->menu_size.X = max_width + 4; // adding padding for " [ text ] " style
    menu->menu_size.Y = menu->count + 2; // base height: options + top/bottom borders

    if (menu->menu_settings.header_enabled) menu->menu_size.Y += 2; // add space for header and a blank line
    if (menu->menu_settings.footer_enabled) menu->menu_size.Y += 2; // add space for footer and a blank line

    menu->halt_size.X = menu->menu_size.X / 2;
    menu->halt_size.Y = menu->menu_size.Y / 2;

    _update_formatted_strings(menu);
}

static int _size_check(MENU menu)
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

static void _clear_buffer(HANDLE hBuffer)
{
    _lwrite_string(hBuffer, CLEAR_SCREEN);
    _lwrite_string(hBuffer, CLEAR_SCROLL_BUFFER);
    _lwrite_string(hBuffer, RESET_MOUSE_POSITION);
}

// for non vt consoles
static void _clear_buffer_legacy(HANDLE hBuffer)
{
    static COORD saved_buffer_size =
    {
        0, 0
    };
    static DWORD calculated_size = 1;

    DWORD swritten;
    COORD buffer_size;
    CONSOLE_SCREEN_BUFFER_INFO clr_csbi;

    SetConsoleCursorPosition(hBuffer, zero_point);
    if (GetConsoleScreenBufferInfo(hBuffer, &clr_csbi))
        {
            buffer_size = clr_csbi.dwSize;
            if (saved_buffer_size.X != buffer_size.X || saved_buffer_size.Y != buffer_size.Y)
                {
                    saved_buffer_size = buffer_size;
                    calculated_size = buffer_size.X * buffer_size.Y;
                }

            // clearing process
            FillConsoleOutputCharacter(hBuffer, ' ', calculated_size, zero_point, &swritten);
            FillConsoleOutputAttribute(hBuffer, reset_color_attribute, calculated_size, zero_point, &swritten);
        }
}

// implementation of the disabled mouse handler
static int _handle_mouse_event_disabled(MENU used_menu, const MOUSE_EVENT_RECORD* event,
                                        int y_min, int y_max, int x, int x_max,
                                        int* last_selected_index, int* something_is_selected, int* holding)
{
    // idc ig
    return FALSE;
}

// implementation of the enabled mouse handler
static int _handle_mouse_event_enabled(MENU used_menu, const MOUSE_EVENT_RECORD* event,
                                       int y_min, int y_max, int x, int x_max,
                                       int* last_selected_index, int* something_is_selected, int* holding)
{
    int mouse_option_selected = FALSE;
    COORD mouse_pos = event->dwMousePosition;

    if (mouse_pos.Y < y_max && mouse_pos.Y >= y_min && mouse_pos.X >= x && mouse_pos.X <= x_max)
        {
            int selected_index = mouse_pos.Y - y_min;
            if (mouse_pos.X <= used_menu->options[selected_index]->boundaries.X)
                {
                    used_menu->selected_index = selected_index;
                    mouse_option_selected = *something_is_selected = TRUE;
                    if (*last_selected_index != selected_index)
                        {
                            used_menu->need_redraw = TRUE;
                            *last_selected_index = selected_index;
                        }
                }
        }

    if (!mouse_option_selected && *something_is_selected)
        {
            *something_is_selected = FALSE;
            used_menu->need_redraw = TRUE;
            used_menu->selected_index = DISABLED;
            *last_selected_index = DISABLED;
        }

    if (event->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED && !(*holding))
        {
            *holding = TRUE;
            if (*something_is_selected)
                return TRUE; // signal to jump to the input handler
        }
    else if (event->dwButtonState == 0)
        *holding = FALSE;

    return FALSE; // signal to continue the loop
}


/* ----- Input Handling ----- */
static void _block_input(DWORD* oldMode)
{
    GetConsoleMode(hStdin, oldMode);
    DWORD newMode = *oldMode;
    newMode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    newMode |= (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
    if (vt100_support) newMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdin, newMode);
}

static void _reset_mouse_state()
{
    SendInput(1, &input, sizeof(INPUT));
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
    int running, event_running;
    DWORD objectWait, k, numEvents, oldMode;

    // error message intialization
    char error_message[BUFFER_CAPACITY];
    sprintf(error_message, ERROR_MESSAGE1, menu_size.X, menu_size.Y);
    strcat(error_message, ERROR_MESSAGE2);
    MENU_RENDER_UNIT errormsg_render_unit;
    MENU_RENDER_ARGUMENT rargument = _create_render_argument(HANDLE_TYPE, (void*)_hError);

    // pre-start calls
    _block_input(&oldMode);

    char TBUFFER[BUFFER_CAPACITY];
    sprintf(TBUFFER, error_message, current_size.X, current_size.Y);
    errormsg_render_unit = _create_render_unit(TBUFFER, ERROR_TYPE, NULL);
    _draw_render_unit_legacy(rargument, zero_point, &errormsg_render_unit);

    _setConsoleActiveScreenBuffer(_hError);
    FlushConsoleInputBuffer(hStdin);
    SetConsoleScreenBufferSize(_hError, menu_size);

    running = TRUE;
    while (running)
        {
            // some stuff is going on here!
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
                            _clear_buffer_func(_hError);
                            memset(TBUFFER, '\0', BUFFER_CAPACITY);
                            sprintf(TBUFFER, error_message, current_size.X, current_size.Y);
                            errormsg_render_unit.text = TBUFFER;
                            _draw_render_unit_legacy(rargument, zero_point, &errormsg_render_unit);
                        }
                }
        }

    _clear_buffer_func(_hError);
    FlushConsoleInputBuffer(hStdin);
    _reset_mouse_state();
    _setConsoleActiveScreenBuffer(menu->hBuffer[menu->active_buffer]);
    cached_size = current_size;
}

/* ----- Main Rendering Loop ----- */
static MENU_RENDER_UNIT _create_render_unit(const char* text, DWORD unit_type, void* extra_data)
{
    return (MENU_RENDER_UNIT)
    {
        text, unit_type, extra_data
    };
}

static MENU_RENDER_ARGUMENT _create_render_argument(enum RenderArgumentTag data_tag, void* value)
{
    MENU_RENDER_ARGUMENT new_argument;
    new_argument.tag = data_tag;
    if (data_tag == MENU_TYPE)
        new_argument.value.menu = (MENU)value;
    else
        new_argument.value.handle = (HANDLE)value;
    return new_argument;
}

static COORD _calculate_start_coordinates(MENU menu, COORD current_size)
{
    MENU_COORD normcoord = menu->menu_settings.menu_center;
    COORD menu_size = menu->menu_size;
    COORD newcoord;

    int haltx = menu->halt_size.X;
    int halty = menu->halt_size.Y;

    // main calculations
    newcoord.X = (float)(normcoord.X + 1.0f) * (float)(current_size.X - 1.0f) / 2.0f - haltx;
    newcoord.Y = (float)(1.0f - normcoord.Y) * (float)(current_size.Y - 1.0f) / 2.0f - halty;

    // clamping time
    if (newcoord.X + menu_size.X > current_size.X) newcoord.X -= haltx;
    if (newcoord.Y + menu_size.Y > current_size.Y) newcoord.Y -= OFFSET_VALUE;

    if (newcoord.X < 0) newcoord.X = 0;
    if (newcoord.Y < 0) newcoord.Y = 0;

    return newcoord;
}

static void _draw_render_unit(MENU_RENDER_ARGUMENT rargument, COORD pos, PMENU_RENDER_UNIT render_unit)
{
    HANDLE backBuffer;
    MENU_COLOR menu_color;

    if (rargument.tag == MENU_TYPE)
        {
            menu_color = rargument.value.menu->color_object;
            backBuffer = rargument.value.menu->hBuffer[rargument.value.menu->active_buffer ^ 1];
        }
    else
        {
            menu_color = MENU_DEFAULT_COLOR; // fallback for non-menu contexts jic
            backBuffer = rargument.value.handle;
        }

    DWORD unit_type = render_unit->unit_type;
    const char* color_seq = "";

    switch (unit_type)
        {
            case (HEADER_TYPE): // HEADER
            case (FOOTER_TYPE): // FOOTER
                _ldraw_at_position(backBuffer, pos.X, pos.Y, render_unit->text);
                break;
            case (SELECTABLE_TYPE): // SELECTABLE (option)
                if (*((WORD*)render_unit->extra_data))   // is selected
                    {
                        color_seq = menu_color.optionColor.__rgb_seq;
                        _draw_at_position(backBuffer, pos.X, pos.Y, "%s%s"RESET_ALL_STYLES, color_seq, render_unit->text);
                    }
                else
                    _ldraw_at_position(backBuffer, pos.X, pos.Y, render_unit->text);
                break;
        }
}

static void _draw_render_unit_legacy(MENU_RENDER_ARGUMENT rargument, COORD pos, PMENU_RENDER_UNIT render_unit)
{
    HANDLE backBuffer;
    LEGACY_MENU_COLOR menu_color;

    if (rargument.tag == MENU_TYPE)
        {
            menu_color = rargument.value.menu->legacy_color_object;
            backBuffer = rargument.value.menu->hBuffer[rargument.value.menu->active_buffer ^ 1];
        }
    else
        {
            menu_color = MENU_LEGACY_DEFAULT_COLOR;
            backBuffer = rargument.value.handle;
        }

    DWORD unit_type = render_unit->unit_type;
    WORD text_color = reset_color_attribute;

    switch (unit_type)
        {
            case (HEADER_TYPE): // HEADER
                text_color = menu_color.headerColor;
                break;
            case (FOOTER_TYPE): // FOOTER
                text_color = menu_color.footerColor;
                break;
            case (SELECTABLE_TYPE): // SELECTABLE (option)
                if (*((WORD*)render_unit->extra_data))
                    text_color = menu_color.optionColor;
                break;
            case (ERROR_TYPE): // error msgs
                text_color = ERROR_COLOR;
                break;
        }

    if (text_color == reset_color_attribute)
        _ldraw_at_position(backBuffer, pos.X, pos.Y, render_unit->text);
    else
        {
            SetConsoleTextAttribute(backBuffer, text_color);
            _ldraw_at_position(backBuffer, pos.X, pos.Y, render_unit->text);
            SetConsoleTextAttribute(backBuffer, reset_color_attribute);
        }
}

static void _renderMenu(const MENU used_menu)
{
    if (!used_menu) return;

    // BASIC VARIABLES BLOCK
    COORD current_size, old_size, start;
    int y_max, y_min, x_max;
    int size_check;
    int y, x, i, last_selected_index;

    unsigned long long saved_id; // saved menu ID to verify menu validity after callbacks
    static int something_is_selected; // static flag persisting between function calls (indicates if any option is visually selected)

    // declare the function pointer
    MouseEventHandler mouse_event_handler;

    HANDLE hBackBuffer;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT new_window;
    COORD menu_size, mouse_pos;

    DWORD old_mode, numEvents, event, waitResult;
    WORD vk;
    INPUT_RECORD inputRecords[EVENT_MAX_RECORDS];

#ifdef DEBUG
    int mouse_status;
    size_t tick_count;
    COORD debug_mouse_pos;
#endif

    GetConsoleScreenBufferInfo(hCurrent, &csbi);
    old_size = current_size = (COORD)
    {
        csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1
    };

    last_selected_index = DISABLED;

    menu_size = used_menu->menu_size;
    saved_id = used_menu->__ID;
    used_menu->need_redraw = TRUE;

    used_menu->selected_index = used_menu->menu_settings.mouse_enabled ? DISABLED : 0;

    MENU_RENDER_ARGUMENT rargument = _create_render_argument(MENU_TYPE, used_menu);

    // predefined render units
    MENU_RENDER_UNIT header_render_unit = _create_render_unit("", HEADER_TYPE, NULL);
    MENU_RENDER_UNIT option_render_unit = _create_render_unit("", SELECTABLE_TYPE, NULL);
    MENU_RENDER_UNIT footer_render_unit = _create_render_unit("", FOOTER_TYPE, NULL);

    RenderUnitDrawer _draw_render_unit_func = (vt100_support && used_menu->menu_settings.force_legacy_mode ^ 1)
            ? _draw_render_unit
            : _draw_render_unit_legacy;

    // assign the correct mouse handler ONCE before the loop starts
    if (used_menu->menu_settings.mouse_enabled)
        mouse_event_handler = _handle_mouse_event_enabled;
    else
        mouse_event_handler = _handle_mouse_event_disabled;

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
                    FlushConsoleInputBuffer(hStdin);
                    used_menu->need_redraw = 1;
                }
            if (used_menu->need_redraw)
                {
                    hBackBuffer = used_menu->hBuffer[used_menu->active_buffer ^ 1];
                    _clear_buffer_func(hBackBuffer);
                    start = _calculate_start_coordinates(used_menu, current_size);

#ifdef DEBUG
                    _draw_at_position(hBackBuffer, 0, 0, "__ID: %llu", used_menu->__ID);
                    _draw_at_position(hBackBuffer, 0, 2, "SELECTED: %d", something_is_selected);
                    _draw_at_position(hBackBuffer, 0, 6, "WINDOW SIZE: %d %d ", current_size.X, current_size.Y);
                    _draw_at_position(hBackBuffer, 0, 10, "MOUSE POS: %d %d    ", debug_mouse_pos.X, debug_mouse_pos.Y);
                    _draw_at_position(hBackBuffer, 0, 14, "menus_amount: %d", menus_amount);
                    _print_memory_info(hBackBuffer); // -> 30
                    _draw_at_position(hBackBuffer, 0, 32, "menus start coord: %d %d", start.X, start.Y);
#endif

                    // header
                    if (used_menu->menu_settings.header_enabled)
                        {
                            header_render_unit.text = used_menu->formatted_header;
                            _draw_render_unit_func(rargument,
                                                   (COORD)
                            {
                                start.X + 2, start.Y + 1
                            }, &header_render_unit);
                        }

                    // options
                    x = start.X + 2;
                    y = y_min = start.Y + (used_menu->menu_settings.header_enabled ? 3 : 1);
                    x_max = 0;

                    for (i = 0; i < used_menu->count; i++, y++)
                        {
                            // option printing setup
                            WORD is_selected = (i == used_menu->selected_index);
                            option_render_unit.text = used_menu->options[i]->text;
                            option_render_unit.extra_data = (void*)&is_selected;
                            _draw_render_unit_func(rargument,
                                                   (COORD)
                            {
                                x, y
                            }, &option_render_unit);

                            // boundaries calc
                            used_menu->options[i]->boundaries.Y = y;
                            used_menu->options[i]->boundaries.X = x + used_menu->options[i]->text_len - 1;
                            if (used_menu->options[i]->boundaries.X > x_max)
                                x_max = used_menu->options[i]->boundaries.X;
                        }
                    y_max = y;

                    // footer
                    if (used_menu->menu_settings.footer_enabled)
                        {
                            footer_render_unit.text = used_menu->formatted_footer;
                            _draw_render_unit_func(rargument,
                                                   (COORD)
                            {
                                start.X + 2, y + 1
                            }, &footer_render_unit);
                        }

                    _setConsoleActiveScreenBuffer(hBackBuffer);
                    used_menu->active_buffer ^= 1;
                    used_menu->need_redraw = FALSE;
                }

            // events handling
            waitResult = WaitForSingleObject(hStdin, UPDATE_FREQUENCE);
            if (waitResult == WAIT_OBJECT_0)
                {
                    if (GetNumberOfConsoleInputEvents(hStdin, &numEvents))
                        {
                            ReadConsoleInput(hStdin, inputRecords, min(EVENT_MAX_RECORDS, numEvents), &numEvents);
                            for (event = 0; event < numEvents; event++)
                                switch(inputRecords[event].EventType)
                                    {
                                        case KEY_EVENT:
                                            if (inputRecords[event].Event.KeyEvent.bKeyDown)
                                                {
                                                    vk = inputRecords[event].Event.KeyEvent.wVirtualKeyCode;
                                                    if ((vk == VK_UP) || (vk == VK_DOWN) || (vk == VK_RETURN) || (vk == VK_ESCAPE) || (vk == VK_DELETE))
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

                                                                                _clear_buffer_func(hConsole);
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
                                                            goto next_event_iteration; // break from key handling to avoid processing other events in this batch
                                                        }
                                                }
                                            break;
                                        case MOUSE_EVENT:
#ifdef DEBUG
                                            debug_mouse_pos = inputRecords[event].Event.MouseEvent.dwMousePosition;
                                            mouse_status = inputRecords[event].Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED;
                                            _draw_at_position(hCurrent, 0, 10, "MOUSE POS: %d %d    ", debug_mouse_pos.X, debug_mouse_pos.Y);
#endif
                                            // a single, unconditional function call
                                            if (mouse_event_handler(used_menu, &inputRecords[event].Event.MouseEvent,
                                                                    y_min, y_max, x, x_max,
                                                                    &last_selected_index, &something_is_selected, &holding))
                                                {
                                                    goto input_handler;
                                                }
                                            break;
                                        case WINDOW_BUFFER_SIZE_EVENT:
                                            current_size = inputRecords[event].Event.WindowBufferSizeEvent.dwSize;
                                            break;
                                    }
                        next_event_iteration:
                            ;
                        }
                    FlushConsoleInputBuffer(hStdin);
                }
        }
end_render_loop:; // anchor
    FlushConsoleInputBuffer(hStdin);
    SetConsoleMode(hStdin, old_mode);
    if (menus_amount == 0)
        _setConsoleActiveScreenBuffer(hConsole);
    else _setConsoleActiveScreenBuffer(_find_first_active_menu_buffer());
}

// TODO: check
static void _update_formatted_strings(MENU menu)
{
    free(menu->formatted_header);
    free(menu->formatted_footer);

    // determine the inner width for text content, ensuring its not negative
    size_t inner_width = menu->menu_size.X > 4 ? menu->menu_size.X - 4 : 0;

    // determine if we are using modern VT100 rendering or legacy
    int use_vt100 = vt100_support && (menu->menu_settings.force_legacy_mode ^ 1);

    // size is menu_size.X (visual width) * 4 (max UTF-8 bytes) + color codes + null terminator
    size_t buffer_size = menu->menu_size.X * 4 + MAX_RGB_LEN * 2;
    menu->formatted_header = _safe_malloc(buffer_size);
    menu->formatted_footer = _safe_malloc(buffer_size);

    // format Header
    size_t header_text_len = _count_utf8_chars(menu->header);
    size_t header_pad_left = (inner_width > header_text_len) ? (inner_width - header_text_len) / 2 : 0;
    size_t header_pad_right = (inner_width > header_text_len) ? (inner_width - header_text_len - header_pad_left) : 0;

    if (use_vt100)
        snprintf(menu->formatted_header, buffer_size, "%s%*s%s%*s" RESET_ALL_STYLES,
                 menu->color_object.headerColor.__rgb_seq,
                 (int)header_pad_left, "", menu->header, (int)header_pad_right, "");
    else
        snprintf(menu->formatted_header, buffer_size, "%*s%s%*s",
                 (int)header_pad_left, "", menu->header, (int)header_pad_right, "");

    // format Footer
    size_t footer_text_len = _count_utf8_chars(menu->footer);
    size_t footer_pad_left = (inner_width > footer_text_len) ? (inner_width - footer_text_len) / 2 : 0;
    size_t footer_pad_right = (inner_width > footer_text_len) ? (inner_width - footer_text_len - footer_pad_left) : 0;

    if (use_vt100)
        snprintf(menu->formatted_footer, buffer_size, "%s%*s%s%*s" RESET_ALL_STYLES,
                 menu->color_object.footerColor.__rgb_seq,
                 (int)footer_pad_left, "", menu->footer, (int)footer_pad_right, "");
    else
        snprintf(menu->formatted_footer, buffer_size, "%*s%s%*s",
                 (int)footer_pad_left, "", menu->footer, (int)footer_pad_right, "");
}