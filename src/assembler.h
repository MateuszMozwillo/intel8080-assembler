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

static const char* OPCODE_TABLE[] = {
    "nop",     "lxi b",    "stax b",  "inx b",   "inr b",   "dcr b",    "mvi b",   "rlc",
    "x",       "dad b",    "ldax b",  "dcx b",   "inr c",   "dcr c",    "mvi c",   "rrc",
    "x",       "lxi d",    "stax d",  "inx d",   "inr d",   "dcr d",    "mvi d",   "ral",
    "x",       "dad d",    "ldax d",  "dcx d",   "inr e",   "dcr e",    "mvi e",   "rar",
    "x",       "lxi h",    "shld",    "inx h",   "inr h",   "dcr h",    "mvi h",   "daa",
    "x",       "dad h",    "lhld",    "dcx h",   "inr l",   "dcr l",    "mvi l",   "cma",
    "x",       "lxi sp",   "sta",     "inx sp",  "inr m",   "dcr m",    "mvi m",   "stc",
    "x",       "dad sp",   "lda",     "dcx sp",  "inr a",   "dcr a",    "mvi a",   "cmc",
    "mov b,b", "mov b,c",  "mov b,d", "mov b,e", "mov b,h", "mov b,l",  "mov b,m", "mov b,a",
    "mov c,b", "mov c,c",  "mov c,d", "mov c,e", "mov c,h", "mov c,l",  "mov c,m", "mov c,a",
    "mov d,b", "mov d,c",  "mov d,d", "mov d,e", "mov d,h", "mov d,l",  "mov d,m", "mov d,a",
    "mov e,b", "mov e,c",  "mov e,d", "mov e,e", "mov e,h", "mov e,l",  "mov e,m", "mov e,a",
    "mov h,b", "mov h,c",  "mov h,d", "mov h,e", "mov h,h", "mov h,l",  "mov h,m", "mov h,a",
    "mov l,b", "mov l,c",  "mov l,d", "mov l,e", "mov l,h", "mov l,l",  "mov l,m", "mov l,a",
    "mov m,b", "mov m,c",  "mov m,d", "mov m,e", "mov m,h", "mov m,l",  "hlt",     "mov m,a",
    "mov a,b", "mov a,c",  "mov a,d", "mov a,e", "mov a,h", "mov a,l",  "mov a,m", "mov a,a",
    "add b",   "add c",    "add d",   "add e",   "add h",   "add l",    "add m",   "add a",
    "adc b",   "adc c",    "adc d",   "adc e",   "adc h",   "adc l",    "adc m",   "adc a",
    "sub b",   "sub c",    "sub d",   "sub e",   "sub h",   "sub l",    "sub m",   "sub a",
    "sbb b",   "sbb c",    "sbb d",   "sbb e",   "sbb h",   "sbb l",    "sbb m",   "sbb a",
    "ana b",   "ana c",    "ana d",   "ana e",   "ana h",   "ana l",    "ana m",   "ana a",
    "xra b",   "xra c",    "xra d",   "xra e",   "xra h",   "xra l",    "xra m",   "xra a",
    "ora b",   "ora c",    "ora d",   "ora e",   "ora h",   "ora l",    "ora m",   "ora a",
    "cmp b",   "cmp c",    "cmp d",   "cmp e",   "cmp h",   "cmp l",    "cmp m",   "cmp a",
    "rnz",     "pop b",    "jnz",     "jmp",     "cnz",     "push b",   "adi",     "rst 0",
    "rz",      "ret",      "jz",      "x",       "cz",      "call",     "aci",     "rst 1",
    "rnc",     "pop d",    "jnc",     "out p",   "cnc",     "push d",   "sui",     "rst 2",
    "rc",      "x",        "jc",      "in p",    "cc",      "x",        "sbi",     "rst 3",
    "rpo",     "pop h",    "jpo",     "xthl",    "cpo",     "push h",   "ani",     "rst 4",
    "rpe",     "pchl",     "jpe",     "xchg",    "cpe",     "x",        "xri",     "rst 5",
    "rp",      "pop psw",  "jp",      "di",      "cp",      "push psw", "ori",     "rst 6",
    "rm",      "sphl",     "jm",      "ei",      "cm",      "x",        "cpi",     "rst 7"
};

static const char* INSTRUCTION_TABLE_NO_OPERAND[] = {
    "mov",  "mvi",  "lxi", "lda",  "sta",  "lhld", "shld", "ldax",
    "stax", "xchg", "add", "adi",  "adc",  "aci",  "sub",  "sui",
    "sbb",  "sbi",  "inr", "dcr",  "inx",  "dcx",  "dad",  "daa",
    "ana",  "ani",  "ora", "ori",  "xra",  "xri",  "cmp",  "cpi",
    "rlc",  "rrc",  "ral", "rar",  "cma",  "cmc",  "stc",  "jmp",
    "jnz",  "jnc",  "jpo", "jp",   "jz",   "jc",   "jpe",  "jm",
    "call", "cnz",  "cnc", "cpo",  "cp",   "cz",   "cc",   "cpe",
    "cm",   "ret",  "rnz", "rnc",  "rpo",  "rp",   "rz",   "rc",
    "rpe",  "rm",   "rst", "pchl", "push", "pop",  "xthl", "sphl",
    "in",   "out",  "ei",  "di",   "hlt",  "nop"
};

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

static const OpcodeOperandType OPERAND_TYPE_TABLE[] = {
    OP_REG_REG, OP_REG_D8,  OP_RP_D16,       OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_RP,
    OP_RP,      OP_NONE,    OP_REG,          OP_D8,      OP_REG,     OP_D8,      OP_REG,     OP_D8,
    OP_REG,     OP_D8,      OP_D8,           OP_D8,      OP_RP,      OP_RP,      OP_RP,      OP_NONE,
    OP_REG,     OP_D8,      OP_REG,          OP_D8,      OP_REG,     OP_D8,      OP_REG,     OP_D8,
    OP_NONE,    OP_NONE,    OP_NONE,         OP_NONE,    OP_NONE,    OP_NONE,    OP_NONE,    OP_ADDRESS,
    OP_ADDRESS, OP_ADDRESS, OP_ADDRESS,      OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS,
    OP_ADDRESS, OP_ADDRESS, OP_ADDRESS,      OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS,
    OP_ADDRESS, OP_NONE,    OP_NONE,         OP_NONE,    OP_NONE,    OP_NONE,    OP_NONE,    OP_NONE,
    OP_NONE,    OP_NONE,    OP_RESET_NUMBER, OP_NONE,    OP_RP,      OP_RP,      OP_NONE,    OP_NONE,
    OP_PORT,    OP_PORT,    OP_NONE,         OP_NONE,    OP_NONE,    OP_NONE
};

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
