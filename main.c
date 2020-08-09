#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

enum { LVAL_NUM, LVAL_ERR};
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};


typedef struct {
    int type;
    long num;
    int err;
} lval;

lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}


void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM: printf("%li\n", v.num); break;
        case LVAL_ERR:
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division By Zero!\n");
            }
            if (v.err == LERR_BAD_OP)   {
                printf("Error: Invalid Operator!\n");
            }
            if (v.err == LERR_BAD_NUM)  {
                printf("Error: Invalid Number!\n");
            }
            break;
    }
}


int number_of_leaves(mpc_ast_t* tree){
    if (tree->children_num == 0) { return 1; }

    int leaves_count = 0;
    for (int i=0; i < tree->children_num; i++) {
        leaves_count += number_of_leaves(tree->children[i]);
    }

    return leaves_count;
}


lval eval_op(lval x, char* op, lval y) {

    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) {
        return y.num == 0 ? lval_num(-x.num) : lval_num(x.num - y.num);
    }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }

    if (strcmp(op, "/") == 0) {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }

    if (strcmp(op, "%") == 0) {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num);
    }

    if (strcmp(op, "^") == 0) {
        if (y.num == 0) { return lval_num(1); }
        if (y.num == 1) { return x; }
        if (y.num < 0) {return lval_err(LERR_BAD_NUM);}

        long result = x.num;
        for (int i=2; i <= y.num; i++){
            result *= x.num;
        }

        return lval_num(result);
    }

    if (strcmp(op, "min") == 0) { return x.num <= y.num ? x : y; }
    if (strcmp(op, "max") == 0) { return x.num >= y.num ? x : y; }

    return lval_err(LERR_BAD_OP);
}


lval eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;
    lval x = eval(t->children[2]);

    if (strcmp(op, "-") == 0 && t->children_num <= 4) { return lval_num(-x.num); }

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
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
//            mpc_ast_print(mpcResult.output);
            lval_print(eval(mpcResult.output));
//            printf("%i\n", number_of_leaves(mpcResult.output));
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
