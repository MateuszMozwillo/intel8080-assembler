#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_TOKEN_LEN 32

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
    STATE_DEFAULT
} TokenizerState;

int main() {

    char *to_tokenize = "START:  MVI A, 10\n    MVI B, 25 \nSUBTRACT:  SUB B ";

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

    for (size_t i = 0; i < tokens.count; i++) {
        printf("%s\n", tokens.items[i].data);
    }

    return 0;
}
