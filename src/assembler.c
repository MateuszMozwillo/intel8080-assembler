#include "assembler.h"
#include "vector.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void* xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "FATAL ERROR: Out of memory (malloc failed for %zu bytes)\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* xrealloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL && size > 0) {
        fprintf(stderr, "FATAL ERROR: Out of memory (realloc failed for %zu bytes)\n", size);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

void string_to_lower(char* in) {
    for (size_t i = 0; in[i]; i++) in[i] = tolower(in[i]);
}

void tokens_append_bfr(Tokens *tokens, char bfr[32], uint8_t *current_bfr_size) {
    if (tokens->count + 1 >= tokens->capacity) {
        tokens->capacity *= 2;

        tokens->items = xrealloc(tokens->items, sizeof(Token) * tokens->capacity);;
    }

    bfr[*current_bfr_size] = 0;

    tokens->items[tokens->count].data = xmalloc(sizeof(char) * (*current_bfr_size + 1));
    memcpy(tokens->items[tokens->count].data, bfr, *current_bfr_size + 1);

    string_to_lower(tokens->items[tokens->count].data);

    tokens->items[tokens->count].size = *current_bfr_size;

    *current_bfr_size = 0;
    tokens->count++;
}

void free_tokens(Tokens *tokens) {
    for (size_t i = 0; i < tokens->count; i++) {
        free(tokens->items[i].data);
    }
    free(tokens->items);
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}

bool is_token_label(Token token) {
    return token.data[token.size-1] == ':';
}

int find_instruction_no_operand(char *to_find) {
    for (size_t i = 0; i < 78; i++) {
        if (strcmp(to_find, INSTRUCTION_TABLE_NO_OPERAND[i]) == 0) {
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

OpcodeOperandType get_operand_type(Token token) {
    char temp[MAX_TOKEN_LEN];
    strcpy(temp, token.data);

    string_to_lower(temp);

    int instruction_index = find_instruction_no_operand(temp);

    if (instruction_index == -1) {
        fprintf(stderr, "ERROR: instruction doesnt exist: %s\n", temp);
        exit(EXIT_FAILURE);
    }
    return OPERAND_TYPE_TABLE[instruction_index];
}

InstInfo get_inst_info(OpcodeOperandType type) {
    switch (type) {
        case OP_NONE:
            return (InstInfo){1, 0};
        case OP_REG:
        case OP_RESET_NUMBER:
        case OP_RP:
            return (InstInfo){1, 1};
        case OP_D8:
        case OP_PORT:
            return (InstInfo){2, 1};
        case OP_REG_D8:
            return (InstInfo){2, 2};
        case OP_D16:
        case OP_ADDRESS:
        case OP_RP_D16:
            return (InstInfo){3, 1};
        case OP_REG_D16:
            return (InstInfo){3, 2};
        case OP_REG_REG:
            return (InstInfo){1, 2};
        default:
            return (InstInfo){0, 0};
    }
}

void labels_append(Labels *labels, Label *label) {
    if (labels->count + 1 >= labels->capacity) {
        labels->capacity *= 2;

        labels->items = xrealloc(labels->items, sizeof(Label) * labels->capacity);;
    }

    labels->items[labels->count++] = *label;
}

Labels find_labels(Tokens tokens) {
    size_t current_byte = 0;

    Labels labels;
    labels.count = 0;
    labels.capacity = 100;
    labels.items = xmalloc(sizeof(Label) * labels.capacity);

    for (size_t i = 0; i < tokens.count; i++) {
        if (is_token_label(tokens.items[i])) {
            Label label;
            label.name_size = tokens.items[i].size-1;
            label.name = xmalloc(label.name_size);
            memcpy(label.name, tokens.items[i].data, tokens.items[i].size-1);
            label.name[label.name_size] = 0;
            label.address = current_byte;
            labels_append(&labels, &label);
        } else if (strcmp(tokens.items[i].data, "db") == 0) {
            current_byte += 1;
            i++;
        }
        else if (strcmp(tokens.items[i].data, "dw") == 0) {
            current_byte += 2;
            i++;
        } else {
            InstInfo inst_info = get_inst_info(get_operand_type(tokens.items[i]));
            i += inst_info.token_skip;
            current_byte += inst_info.byte_len;
        }
    }

    return labels;
}

void labels_free(Labels *labels) {
    for (size_t i = 0; i < labels->count; i++) {
        free(labels->items[i].name);
    }
    free(labels->items);
    labels->items = NULL;
    labels->count = 0;
    labels->capacity = 0;
}

uint16_t get_value_from_label(Labels labels, char* label_name) {
    for (size_t i = 0; i < labels.count; i++) {
        if (strcmp(labels.items[i].name, label_name) == 0) {
            return labels.items[i].address;
        }
    }
    printf("%s, %s\n", label_name, labels.items[0].name);
    fprintf(stderr, "ERROR: Label doesnt exist");
    exit(EXIT_FAILURE);
}

uint16_t token_to_val(Labels labels, Token *to_convert) {
    if (to_convert->data[to_convert->size-1] == 'h') {
        to_convert->data[to_convert->size-1] = '\0';
        return strtol(to_convert->data, NULL, 16);
    } else if (to_convert->data[to_convert->size-1] == 'b') {
        to_convert->data[to_convert->size-1] = '\0';
        return strtol(to_convert->data, NULL, 2);
    } else if (!isdigit(to_convert->data[0])) {
        return get_value_from_label(labels, to_convert->data);
    } else {
        return strtol(to_convert->data, NULL, 10);
    }
}

Tokens tokenize(char *to_tokenize) {

    Tokens tokens;
    tokens.count = 0;
    tokens.capacity = 100;
    tokens.items = xmalloc(sizeof(Token) * tokens.capacity);

    size_t ptr = 0;
    char bfr[MAX_TOKEN_LEN];
    uint8_t current_bfr_size = 0;

    TokenizerState state = STATE_DEFAULT;

    uint16_t current_byte;

    while(to_tokenize[ptr]) {

        char c = to_tokenize[ptr];

        switch (state) {
            case STATE_DEFAULT: {
                if (c == ';') {
                    state = STATE_COMMENT;
                } else if (!isspace(c) && c != ',') {
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
                else if (isspace(c) || c == ',') {
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

ByteCode generate_byte_code(char* code) {

    Tokens tokens = tokenize(code);
    Labels labels = find_labels(tokens);

    vec(uint8_t) bytes;
    vec_init(bytes, 100);

    for (size_t i = 0; i < tokens.count; i++) {
        if (is_token_label(tokens.items[i])) {
            continue;
        }

        if (strcmp(tokens.items[i].data, "db") == 0) {

            vec_append(bytes, (uint8_t)token_to_val(labels, &tokens.items[++i]));

            continue;
        }

        if (strcmp(tokens.items[i].data, "dw") == 0) {

            i++;
            uint16_t value = (uint16_t)token_to_val(labels, &tokens.items[i]);
            vec_append(bytes, (uint8_t)value);
            vec_append(bytes, (uint8_t)(value >> 8));

            continue;
        }

        OpcodeOperandType opcode_operand_type = get_operand_type(tokens.items[i]);

        switch (opcode_operand_type) {
            case OP_NONE: {
                vec_append(bytes, find_opcode(tokens.items[i].data));
                break;
            }
            case OP_REG: case OP_RESET_NUMBER: case OP_RP: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                strcat(temp, " ");
                i++;
                strcat(temp, tokens.items[i].data);
                vec_append(bytes, find_opcode(temp));
                break;
            }
            case OP_D8: case OP_PORT: {
                vec_append(bytes, find_opcode(tokens.items[i].data));
                i++;
                vec_append(bytes, (uint8_t)token_to_val(labels, &tokens.items[i]));
                break;
            }
            case OP_D16: case OP_ADDRESS: case OP_RP_D16: {
                vec_append(bytes, find_opcode(tokens.items[i].data));
                i++;
                uint16_t value = (uint16_t)token_to_val(labels, &tokens.items[i]);
                vec_append(bytes, (uint8_t)value);
                vec_append(bytes, (uint8_t)(value >> 8));
                break;
            }
            case OP_REG_D8: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                strcat(temp, " ");
                i++;
                strcat(temp, tokens.items[i].data);
                vec_append(bytes, find_opcode(temp));
                i++;
                vec_append(bytes, (uint8_t)token_to_val(labels, &tokens.items[i]));
                break;
            }
            case OP_REG_D16: {
                char temp[MAX_TOKEN_LEN];
                strcpy(temp, tokens.items[i].data);
                strcat(temp, " ");
                i++;
                strcat(temp, tokens.items[i].data);
                vec_append(bytes, find_opcode(temp));
                i++;
                uint16_t value = (uint16_t)token_to_val(labels, &tokens.items[i]);
                vec_append(bytes, (uint8_t)value);
                vec_append(bytes, (uint8_t)(value >> 8));
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
                vec_append(bytes, find_opcode(temp));
                break;
            }
        }
    }

    free_tokens(&tokens);
    labels_free(&labels);

    ByteCode byte_code;
    byte_code.bytes = bytes.data;
    byte_code.len = bytes.len;

    return byte_code;
}
