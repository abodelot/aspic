#include "repl.h"
#include "vm.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Load file content into a string buffer
 * @return new string buffer
 */
static char* read_file(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "aspic: Cannot open %s (%s)\n", path, strerror(errno));
        exit(1);
    }

    // Get file size
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    // Allocate buffer
    char* buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "aspic: Memory error while reading %s\n", path);
        exit(1);
    }

    // Read the whole file
    size_t read_size = fread(buffer, sizeof(char), file_size, file);
    buffer[read_size] = '\0';
    if (read_size < file_size) {
        fprintf(stderr, "aspic: Cannot read %s\n", path);
        exit(1);
    }

    fclose(file);
    return buffer;
}

int main(int argc, const char* argv[])
{
    vm_init();
    VmResult result = VM_OK;
    if (argc == 1) {
        // No argument: start interactive prompt
        repl();
    } else if (argc == 2 && argv[1][0] != '-') {
        // Source passed as filename
        char* source = read_file(argv[1]);
        result = vm_interpret(source);
        free(source);
    } else if (strcmp(argv[1], "-c") == 0) {
        // Source passed as command argument
        if (argc > 2) {
            result = vm_interpret(argv[2]);
        } else {
            fprintf(stderr, "Missing argument for -c\n");
        }
    } else if (strcmp(argv[1], "-v") == 0) {
        // Print version
        printf("Aspic " ASPIC_VERSION_STRING " (Built " __DATE__ ", " __TIME__ ")\n");
    } else {
        // Print usage
        fprintf(stderr, "Unknown option %s\n", argv[1]);
        fprintf(stderr, "Usage: %s <path> ", argv[0]);
        fprintf(stderr, "Usage: %s -c <command>", argv[0]);
    }

    vm_free();
    return result == VM_OK ? 0 : 1;
}
