#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>

#include "mpc.h"

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

enum { LVAL_LONG, LVAL_DOUBLE};


union Number {
    long long_num;
    double double_num;
};

typedef struct lval {
    int type;
    int num_type;
    union Number* num;

    /* Error and Symbol types have some string data */
    char* err;
    char* sym;
    /* Count and Pointer to a list of "lval*" */
    int count;
    struct lval** cell;
} lval;


lval* set_long_num(lval* v, long x) {
    v->num->long_num = x;
    v->num_type = LVAL_LONG;
    return v;
}

lval* set_double_num(lval* v, double x) {
    v->num->double_num = x;
    v->num_type = LVAL_DOUBLE;
    return v;
}


/* Construct a pointer to a new Number lval */
lval* lval_long_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = malloc(sizeof(union Number));
    return set_long_num(v, x);

}

/* Construct a pointer to a new Number lval */
lval* lval_double_num(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = malloc(sizeof(union Number));
    return set_double_num(v, x);
}


/* Construct a pointer to a new Error lval */
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

/* Construct a pointer to a new Symbol lval */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}


/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    if (strstr(t->contents, ".")){
        double double_num = strtod(t->contents, NULL);
        return errno != ERANGE ? lval_double_num(double_num) : lval_err("invalid number");
    } else {
        long long_num = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_long_num(long_num) : lval_err("invalid number");
    }
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_read(mpc_ast_t* t) {

    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* If root (>) or sexpr then create empty list */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}


void lval_del(lval* v) {

    switch (v->type) {
        /* Do nothing special for number type */
        case LVAL_NUM: break;

            /* For Err or Sym free the string data */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        /* If Sexpr OS Qexpr then delete all elements inside */
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            /* Also free the memory allocated to contain the pointers */
            free(v->cell);
            break;
    }

    /* Free the memory allocated for the "lval" struct itself */
    free(v);
}

void lval_expr_print(lval* v, char open, char close);

void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM:
            if (v->num_type == LVAL_LONG) { printf("%li", v->num->long_num); }
            if (v->num_type == LVAL_DOUBLE) { printf("%f", v->num->double_num); }
            break;
        case LVAL_ERR:   printf("Error: %s", v->err); break;
        case LVAL_SYM:   printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        /* Print Value contained within */
        lval_print(v->cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}


void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

int number_of_leaves(mpc_ast_t* tree){
    if (tree->children_num == 0) { return 1; }

    int leaves_count = 0;
    for (int i=0; i < tree->children_num; i++) {
        leaves_count += number_of_leaves(tree->children[i]);
    }

    return leaves_count;
}

lval* lval_pop(lval* v, int i) {
    /* Find the item at "i" */
    lval* x = v->cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

    /* Decrease the count of items in the list */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}


void lval_add_op(lval* x, lval* y) {
    if (x->num_type == LVAL_LONG) {x->num->long_num += y->num->long_num; }
    if (x->num_type == LVAL_DOUBLE) { x->num->double_num += y->num->double_num; }
}

void lval_sub_op(lval* x, lval* y) {
    if (x->num_type == LVAL_LONG) {x->num->long_num -= y->num->long_num; }
    if (x->num_type == LVAL_DOUBLE) { x->num->double_num -= y->num->double_num; }
}

void lval_div_op(lval* x, lval* y) {
    if (x->num_type == LVAL_LONG) {x->num->long_num /= y->num->long_num; }
    if (x->num_type == LVAL_DOUBLE) { x->num->double_num /= y->num->double_num; }
}

void lval_fmod_op(lval* x, lval* y) {
    if (x->num_type == LVAL_LONG) {x->num->long_num %= y->num->long_num; }
    if (x->num_type == LVAL_DOUBLE) { x->num->double_num = fmod(x->num->double_num, y->num->double_num); }
}

void lval_mil_op(lval* x, lval* y) {
    if (x->num_type == LVAL_LONG) {x->num->long_num *= y->num->long_num; }
    if (x->num_type == LVAL_DOUBLE) { x->num->double_num *= y->num->double_num; }
}

lval* builtin_op(lval* a, char* op) {

    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }

    /* Pop the first element */
    lval* x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        if (x->num_type == LVAL_LONG){ x->num->long_num = -x->num->long_num; }
        if (x->num_type == LVAL_DOUBLE){ x->num->double_num = -x->num->double_num; }
    }

    /* While there are still elements remaining */
    while (a->count > 0) {

        /* Pop the next element */
        lval* y = lval_pop(a, 0);

        if (x->num_type != y->num_type) {
            lval_del(x); lval_del(y);
            x = lval_err("Different types of operands!"); break;
        }

        if (strcmp(op, "+") == 0) { lval_add_op(x, y); }
        if (strcmp(op, "-") == 0) { lval_sub_op(x, y); }
        if (strcmp(op, "*") == 0) { lval_mil_op(x, y); }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division By Zero!"); break;
            }
            lval_div_op(x, y);
        }
        if (strcmp(op, "%") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division By Zero!"); break;
            }
            lval_fmod_op(x, y);
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {

    /* Evaluate Children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    /* Error Checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty Expression */
    if (v->count == 0) { return v; }

    /* Single Expression */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure First Element is Symbol */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-expression Does not start with symbol!");
    }

    /* Call builtin with operator */
    lval* result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_eval(lval* v) {
    /* Evaluate Sexpressions */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
    /* All other lval types remain the same */
    return v;
}

int main(int argc, char** argv) {

    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr  = mpc_new("sexpr");
    mpc_parser_t* Qexpr  = mpc_new("qexpr");
    mpc_parser_t* Expr   = mpc_new("expr");
    mpc_parser_t* Lispy  = mpc_new("lispy");


    mpca_lang(MPCA_LANG_DEFAULT,
        "number : /-?[0-9.]+/ ;"
        "symbol : '+' | '-' | '*' | '/' | '%' ;"
        "sexpr  : '(' <expr>* ')' ;"
        "qexpr  : '{' <expr>* '}' ;"
        "expr   : <number> | <symbol> | <sexpr> | <qexpr> ;"
        "lispy  : /^/ <expr>* /$/ ;",
        Number, Symbol, Sexpr, Qexpr, Expr, Lispy
    );


    puts("Tlisp Version 0.0.0.0.4");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* repl_input = readline("tlisp> ");
        add_history(repl_input);
//        printf("No you're a %s\n", repl_input);

        mpc_result_t mpcResult;
        if (mpc_parse("<stdin>", repl_input, Lispy, &mpcResult)) {
            lval* x = lval_eval(lval_read(mpcResult.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(mpcResult.output);
        } else {
            /* Otherwise Print the Error */
            mpc_err_print(mpcResult.error);
            mpc_err_delete(mpcResult.error);
        }

        free(repl_input);

    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}
