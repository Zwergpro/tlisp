#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"


int number_of_leaves(mpc_ast_t* tree){
    if (tree->children_num == 0) { return 1; }

    int leaves_count = 0;
    for (int i=0; i < tree->children_num; i++) {
        leaves_count += number_of_leaves(tree->children[i]);
    }

    return leaves_count;
}


long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) {
        printf("%li - %li\n", x, y);
        if (y == 0) { return -x; }
        return x - y;
    }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) {
        if (y == 0) { return 1; }
        if (y == 1) { return x; }

        long result = x;
        for (int i=2; i <= y; i++){
            result *= x;
        }

        return result;
    }

    if (strcmp(op, "min") == 0) {
        if (x <= y) { return x; }
        return y;
    }

    if (strcmp(op, "max") == 0) {
        if (x >= y) { return x; }
        return y;
    }

    return 0;
}


long eval(mpc_ast_t* t) {

    /* If tagged as number return it directly. */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* The operator is always second child. */
    char* op = t->children[1]->contents;

    /* We store the third child in `x` */
    long x = eval(t->children[2]);

    if (strcmp(op, "-") == 0 && t->children_num <= 4) { return -x; }

    /* Iterate the remaining children and combining. */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        printf("%s\n", t->children[i]->tag);
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}


int main(int argc, char** argv) {

    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");


    mpca_lang(MPCA_LANG_DEFAULT,
        "number   : /-?[0-9.]+/ ;"
        "operator : /(add|+)/ | /(sub|-)/ | /(mul|*)/ | /div/ | '/' | '%' | '^' | /min/ | /max/;"
        "expr     : <number> | '(' <operator> <expr>+ ')' | '(' <expr>+ <operator> <expr>+ ')' ;"
        "lispy    : /^/ <operator> <expr>+ /$/ | /^/ <expr>+ <operator> <expr>+ /$/;",
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
            printf("%li\n", eval(mpcResult.output));
            printf("%i\n", number_of_leaves(mpcResult.output));
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
