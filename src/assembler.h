#pragma once

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>

#include "vector.h"

static const size_t MAX_TOKEN_LEN = 32;

typedef struct {
    char *data;
    uint8_t size;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} Tokens;

typedef enum {
    STATE_WORD,
    STATE_COMMENT,
    STATE_DEFAULT,
} TokenizerState;

typedef enum {
    OP_NONE = 0,
    OP_REG = 1,
    OP_D8 = 2,
    OP_D16 = 3,
    OP_ADDRESS = 4,
    OP_REG_D8 = 5,
    OP_REG_D16 = 6,
    OP_REG_REG = 7,
    OP_RESET_NUMBER = 8,
    OP_RP = 9,
    OP_RP_D16 = 10,
    OP_PORT = 11
} OpcodeOperandType;

typedef struct {
    uint8_t byte_len;
    uint8_t token_skip;
} InstInfo;

typedef struct {
    char* name;
    uint8_t name_size;
    uint16_t address;
} Label;

typedef struct {
    Label *items;
    size_t count;
    size_t capacity;
} Labels;

typedef struct {
    size_t len;
    uint8_t *bytes;
} ByteCode;

ByteCode generate_byte_code(char* code);
