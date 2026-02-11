#ifndef BFI_H
#define BFI_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define INIT_DA_CAPACITY 128
#define STACK_CAPACITY 64
#define TAPE_LEN (1 << 16)
#define TAPE_BEG_CHARS 64

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
i8 *stack_top(PtrStack *stack);
void stack_push(PtrStack *stack, i8 *ptr);
i8 *stack_pop(PtrStack *stack);
void read_src_to_da(FILE *file, CharDA *instructions);
bool valid_loops(CharDA *instructions);
void print_cc(u8 code);
void print_tape(u8 *tape_left_bound, u8 *tape_ptr, u8 lpc);
i8 *find_matching_cbr(i8 *instr_ptr);
void handle_obr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr);
void handle_cbr(i8 **instr_ptr, u8 *tape_ptr, PtrStack *obr, PtrStack *cbr);
void run(CharDA *instructions, u8 *tape);

#endif // BFI_H
