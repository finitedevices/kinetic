#include <stdio.h>
#include <string.h>

#include "kinetic.h"

Kinetic* vm;
KByte mem[65536];

KByte readHandler(KWord addr) {
    return mem[addr];
}

void writeHandler(KWord addr, KByte data) {
    mem[addr] = data;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No input file specified\n");

        return 1;
    }

    FILE* fp = fopen(argv[1], "r");

    if (!fp) {
        fprintf(stderr, "Error when reading file\n");

        return 1;
    }

    memset(mem, 0, sizeof(mem));

    fseek(fp, 0, SEEK_END);

    size_t size = ftell(fp);

    fseek(fp, 0, SEEK_SET);

    if (fread(mem, sizeof(char), size, fp) != size) {
        fprintf(stderr, "Error reading file contents\n");
        fclose(fp);

        return 1;
    }

    mem[size] = '\0';

    Kinetic vm;

    kinetic_init(&vm, readHandler, writeHandler);

    while (1) {
        kinetic_step(&vm);

        printf("IP = %04x A = %04x\n", vm.currentContext->ip, vm.currentContext->a);
    }

    return 0;
}