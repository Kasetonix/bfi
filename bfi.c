#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define INIT_SIZE 128
#define TAPE_LEN (1 << 16)

typedef int8_t i8;
typedef uint8_t u8;
typedef int32_t i32;

typedef struct {
    size_t length;
    size_t capacity;
    i8 *data;
} CharDA;

void error(const char *msg);
void da_init(CharDA *array);
void da_append(CharDA *array, i8 val);
void open_file(FILE **file, const char* path);
void read_src_to_da(FILE *file, CharDA *instructions);
bool valid_loops(CharDA *instructions);
void process_instruction(CharDA *instructions, u8 *tape, size_t *instr_ptr, size_t *tape_ptr); 
void run(CharDA *instructions, u8 *tape);

int main(int argc, char *argv[]) {
    CharDA instructions;
    u8 tape[TAPE_LEN] = { 0 };
    FILE *src_file = NULL;

    if (argc < 2)
        error("Too little arguments.\n");
    const char *path = argv[1];

    open_file(&src_file, path);
    da_init(&instructions);
    read_src_to_da(src_file, &instructions);
    fclose(src_file);

    if (!valid_loops(&instructions))
        error("Invalid loops found.\n");

    run(&instructions, tape);
    free(instructions.data);
    return 0;
}

void error(const char *msg) {
    fputs(msg, stderr);
    exit(1);
}

void da_init(CharDA *array) {
    array->length = 0;
    array->capacity = INIT_SIZE;
    array->data = malloc(array->capacity * sizeof(i8));

    if (array->data == NULL)
        error("Failed to allocate memory.");
}

void da_append(CharDA *array, i8 val) {
    if (array->length < array->capacity) {
        array->data[array->length] = val;
        array->length++;
        return;
    }
    
    array->capacity *= 2;
    array->data = realloc(array->data, array->capacity * sizeof(i8));

    if (array->data == NULL)
        error("Failed to allocate memory.");

    array->data[array->length] = val;
    array->length++;
}

void open_file(FILE **file, const char* path) {
    *file = fopen(path, "r");
    if (*file == NULL) {
        fprintf(stderr, "Couldn't open file '%s'.\n", path);
        exit(1);
    }
}

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
                    error("Pointer out of bounds. (>)\n");
                tape_ptr++; break;
            case '<': 
                if (tape_ptr == left_tape_bound)
                    error("Pointer out of bounds. (<)\n"); 
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
