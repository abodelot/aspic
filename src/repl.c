#include "repl.h"
#include "shared.h"
#include "vm.h"

#include <stdio.h>

#include <readline/history.h>
#include <readline/readline.h>

void repl()
{
    printf("Aspic " ASPIC_VERSION_STRING " (Built " __DATE__ ", " __TIME__ ")\n");
    printf("  * exit: exit current session\n"
           "  * strings: print list of interned strings\n"
           "  * globals: print list of global identifiers\n");

    // Configure readline to insert tabs (instead of PATH completion)
    rl_bind_key('\t', rl_insert);

    bool running = true;
    char* line = NULL;
    const char* prompt = ">> ";

    while (running && (line = readline(prompt)) != NULL) {
        if (strlen(line) > 0) {
            add_history(line);
        }

        if (strcmp(line, "exit") == 0) {
            running = false;
        } else if (strcmp(line, "strings") == 0) {
            vm_debug_strings();
        } else if (strcmp(line, "globals") == 0) {
            vm_debug_globals();
        } else {
            if (vm_interpret(line) == VM_OK) {
                Value value = vm_last_value();
                if (value.type != TYPE_NULL) {
                    value_repr(value);
                    printf("\n");
                }
            }
        }

        free(line);
    }
}
