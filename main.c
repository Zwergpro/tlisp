#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"




int main(int argc, char** argv) {

    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");


    mpca_lang(MPCA_LANG_DEFAULT,
"                                                                                            \
            number   : /-?[0-9.]+/ ;                                                                 \
            operator : /(add|+)/ | /(sub|-)/ | /(mul|*)/ | /div/ | '/' | '%';                        \
            expr     : <number> | '(' <operator> <expr>+ ')' | '(' <expr>+ <operator> <expr>+ ')' ;  \
            lispy    : /^/ <operator> <expr>+ /$/ | /^/ <expr>+ <operator> <expr>+ /$/;              \
        ",
        Number, Operator, Expr, Lispy
    );


    puts("Tlisp Version 0.0.0.0.3");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* repl_input = readline("tlisp> ");
        add_history(repl_input);
//        printf("No you're a %s\n", repl_input);

        mpc_result_t mpcResult;
        if (mpc_parse("<stdin>", repl_input, Lispy, &mpcResult)) {
            /* On Success Print the AST */
            mpc_ast_print(mpcResult.output);
            mpc_ast_delete(mpcResult.output);
        } else {
            /* Otherwise Print the Error */
            mpc_err_print(mpcResult.error);
            mpc_err_delete(mpcResult.error);
        }

        free(repl_input);

    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return 0;
}
