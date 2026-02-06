#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define INIT_DA_CAPACITY 128
#define STACK_CAPACITY 32
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
    i8 *data[STACK_CAPACITY];
} PtrStack;

void error(const char *msg);
void open_file(FILE **file, const char* path);
void da_init(CharDA *array);
void da_append(CharDA *array, i8 val);
void stack_init(PtrStack *stack);
void stack_push(PtrStack *stack, i8 *ptr);
i8 *stack_pop(PtrStack *stack);
void read_src_to_da(FILE *file, CharDA *instructions);
bool valid_loops(CharDA *instructions);
i8 *find_matching_cbr(i8 *instr_ptr);
void handle_obr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr, PtrStack *cbr);
void handle_cbr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr, PtrStack *cbr);
void run(CharDA *instructions, u8 *tape);

int main(int argc, char *argv[]) {
    CharDA instructions;
    u8 tape[TAPE_LEN] = { 0 };
    FILE *src_file = NULL;

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
        stack->data[i] = NULL;
}

// Pushes a pointer onto the stack
void stack_push(PtrStack *stack, i8 *ptr) {
    if (stack->size == stack->capacity)
        error("[ERR]: Stack overflow.\n");
    stack->data[stack->size] = ptr;
    stack->size++;
}

// Pops a pointer off a stack
i8 *stack_pop(PtrStack *stack) {
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
void handle_obr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr, PtrStack *cbr) {
    if (obr->size == 0 || obr->data[obr->size - 1] != *instr_ptr)
        stack_push(obr, *instr_ptr);

    if (*tape_ptr == 0) {
        if (obr->size == cbr->size)
            *instr_ptr = cbr->data[cbr->size - 1];
        else
            *instr_ptr = find_matching_cbr(*instr_ptr);
    } else (*instr_ptr)++;
}

// Function for handling closing brackets
void handle_cbr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr, PtrStack *cbr) {
    if (cbr->size == 0 || cbr->data[cbr->size - 1] != *instr_ptr)
        stack_push(cbr, *instr_ptr);

    if (*tape_ptr != 0)
        *instr_ptr = obr->data[obr->size - 1];
    else {
        stack_pop(obr);
        stack_pop(cbr);
        (*instr_ptr)++;
    }
}

// Runs the program reading from instruction array
void run(CharDA *instructions, u8 *tape_ptr) {
    PtrStack obr, cbr; // opening/closing bracket stack
    stack_init(&obr);
    stack_init(&cbr);

    i8 *instr_ptr = instructions->data;
    i8 *instr_end = instr_ptr + instructions->length;
    u8 *left_tape_bound = tape_ptr;
    u8 *right_tape_bound = tape_ptr + TAPE_LEN - 1;

    while (instr_ptr < instr_end) {
        switch (*instr_ptr) {
            case '+': (*tape_ptr)++; instr_ptr++; break;
            case '-': (*tape_ptr)--; instr_ptr++; break;
            case '>':
                if (tape_ptr == right_tape_bound)
                    error("[ERR]: Pointer out of bounds. (>)\n");
                tape_ptr++; instr_ptr++; break;
            case '<':
                if (tape_ptr == left_tape_bound)
                    error("[ERR]: Pointer out of bounds. (<)\n");
                tape_ptr--; instr_ptr++; break;
            case '.': putchar(*tape_ptr); instr_ptr++; break;
            case ',': *tape_ptr = getchar(); instr_ptr++; break;
            case '[': handle_obr(&instr_ptr, tape_ptr, &obr, &cbr); break;
            case ']': handle_cbr(&instr_ptr, tape_ptr, &obr, &cbr); break;
        } 
    }
}
