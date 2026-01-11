#include "assembler.h"

int main() {

    ByteCode assembled = generate_byte_code("START: MVI A, 10");

    for (size_t i = 0; i < assembled.len; i++) {
        printf("0x%x\n", assembled.bytes[i]);
    }

    return 0;
}
