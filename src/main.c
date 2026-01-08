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

void tokens_append_bfr(Tokens *tokens, char bfr[32], uint8_t *current_bfr_size) {
    if (tokens->count + 1 >= tokens->capacity) {
        tokens->capacity *= 2;

        Token *temp = realloc(tokens->items, sizeof(Token) * tokens->capacity);
        if (!temp) {
            fprintf(stderr, "ERROR: Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        tokens->items = temp;
    }

    bfr[*current_bfr_size] = 0;

    tokens->items[tokens->count].data = malloc(sizeof(char) * (*current_bfr_size + 1));
    memcpy(tokens->items[tokens->count].data, bfr, *current_bfr_size + 1);

    tokens->items[tokens->count].size = *current_bfr_size;

    *current_bfr_size = 0;
    tokens->count++;
}

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
    "x",       "lxi sp",   "sta",     "inx sp",  "inr m",   "dcr m",    "mvi M",   "stc",
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

Tokens tokenize(char *to_tokenize) {

    Tokens tokens;
    tokens.count = 0;
    tokens.capacity = 100;
    tokens.items = malloc(sizeof(Token) * tokens.capacity);

    size_t ptr = 0;
    char bfr[MAX_TOKEN_LEN];
    uint8_t current_bfr_size = 0;

    TokenizerState state = STATE_DEFAULT;

    while(to_tokenize[ptr]) {

        char c = to_tokenize[ptr];

        switch (state) {
            case STATE_DEFAULT: {
                if (c == ';') {
                    state = STATE_COMMENT;
                } else if (!isspace(c)) {
                    state = STATE_WORD;
                    bfr[current_bfr_size++] = c;
                }
                break;
            }
            case STATE_WORD: {
                if (c == ';') {
                    tokens_append_bfr(&tokens, bfr, &current_bfr_size);
                    state = STATE_COMMENT;
                }
                else if (isspace(c)) {
                    tokens_append_bfr(&tokens, bfr, &current_bfr_size);
                    state = STATE_DEFAULT;
                } else {
                    if (current_bfr_size < MAX_TOKEN_LEN-1) {
                        bfr[current_bfr_size++] = c;
                    } else {
                        fprintf(stderr, "ERROR: Token size exceeded limit\n");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            }
            case STATE_COMMENT: {
                if (c == '\n') {
                    state = STATE_DEFAULT;
                }
                break;
            }
        }

        ptr++;
    }

    if (state == STATE_WORD) {
        tokens_append_bfr(&tokens, bfr, &current_bfr_size);
    }

    return tokens;
}

void string_to_lower(char* in) {
    int i = 0;
    while (in[i]) {
        in[i] = tolower(in[i]);
        i++;
    }
}

bool is_token_label(Token token) {
    return token.data[token.size-1] == ':';
}

// static const OpcodeOperandType OPERAND_TYPE_TABLE[] = {
//     0, 6, 1, 1, 1, 1, 5, 0, 0, 1, 1, 1, 1, 1, 5, 0,
//     0, 6, 1, 1, 1, 1, 5, 0, 0, 1, 1, 1, 1, 1, 5, 0,
//     0, 6, 4, 1, 1, 1, 5, 0, 0, 1, 4, 1, 1, 1, 5, 0,
//     0, 6, 4, 1, 1, 1, 5, 0, 0, 1, 4, 1, 1, 1, 5, 0,
//     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
//     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
//     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
//     7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7,
//     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//     0, 1, 4, 4, 4, 1, 2, 8, 0, 0, 4, 4, 4, 4, 2, 8,
//     0, 1, 4, 2, 4, 1, 2, 8, 0, 0, 4, 2, 4, 4, 2, 8,
//     0, 1, 4, 0, 4, 1, 2, 8, 0, 0, 4, 0, 4, 4, 2, 8,
//     0, 1, 4, 0, 4, 1, 2, 8, 0, 0, 4, 0, 4, 4, 2, 8,
// };

static const char* OPCODE_TABLE_NO_OPERAND[] = {
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

// static const OpcodeOperandType OPERAND_TYPE_TABLE[] = {
//     OP_REG_REG, OP_REG_D8, OP_RP_D16, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_RP,
//     OP_RP, OP_NONE, OP_REG, OP_D8, OP_REG, OP_D8, OP_REG, OP_D8,
//     OP_REG, OP_D8, OP_D8, OP_D8, OP_RP, OP_RP, OP_RP, OP_NONE,
//     OP_REG, OP_D8, OP_REG, OP_D8, OP_REG, OP_D8, OP_REG, OP_D8,
//     OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_ADDRESS,
//     OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS,
//     OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS, OP_ADDRESS,
//     OP_ADDRESS, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE, OP_NONE,
//     OP_NONE, OP_NONE, OP_RESET_NUMBER, OP_NONE, OP_RP, OP_RP, OP_NONE, OP_NONE,
//     OP_PORT, OP_PORT, OP_NONE, OP_NONE, OP_NONE, OP_NONE

// };

static const OpcodeOperandType OPERAND_TYPE_TABLE[] = {
     7,  5, 10,  4,  4,  4,  4,  9,
     9,  0,  1,  2,  1,  2,  1,  2,
     1,  2,  2,  2,  9,  9,  9,  0,
     1,  2,  1,  2,  1,  2,  1,  2,
     0,  0,  0,  0,  0,  0,  0,  4,
     4,  4,  4,  4,  4,  4,  4,  4,
     4,  4,  4,  4,  4,  4,  4,  4,
     4,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  8,  0,  9,  9,  0,  0,
     11, 11, 0,  0,  0,  0
};

int find_opcode_no_operand(char *to_find) {
    for (size_t i = 0; i < 78; i++) {
        if (strcmp(to_find, OPCODE_TABLE_NO_OPERAND[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int find_opcode(char *to_find) {
    for (size_t i = 0; i < 256; i++) {
        if (strcmp(to_find, OPCODE_TABLE[i]) == 0) {
            return i;
        }
    }
    return -1;
}

typedef struct {
    uint8_t *bytes;
    size_t capacity;
    size_t len;
} ByteCode;

OpcodeOperandType get_operand_type(Token token) {
    char temp[MAX_TOKEN_LEN];
    strcpy(temp, token.data);

    string_to_lower(temp);

    int opcode_index = find_opcode_no_operand(temp);

    if (opcode_index == -1) {
        fprintf(stderr, "ERROR: opcode doesnt exist: %s\n", temp);
        exit(EXIT_FAILURE);
    }
    return OPERAND_TYPE_TABLE[opcode_index];
}

uint8_t token_to_val(Token *to_convert) {
    if (to_convert->data[to_convert->size-1] == 'H') {
        to_convert->data[to_convert->size-1] = '\0';
        return strtol(to_convert->data, NULL, 16);
    } else if (to_convert->data[to_convert->size-1] == 'B') {
        to_convert->data[to_convert->size-1] = '\0';
        return strtol(to_convert->data, NULL, 2);
    } else {
        return strtol(to_convert->data, NULL, 10);
    }
}

int main() {

    Tokens tokens = tokenize("LXI B,20");

    for (size_t i = 0; i < tokens.count; i++) {
        printf("%s\n", tokens.items[i].data);
    }

    vec(uint8_t) byte_code;
    vec_init(byte_code, 100);

    for (size_t i = 0; i < tokens.count; i++) {
        if (is_token_label(tokens.items[i])) {
            continue;
        }

        OpcodeOperandType opcode_operand_type = get_operand_type(tokens.items[i]);

        switch (opcode_operand_type) {
            case OP_NONE: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                string_to_lower(temp);
                vec_append(byte_code, find_opcode(temp));
                break;
            }
            case OP_REG: case OP_RESET_NUMBER: case OP_RP: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                strcat(temp, " ");
                i++;
                strcat(temp, tokens.items[i].data);
                string_to_lower(temp);
                vec_append(byte_code, find_opcode(temp));
                break;
            }
            case OP_D8: case OP_PORT: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                string_to_lower(temp);
                vec_append(byte_code, find_opcode(temp));
                i++;
                vec_append(byte_code, (uint8_t)token_to_val(&tokens.items[i]));
                break;
            }
            case OP_D16: case OP_ADDRESS: case OP_RP_D16: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                string_to_lower(temp);
                vec_append(byte_code, find_opcode(temp));
                i++;
                uint16_t value = (uint16_t)token_to_val(&tokens.items[i]);
                vec_append(byte_code, (uint8_t)value);
                vec_append(byte_code, (uint8_t)(value >> 8));
                break;
            }
            case OP_REG_D8: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                strcat(temp, " ");
                i++;
                strcat(temp, tokens.items[i].data);
                string_to_lower(temp);
                vec_append(byte_code, find_opcode(temp));
                i++;
                vec_append(byte_code, (uint8_t)token_to_val(&tokens.items[i]));
                break;
            }
            case OP_REG_D16: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                strcat(temp, " ");
                i++;
                strcat(temp, tokens.items[i].data);
                string_to_lower(temp);
                vec_append(byte_code, find_opcode(temp));
                i++;
                uint16_t value = (uint16_t)token_to_val(&tokens.items[i]);
                vec_append(byte_code, (uint8_t)value);
                vec_append(byte_code, (uint8_t)(value >> 8));
                break;
            }
            case OP_REG_REG: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                strcat(temp, " ");
                i++;
                strcat(temp, tokens.items[i].data);
                strcat(temp, ",");
                i++;
                strcat(temp, tokens.items[i].data);
                string_to_lower(temp);
                vec_append(byte_code, find_opcode(temp));
                break;
            }
        }
    }

    for (size_t i = 0; i < byte_code.len; i++) {
        printf("0x%x\n", byte_code.data[i]);
    }

    free(tokens.items);
    vec_cleanup(byte_code);

    return 0;
}
