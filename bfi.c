#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define INIT_DA_CAPACITY 128
#define STACK_CAPACITY 16
#define TAPE_LEN (1 << 16)

typedef int8_t i8;
typedef uint8_t u8;
typedef int32_t i32;

typedef struct {
    size_t length;
    size_t capacity;
    i8 *data;
} CharDA;

typedef struct {
    size_t size;
    size_t capacity;
    uintptr_t data[STACK_CAPACITY];
} PtrStack;

void error(const char *msg);
void open_file(FILE **file, const char* path);
void da_init(CharDA *array);
void da_append(CharDA *array, i8 val);
void stack_init(PtrStack *stack);
void stack_push(PtrStack *stack, uintptr_t ptr);
uintptr_t stack_pop(PtrStack *stack);
void read_src_to_da(FILE *file, CharDA *instructions);
bool valid_loops(CharDA *instructions);
void run(CharDA *instructions, u8 *tape);

int main(int argc, char *argv[]) {
    CharDA instructions;
    PtrStack obr, cbr; // opening/closing bracket stack
    u8 tape[TAPE_LEN] = { 0 };
    FILE *src_file = NULL;

    if (argc < 2)
        error("[ERR]: Too little arguments.\n");
    const char *path = argv[1];

    open_file(&src_file, path);
    da_init(&instructions);
    read_src_to_da(src_file, &instructions);
    stack_init(&obr);
    stack_init(&cbr);
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
        array->data[array->length] = val;
        array->length++;
        return;
    }

    array->capacity *= 2;
    array->data = realloc(array->data, array->capacity * sizeof(i8));

    if (array->data == NULL)
        error("[ERR]: Failed to allocate memory.\n");

    array->data[array->length] = val;
    array->length++;
}

// Initializes a stack
void stack_init(PtrStack *stack) {
    stack->size = 0;
    stack->capacity = STACK_CAPACITY;
    for (u8 i = 0; i < stack->capacity; i++)
        stack->data[i] = (uintptr_t) NULL;
}

// Pushes a pointer onto the stack
void stack_push(PtrStack *stack, uintptr_t ptr) {
    if (stack->size == stack->capacity)
        error("[ERR]: Stack overflow.\n");
    stack->data[stack->size] = ptr;
    stack->size++;
}

// Pops a pointer off a stack
uintptr_t stack_pop(PtrStack *stack) {
    if (stack->size == 0)
        error("[ERR]: Attemted to pop off an empty stack.\n");
    stack->size--;
    return stack->data[stack->size + 1];
}

// Writes the source code without comments to a dynamic array
void read_src_to_da(FILE *file, CharDA *instructions) {
    i8 ch;
    ch = fgetc(file);
    while (ch != EOF) {
        if (ch == '+' || ch == '-' ||
            ch == '>' || ch == '<' ||
            ch == '[' || ch == ']' ||
            ch == '.' || ch == ',')
            da_append(instructions, ch);
        ch = fgetc(file);
    }
}

// Checks validity of all [ ] pairs
bool valid_loops(CharDA *instructions) {
    i32 loop_count = 0;
    for (size_t i = 0; i < instructions->length; i++) {
        if (instructions->data[i] == '[')
            loop_count++;
        else if (instructions->data[i] == ']')
            loop_count--;
        if (loop_count < 0)
            return false;
    }

    return loop_count == 0? true : false;
}

// Moves the instruction pointer to the matching closing bracket
void mv_closing_bracket(i8 **instr_ptr) {
    size_t depth = 0;
    while (true) {
        (*instr_ptr)++;
        if (depth == 0 && **instr_ptr == ']')
            break;
        if (**instr_ptr == '[')
            depth++;
        if (**instr_ptr == ']')
            depth--;
    }
}

// Moves the instruction pointer to the matching opening bracket
void mv_opening_bracket(i8 **instr_ptr) {
    size_t depth = 0;
    while (true) {
        (*instr_ptr)--;
        if (depth == 0 && **instr_ptr == '[')
            break;
        if (**instr_ptr == ']')
            depth++;
        if (**instr_ptr == '[')
            depth--;
    }
}

// Runs the program reading from instruction array
void run(CharDA *instructions, u8 *tape_ptr) {
    i8 *instr_ptr = instructions->data;
    i8 *instr_end = instr_ptr + instructions->length;
    u8 *left_tape_bound = tape_ptr;
    u8 *right_tape_bound = tape_ptr + TAPE_LEN - 1;

    while (instr_ptr <= instr_end) {
        switch (*instr_ptr) {
            case '+': (*tape_ptr)++; break;
            case '-': (*tape_ptr)--; break;
            case '>':
                if (tape_ptr == right_tape_bound)
                    error("[ERR]: Pointer out of bounds. (>)\n");
                tape_ptr++; break;
            case '<':
                if (tape_ptr == left_tape_bound)
                    error("[ERR]: Pointer out of bounds. (<)\n");
                tape_ptr--; break;
            case '.': putchar(*tape_ptr); break;
            case ',': *tape_ptr = getchar(); break;
            case '[':
                if (*tape_ptr == 0)
                    mv_closing_bracket(&instr_ptr);
                break;
            case ']':
                if (*tape_ptr != 0)
                    mv_opening_bracket(&instr_ptr);
                break;
        } instr_ptr++;
    }
}
