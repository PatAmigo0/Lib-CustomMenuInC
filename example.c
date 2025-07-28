#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <time.h>
#include "menu.h"

#define MAX_SIZE 5
#define MAX 10
#define NUMBER_LIMIT 1000
#define MATRIX_NUMBER_LIMIT 10
#define ZERO_CHANCE 3 // In procents, ex. 1 = 10% and 10 = 100%, 0 = error
#define MAX_ERRORS 1
#define COMMAND "C:\\Users\\kusko\\Desktop\\Projects\\MainProgram\\sort_file.exe input.csv output.csv"

static int* s_array_to_sort = NULL;
static int* s_array_copy = NULL;
static int s_array_size = 0;
static size_t s_array_alloc_size = 0;

/*
	TYPEDEFS
*/
typedef struct
{
    int column_index;
    int sum_number;
} matrixSortInfo;

/*
	CONSTANTS
*/
unsigned int SEED = 0;

/*
	UTILIES
*/
void newSeed()
{
    srand(++SEED);
}

void secure_output()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void petc()
{
    fflush(stdin);
    printf("\nPress %sEnter%s to continue...\n", WHITE_BG_BLACK_TEXT, RESET_ALL_STYLES);
    getchar();
    system("cls");
}

void error_input_handle()
{
    secure_output();
    printf("%sInvalid input.%s\n", BRIGHT_RED_TEXT, RESET_ALL_STYLES);
    petc();
}

/*
	ARRAY UTILITIES
*/
int array_sum(int* array, int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += array[i];
    return sum;
}

void swap(int* a, int* b)
{
    int dummy_value = *a;
    *a = *b;
    *b = dummy_value;
}

void print_array(int* array, int n)
{
    int i;
    for (i = 0; i < n; i++)
        printf("%d ", array[i]);
    putchar('\n');
}

int* generate_random_array(int size)
{
    newSeed();
    int* new_array = (int*)malloc(size * sizeof(int));

    for (int i = 0; i < size; i++)
        new_array[i] = rand() % NUMBER_LIMIT + 1;

    return new_array;
}

int* make_empty_array(int size)
{
    int* new_array = (int*)malloc(size * sizeof(int));
    return new_array;
}

void free_array(int* array)
{
    free(array);
}

/*
	BUBBLE SORT
*/
void bubble_sort_fixed_nondecreasing(int* start_pointer, int n)
{
    int i, j;
    for (i = 0; i < n; i++)
        for (j = 0; j < n-1; j++)
            if (start_pointer[j] > start_pointer[j+1])
                swap(&start_pointer[j], &start_pointer[j+1]);
}

void bubble_sort_nonfixed_nongrowth(int* start_pointer, int n)
{
    int i, j, swapped = 1;
    for (i = 0; i < n && swapped; i++)
        {
            swapped = 0;
            for (j = 0; j < n-1; j++)
                if (start_pointer[j] < start_pointer[j+1])
                    {
                        swap(&start_pointer[j], &start_pointer[j+1]);
                        if (swapped == 0) swapped = 1;
                    }
        }
}

/*
	INSERTION SORT
*/
void insertion_sort_nondecreasing(int* array, int n)
{
    int i, j;
    for (i = 1; i < n; i++)
        {
            j = i;
            while (j > 0 && array[j-1] > array[j])
                {
                    swap(&array[j-1], &array[j]);
                    j--;
                }
        }
}

/*
	SELECTION SORT
*/
void selection_sort_nondecreasing(int* array, int n)
{
    int i, j, k, min_number, min_number_index;

    for (i = 0; i < n; i++)
        {
            min_number = array[i];
            min_number_index = i;
            for (j = i; j < n; j++)
                if (array[j] < min_number)
                    min_number = array[j],
                    min_number_index = j;

            for (k = min_number_index - 1; k >= i; k--)
                array[k+1] = array[k];
            array[i] = min_number;
        }
}

/*
	MERGE SORT
*/
void _merge(int* array, int left, int mid, int right)
{
    int i, j, k = left;
    int n1 = mid - left + 1, n2 = right - mid;
    int* L = (int*)malloc(n1 * sizeof(int));
    int* R = (int*)malloc(n2 * sizeof(int));

    for (i = 0; i < n1; i++) L[i] = array[left + i];
    for (i = 0; i < n2; i++) R[i] = array[mid + 1 + i];

    i = j = 0;

    while (i < n1 && j < n2)
        {
            if (L[i] >= R[j]) array[k] = L[i++];
            else array[k] = R[j++];
            k++;
        }

    while (i < n1) array[k++] = L[i++];
    while (j < n2) array[k++] = R[j++];

    free(L);
    free(R);
}

void merge_sort_nongrowth(int* array, int left, int right)
{
    if (left < right)
        {
            int mid = left + (right - left) / 2;
            merge_sort_nongrowth(array, left, mid);
            merge_sort_nongrowth(array, mid + 1, right);
            _merge(array, left, mid, right);
        }
}

/*
	QUICK SORT
*/
int _partition(int* array, int left, int right)
{
    int i = left - 1, j,
        pivot = array[right];
    for (j = left; j < right; j++)
        if (array[j] < pivot)
            swap(&array[++i], &array[j]);
    int pr = i + 1;
    swap(&array[pr], &array[right]);
    return pr;
}

void quick_sort_nongrowth(int* array, int left, int right)
{
    if (left < right)
        {
            int pr = _partition(array, left, right);
            quick_sort_nongrowth(array, left, pr - 1);
            quick_sort_nongrowth(array, pr + 1, right);
        }
}

/*
	UTILIES FOR STRUCTURES
*/
int compare_matrixSortInfo(const void* a, const void* b)
{
    return ((matrixSortInfo*)a)->sum_number - ((matrixSortInfo*)b)->sum_number;
}

/*
	UTILITIES FOR MATRIX
*/
int _column_sum(int** matrix, int matrix_height, int column_index)
{
    int sum = 0;
    for (int i = 0; i < matrix_height; i++)
        sum += matrix[i][column_index];
    return sum;
}

void _swap_rows(void *a, void *b, size_t size)
{
    unsigned char* temp = (unsigned char*)malloc(size);
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
    free(temp);
}

void _copy(void *where_to_copy, void *what_to_copy, size_t size)
{
    memcpy(where_to_copy, what_to_copy, size);
}

int** generate_random_matrix(int n, int m)
{
    newSeed();
    int i, j;
    int** matrix = (int**)malloc(n * sizeof(int*));
    for (i = 0; i < n; i++)
        {
            matrix[i] = (int*)malloc(m * sizeof(int));
            for (j = 0; j < m; j++)
                {
                    int randomNumber = rand() % MATRIX_NUMBER_LIMIT;
                    matrix[i][j] = randomNumber < ZERO_CHANCE ? 0 : randomNumber;
                }
        }

    return matrix;
}

int** build_empty_matrix(int n, int m)
{
    int** matrix = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++)
        matrix[i] = (int*)malloc(m * sizeof(int));
    return matrix;
}

void free_matrix(int** matrix, int n)
{
    int i;
    for (i = 0; i < n; i++)
        free(matrix[i]);
    free(matrix);
}

void print_matrix(int** matrix, int n, int m)
{
    int width = 0, number_limit = NUMBER_LIMIT,
        i, j;

    while (number_limit > 0)
        width++, number_limit /= 10;

    putchar('\n');
    for (i = 0; i < n; i++)
        {
            for (j = 0; j < m; j++)
                printf("%-*d ", width, matrix[i][j]);
            putchar('\n');
        }
}

void sort_matrix(int** matrix, int n, int m)
{
    int i, j;

    matrixSortInfo* count_array = (matrixSortInfo*)malloc(m * sizeof(matrixSortInfo));
    for (i = 0; i < m; i++)
        count_array[i] = (matrixSortInfo)
        {
            i, _column_sum(matrix, n, i)
        };

    qsort(count_array, m, sizeof(matrixSortInfo), compare_matrixSortInfo);

    int** temp_matrix = build_empty_matrix(n, m);
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            temp_matrix[j][i] = matrix[j][count_array[i].column_index];

    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            matrix[j][i] = temp_matrix[j][i];

    free(count_array);
    free_matrix(temp_matrix, n);
}

/*
	MENU FUNCTIONS (UPDATED)
*/

// FOR ANY MENU
void go_back(void* void_menu)
{
    clear_menu((MENU)void_menu);
}

static void common_after_sort(double elapsed_time)
{
    printf("Time taken: %lfs\n", elapsed_time);
    fflush(stdout);
    char answer;
    printf("Do you want to view the sorted array? (%sY%s/%sN%s): ", BRIGHT_GREEN_TEXT, RESET_ALL_STYLES, BRIGHT_RED_TEXT, RESET_ALL_STYLES);
    scanf(" %c", &answer);
    secure_output();
    if (answer == 'Y' || answer == 'y')
        print_array(s_array_copy, s_array_size);
    printf("LAST VALUE IS %d\n", s_array_copy[s_array_size - 1]);
    petc();
}

static void quick_sort_wrapper()
{
    _copy((void*)s_array_copy, (void*)s_array_to_sort, s_array_alloc_size);
    double ntick = tick();
    quick_sort_nongrowth(s_array_copy, 0, s_array_size - 1);
    common_after_sort(tick() - ntick);
}

static void merge_sort_wrapper()
{
    _copy((void*)s_array_copy, (void*)s_array_to_sort, s_array_alloc_size);
    double ntick = tick();
    merge_sort_nongrowth(s_array_copy, 0, s_array_size - 1);
    common_after_sort(tick() - ntick);
}

static void selection_sort_wrapper()
{
    _copy((void*)s_array_copy, (void*)s_array_to_sort, s_array_alloc_size);
    double ntick = tick();
    selection_sort_nondecreasing(s_array_copy, s_array_size);
    common_after_sort(tick() - ntick);
}

static void insertion_sort_wrapper()
{
    _copy((void*)s_array_copy, (void*)s_array_to_sort, s_array_alloc_size);
    double ntick = tick();
    insertion_sort_nondecreasing(s_array_copy, s_array_size);
    common_after_sort(tick() - ntick);
}

static void bubble_sort_wrapper()
{
    _copy((void*)s_array_copy, (void*)s_array_to_sort, s_array_alloc_size);
    double ntick = tick();
    bubble_sort_nonfixed_nongrowth(s_array_copy, s_array_size);
    common_after_sort(tick() - ntick);
}

void select_sort_type_arrays(int* array, int n)
{
    MENU new_menu = create_menu();
    change_header(new_menu, "SORT METHODS");
    change_menu_policy(new_menu, 1, 0);

    s_array_alloc_size = n * sizeof(int);
    s_array_copy = (int*)malloc(s_array_alloc_size);
    s_array_to_sort = array;
    s_array_size = n;

    add_option(new_menu, create_menu_item("Use quick sort", quick_sort_wrapper, NULL));
    add_option(new_menu, create_menu_item("Use merge sort", merge_sort_wrapper, NULL));
    add_option(new_menu, create_menu_item("Use selection sort", selection_sort_wrapper, NULL));
    add_option(new_menu, create_menu_item("Use insertion sort", insertion_sort_wrapper, NULL));
    add_option(new_menu, create_menu_item("Use bubble sort", bubble_sort_wrapper, NULL));
    add_option(new_menu, create_menu_item("Go back", go_back, NULL));
    enable_menu(new_menu);

    free(s_array_copy);
    s_array_copy = NULL;
}

// MENU 4
void menu4_item_1()
{
    int result = system(COMMAND);
    if (result == 0)
        printf(GREEN_BG_WHITE_TEXT"Successfully sorted file!\n"RESET_ALL_STYLES);
    else
        printf(BRIGHT_RED_TEXT"Error while trying, check these files:"COMMAND"\n"RESET_ALL_STYLES);

    petc();
}

// MENU 3
void matrix_common_process(int** matrix, int n, int m)
{
    char answer;
    printf("Do you want to view the generated matrix? (%sY%s/%sN%s): ", BRIGHT_GREEN_TEXT, RESET_ALL_STYLES, BRIGHT_RED_TEXT, RESET_ALL_STYLES);
    scanf(" %c", &answer);
    secure_output();
    if (answer == 'Y' || answer == 'y')
        print_matrix(matrix, n, m);
    petc();

    double start_time = tick();
    sort_matrix(matrix, n, m);
    double end_time = tick();

    printf("Time taken for sorting: %lfs\n", end_time - start_time);

    printf("Do you want to view the sorted matrix? (%sY%s/%sN%s): ", BRIGHT_GREEN_TEXT, RESET_ALL_STYLES, BRIGHT_RED_TEXT, RESET_ALL_STYLES);
    scanf(" %c", &answer);
    secure_output();
    if (answer == 'Y' || answer == 'y')
        print_matrix(matrix, n, m);

    petc();
}

void menu3_item_1()
{
    int n, m;
    printf("Enter the number of rows: ");
    if (scanf("%d", &n) != 1)
        {
            error_input_handle();
            return;
        }
    printf("Enter the number of columns: ");
    if (scanf("%d", &m) != 1)
        {
            error_input_handle();
            return;
        }

    secure_output();

    int** matrix = generate_random_matrix(n, m);
    matrix_common_process(matrix, n, m);
    free_matrix(matrix, n);
}

void menu3_item_2()
{
    int n, m;
    printf("Enter the number of rows: ");
    if (scanf("%d", &n) != 1)
        {
            error_input_handle();
            return;
        }

    printf("Enter the number of columns: ");
    if (scanf("%d", &m) != 1)
        {
            error_input_handle();
            return;
        }

    secure_output();

    int** matrix = build_empty_matrix(n, m);
    int i, j;
    for (i = 0; i < n; i++)
        {
            int row_valid = 0;
            while (!row_valid)
                {
                    row_valid = 1;
                    printf("Enter row %d (%d values separated by spaces): ", i+1, m);
                    for (j = 0; j < m; j++)
                        if (scanf("%d", &matrix[i][j]) != 1)
                            {
                                printf(BRIGHT_RED_TEXT "Invalid input at column %d. Please re-enter the entire row.\n" RESET_ALL_STYLES, j+1);
                                secure_output();
                                row_valid = 0;
                                break;
                            }
                }
        }

    matrix_common_process(matrix, n, m);
    free_matrix(matrix, n);
}

// MENU 2
void menu2_item_1()
{
    fflush(stdin);
    int n;
    printf("Please enter how much values to generate: ");
    if (scanf("%d", &n) != 1)
        {
            error_input_handle();
            return;
        }

    secure_output();
    int* new_generated_array = generate_random_array(n);
    printf("Do you want to view generated array? (%sY%s/%sN%s): ", BRIGHT_GREEN_TEXT, RESET_ALL_STYLES, BRIGHT_RED_TEXT, RESET_ALL_STYLES);
    char answer;
    if (scanf(" %c", &answer) != 1)
        {
            free(new_generated_array);
            secure_output();
            return;
        }

    if (answer == 'Y' || answer == 'y') print_array(new_generated_array, n);
    petc();
    fflush(stdin);

    select_sort_type_arrays(new_generated_array, n);
    free(new_generated_array);
}

void menu2_item_2(MENU m)
{
    clear_menu(m); // clearing menu at the start, wont break because system handles that
    
    fflush(stdin);
    int n, blocked = 0;
    printf("Please enter how much values you want to enter: ");
    while (scanf("%d", &n) != 1)
        {
            secure_output();
            printf("Please enter any valid number: ");
        }

    int* new_array = make_empty_array(n);

    printf("Enter %d amount of values below.\n", n);
    for (int i = 0; i < n && blocked == 0; i++)
        {
            int error_count = 0;
            while (scanf("%d", &new_array[i]) != 1)
                {
                    fflush(stdin);
                    error_count++;
                    if (error_count < MAX_ERRORS)
                        puts(BRIGHT_RED_TEXT"Wrong value! This one wont be counted, enter again."RESET_ALL_STYLES);
                    else
                        {
                            puts(BRIGHT_RED_TEXT"Too much mistakes, exiting..."RESET_ALL_STYLES);
                            blocked = 1;
                            break;
                        }
                }
        }

    if (blocked) petc();
    else
        {
            printf(BRIGHT_GREEN_TEXT"\nSuccess!\n"RESET_ALL_STYLES);
            petc();
            select_sort_type_arrays(new_array, n);
        }
    free(new_array);
}

// MENU 1
void menu1_item_1()
{
    MENU new_menu = create_menu();
    change_header(new_menu, "Array control panel");
    change_menu_policy(new_menu, 1, 0);
    add_option(new_menu, create_menu_item("Generate random array", menu2_item_1, NULL));
    add_option(new_menu, create_menu_item("Enter your own array", menu2_item_2, NULL));
    add_option(new_menu, create_menu_item("Go back", go_back, NULL));
    enable_menu(new_menu);
}

void menu1_item_2()
{
    MENU new_menu = create_menu();
    change_header(new_menu, "Matrix control panel");
    change_menu_policy(new_menu, 1, 0);
    change_width_policy(new_menu, 0);
    add_option(new_menu, create_menu_item("Generate random matrix", menu3_item_1, NULL));
    add_option(new_menu, create_menu_item("Enter your own matrix", menu3_item_2, NULL));
    add_option(new_menu, create_menu_item("Go back", go_back, NULL));
    enable_menu(new_menu);
}

void menu1_item_3()
{
    MENU new_menu = create_menu();
    change_header(new_menu, "File control");
    change_menu_policy(new_menu, 1, 0);
    add_option(new_menu, create_menu_item("Sort input.csv and put in output.csv", menu4_item_1, NULL));
    add_option(new_menu, create_menu_item("Go back", go_back, NULL));
    enable_menu(new_menu);
}

void exit_menu()
{
	clear_menus_and_exit();
}

int main()
{
	MENU_SETTINGS new_settings = create_new_settings();
	new_settings->mouse_enabled = 1;
	set_default_menu_settings(new_settings);
	
	MENU_COLOR new_color = create_color_object();
	new_color->headerColor = GREEN_BG_WHITE_TEXT;
	new_color->footerColor = YELLOW_BG BLUE_TEXT;
	set_default_color_object(new_color);

    SEED = time(NULL);
    MENU new_menu = create_menu();
    
    add_option(new_menu, create_menu_item("Sort array", menu1_item_1, NULL));
    add_option(new_menu, create_menu_item("Sort matrix", menu1_item_2, NULL));
    add_option(new_menu, create_menu_item("Sort file by date", menu1_item_3, NULL));
    add_option(new_menu, create_menu_item("Exit the program", exit_menu, NULL));
    enable_menu(new_menu);
}