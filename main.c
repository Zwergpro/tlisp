#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>


int main(int argc, char** argv) {
    puts("Tlisp Version 0.0.0.0.2");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* repl_input = readline("tlisp> ");
        add_history(repl_input);
        printf("No you're a %s\n", repl_input);
        free(repl_input);
    }

    return 0;
}
