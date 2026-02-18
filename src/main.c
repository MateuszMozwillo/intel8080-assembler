#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int save_bytecode(const char *filename, ByteCode *bc) {
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;

    fwrite(&bc->len, sizeof(size_t), 1, f);

    if (bc->len > 0 && bc->bytes != NULL) {
        fwrite(bc->bytes, 1, bc->len, f);
    }

    fclose(f);
    return 1;
}

#define MAX_FILENAME_LEN 20
int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("invalid argument count specify input and output file");
        exit(EXIT_FAILURE);
    }

    char input_filename[MAX_FILENAME_LEN];
    char output_filename[MAX_FILENAME_LEN];

    if (strlen(argv[1]) > MAX_FILENAME_LEN || strlen(argv[2]) > MAX_FILENAME_LEN) {
        printf("filename too long");
        exit(EXIT_FAILURE);
    }

    strcpy(input_filename, argv[1]);
    strcpy(output_filename, argv[2]);

    char *bfr = 0;
    size_t file_length;
    FILE *f = fopen(input_filename, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);
        file_length = ftell(f);
        fseek(f, 0, SEEK_SET);

        bfr = malloc(file_length + 1);
        if (bfr) {
            fread(bfr, 1, file_length, f);
            bfr[file_length] = '\0';
        }

        fclose(f);
    }

    ByteCode assembled = generate_byte_code(bfr);

    for (size_t i = 0; i < assembled.len; i++) {
        printf("0x%x\n", assembled.bytes[i]);
    }

    save_bytecode(output_filename, &assembled);

    free(assembled.bytes);
    free(bfr);

    return 0;
}
