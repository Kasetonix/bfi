#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _POSIX_SOURCE
#include <wchar.h>
#include <locale.h>
#endif

#include "bfi.h"

int main(int argc, char *argv[]) {
    CharDA instructions;
    u8 tape[TAPE_LEN] = { 0 };
    FILE *src_file = NULL;

    #ifdef _POSIX_SOURCE
    setlocale(LC_ALL, "");
    #endif

    if (argc < 2)
        error("[ERR]: Too little arguments.\n");
    const char *path = argv[1];

    open_file(&src_file, path);
    da_init(&instructions);
    read_src_to_da(src_file, &instructions);
    fclose(src_file);

    if (!valid_loops(&instructions))
        error("[ERR]: Invalid loops found.\n");

    run(&instructions, tape);
    free(instructions.data);
    return 0;
}

// Prints an error message and exits
void error(const char *msg) {
    fputs(msg, stderr);
    exit(1);
}

// Opens a file for reading
void open_file(FILE **file, const char* path) {
    *file = fopen(path, "r");
    if (*file == NULL) {
        fprintf(stderr, "[ERR]: Couldn't open file '%s'.\n", path);
        exit(1);
    }
}

// Initializes a dynamic array
void da_init(CharDA *array) {
    array->length = 0;
    array->capacity = INIT_DA_CAPACITY;
    array->data = malloc(array->capacity * sizeof(i8));

    if (array->data == NULL)
        error("[ERR]: Failed to allocate memory.\n");
}

// Appends to a dynamic array
void da_append(CharDA *array, i8 val) {
    if (array->length < array->capacity) {
        array->data[array->length++] = val;
        return;
    }

    array->capacity *= 2;
    array->data = realloc(array->data, array->capacity * sizeof(i8));

    if (array->data == NULL)
        error("[ERR]: Failed to allocate memory.\n");

    array->data[array->length++] = val;
}

// Initializes a stack
void stack_init(PtrStack *stack) {
    stack->size = 0;
    stack->capacity = STACK_CAPACITY;
    for (u8 i = 0; i < stack->capacity; i++)
        stack->data[i] = NULL;
}

// Retrieves a value from the top of the stack
// If empty returns NULL
i8 *stack_top(PtrStack *stack) {
    if (stack->size == 0)
        return NULL;
    return stack->size > 0? stack->data[stack->size - 1] : NULL;
}

// Pushes a pointer onto the stack
void stack_push(PtrStack *stack, i8 *ptr) {
    if (stack->size == stack->capacity)
        error("[ERR]: Stack overflow.\n");
    stack->data[stack->size++] = ptr;
}

// Pops a pointer off a stack
i8 *stack_pop(PtrStack *stack) {
    if (stack->size == 0)
        error("[ERR]: Attemted to pop off an empty stack.\n");
    return stack->data[--stack->size];
}

// Writes the source code without comments to a dynamic array
void read_src_to_da(FILE *file, CharDA *instructions) {
    i8 ch;
    ch = fgetc(file);
    while (ch != EOF) {
        if (ch == '+' || ch == '-' ||
            ch == '>' || ch == '<' ||
            ch == '[' || ch == ']' ||
            ch == '.' || ch == ',' || ch == '#')
            da_append(instructions, ch);
        ch = fgetc(file);
    }
}

// Checks validity of all [ ] pairs
bool valid_loops(CharDA *instructions) {
    i8 depth = 0;
    for (size_t i = 0; i < instructions->length; i++) {
        if (instructions->data[i] == '[')
            depth++;
        else if (instructions->data[i] == ']')
            depth--;
        if (depth < 0)
            return false;
    }

    return depth == 0? true : false;
}

#ifdef _POSIX_SOURCE
// prints a unicode representation of a given control character
void print_cc(u8 code) {
    wchar_t unich = u'\u2400' + code;
    printf("%lc", unich);
}
#else
// prints a unicode representation of a given control character
void print_cc(u8 code) {
    u8 unich[] = "\u2400";
    unich[2] += code;
    printf("%s", unich);
}
#endif

// prints the initial TAPE_BEG_CHARS of the tape
void print_tape(u8 *tape_left_bound, u8 *tape_ptr, u8 lpc) {
    u8 ch;
    size_t ptr_pos;

    if (lpc != '\n') putchar('\n');
    for (u8 i = 0; i < TAPE_BEG_CHARS; i++) {
        ch = *(tape_left_bound + i);
        if (32 < ch && ch < 127) putchar(ch);
        else if (ch <= 32) print_cc(ch);
        else if (ch == 127) print_cc(33); // DL
        else print_cc(39); // checkered box
    } putchar('\n');

    ptr_pos = tape_ptr - tape_left_bound + 1;
    if (ptr_pos <= 64)
        printf("%*c\n", (int) ptr_pos, '^');
}

// Manually finds the matching closing bracket
i8 *find_matching_cbr(i8 *instr_ptr) {
    size_t depth = 0;
    while (true) {
        instr_ptr++;
        if (*instr_ptr == ']') {
            if (depth == 0) break;
            else depth--;
        }

        if (*instr_ptr == '[')
            depth++;
    }

    return instr_ptr;
}

// Function for handling opening brackets
void handle_obr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr) {
    if (stack_top(obr) != *instr_ptr) { // First entry into this obr
        if (*tape_ptr == 0) // Immidiately jumping after the manually found matching cbr
            *instr_ptr = find_matching_cbr(*instr_ptr);
        else
            stack_push(obr, *instr_ptr);
    }
}

// Function for handling closing brackets
void handle_cbr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr, PtrStack *cbr) {
    if (*tape_ptr != 0) { // jump to matching obr
        if (stack_top(cbr) != *instr_ptr)
            stack_push(cbr, *instr_ptr);
        *instr_ptr = stack_top(obr);
    } else {
        if (stack_top(cbr) == *instr_ptr)
            stack_pop(cbr);
        stack_pop(obr);
    }
}

// Runs the program reading from instruction array
void run(CharDA *instructions, u8 *tape_ptr) {
    PtrStack obr, cbr; // opening/closing bracket stack
    stack_init(&obr);
    stack_init(&cbr);
    u8 lpc= '\n'; // last printed char

    i8 *instr_ptr = instructions->data;
    i8 *instr_end = instr_ptr + instructions->length;
    u8 *tape_left_bound = tape_ptr;
    u8 *tape_right_bound = tape_ptr + TAPE_LEN - 1;

    while (instr_ptr < instr_end) {
        switch (*instr_ptr) {
            case '+': (*tape_ptr)++; break;
            case '-': (*tape_ptr)--; break;
            case '>':
                if (tape_ptr == tape_right_bound)
                    error("[ERR]: Pointer out of bounds. (>)\n");
                tape_ptr++; break;
            case '<':
                if (tape_ptr == tape_left_bound)
                    error("[ERR]: Pointer out of bounds. (<)\n");
                tape_ptr--; break;
            case '.': lpc = *tape_ptr; putchar(lpc); break;
            case ',': *tape_ptr = getchar(); break;
            case '[': handle_obr(&instr_ptr, tape_ptr, &obr); break;
            case ']': handle_cbr(&instr_ptr, tape_ptr, &obr, &cbr); break;
            case '#': print_tape(tape_left_bound, tape_ptr, lpc); break;
        }

        instr_ptr++;
    }
}
