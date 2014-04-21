#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

#else

#include <editline/readline.h>

#endif

// Declare new lval struct
typedef struct {
    int type;
    long num;
    int err;
} lval;

// Create enumeration of possible lval types
enum { LVAL_NUM, LVAL_ERR };

// Create enumeration of possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// Create a new number type lval
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

// Create a new error type lval
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

// Print an "lval"
void lval_print(lval v) {
    switch(v.type) {
        // In the case the type is a number print it, then 'break'
        // of the case
        case LVAL_NUM: printf("%li", v.num); break;

        // In the case the type is an error
        case LVAL_ERR:
            if (v.err == LERR_DIV_ZERO) { printf("Error: Division by Zero!"); }
            if (v.err == LERR_BAD_OP) { printf("Error: Invalid operator!"); }
            if (v.err == LERR_BAD_NUM) { printf("Error: Invalid number!"); }
        break;
    }
}

// Print an "lval" followed by a newline
void lval_println(lval v) {
    lval_print(v); 
    putchar('\n'); 
}

// Use operator string to see which operation to perform
long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    return 0;
}

long eval(mpc_ast_t* t) {
    // If tagged as number return it directly, otherwise expression
    if (strstr(t->tag, "number")) { return atoi(t->contents); }

    // The operator is always the second child
    char* op = t->children[1]->contents;

    // We store the third child in `x`
    long x = eval(t->children[2]);

    // Iterate the remaining children, combining using our operator
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    mpca_lang(MPC_LANG_DEFAULT,
        "                                                     \
        number   : /-?[0-9]+/ ;                             \
        operator : '+' | '-' | '*' | '/' ;                  \
        expr     : <number> | '(' <operator> <expr>+ ')' ;  \
        lispy    : /^/ <operator> <expr>+ /$/ ;             \
        ",
    Number, Operator, Expr, Lispy);

    puts("Tosun Lisp version 0.0.0.0.3");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* input = readline("==> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            long result = eval(r.output);
            printf("%li\n", result);
            mpc_ast_delete(r.output);     
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}
