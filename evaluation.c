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
typedef struct lval {
    int type;
    long num;

    // Error and Symbol types have some string data
    char* err;
    char* sym;

    // Count and Pointer lo a list of "lval*"
    int count;
    struct lval** cell;
} lval;

// Create enumeration of possible lval types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

// Create enumeration of possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// Construct a new pointer to a new Number lval
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct a pointer to a new Error lval
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

// Construct a pointer no a new Symbol lval
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->err, s);
    return v;
}

void lval_del(lval* v) {

    switch(v->type) {
        // Do nothing special for number type
        case LVAL_NUM: break;

        // For Err or Sym free the string data
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        // If Sexpr then delete all elements inside
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            // Also free the memory allocated to contain the pointer
            free(v->cell);
        break;
    }
    // Finally free the memory allocated for the "lval" struct itself
    free(v);
}


// A pointer to a new empty Sexpr lval
lval* lval_sexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) *  v->count);
    v->cell[v->count] = x;
    return v;
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
    // If Symbol or Number return conversion to that type
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    // If root (>) or sexpr then create empty list
    lval* x = NULL;
    if (strcmp(t->tag, ">")== 0) { x = lval_sexpr(); }
    if (strcmp(t->tag, "sexpr")) { x = lval_sexpr(); }

    // Fill this list with any valid expression contained within
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

// Forward declaration of the "lval_print" method
void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);

    for (int i = 0; i < v->count; i++) {
        // Print Value contained within
        lval_print(v->cell[i]);

        // Don't print trailing space if last element
        if (i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

// Print an "lval"
void lval_print(lval* v) {
    switch(v->type) {
        case LVAL_NUM: printf("%li", v->num); break;
        case LVAL_ERR: printf("Error: %s", v->err); break;
        case LVAL_SYM: printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    }
}

// Print an "lval" followed by a newline
void lval_println(lval* v) {
    lval_print(v); 
    putchar('\n'); 
}

// lval eval_op(lval x, char * op, lval y) {
//     // If either value is an error return it
//     if (x.type == LVAL_ERR) { return x; }
//     if (y.type == LVAL_ERR) { return y; }

//     // Otherwise do maths on the number values
//     if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
//     if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
//     if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
//     if (strcmp(op, "/") == 0) {
//         // If second operand is zero return error instead of result
//         return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
//     }
//     if (strcmp(op, "\%") ==  0) {
//         return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num);
//     }

//     return lval_err(LERR_BAD_OP);
// }

// lval eval(mpc_ast_t* t) {

//     if (strstr(t->tag, "number")) {
//         // Check if there is some error in conversion
//         errno = 0;
//         long x = strtol(t->contents, NULL, 10);
//         return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
//     }

//     char* op = t->children[1]->contents;
//     lval x = eval(t->children[2]);

//     int i = 3;
//     while (strstr(t->children[i]->tag, "expr")) {
//         x = eval_op(x, op, eval(t->children[i]));
//         i++;
//     }

//     return x;
// }
int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    mpca_lang(MPC_LANG_DEFAULT,
        "                                                     \
        number   : /-?[0-9]+/ ;                             \
        operator : '+' | '-' | '*' | '/' | '\%' ;           \
        sexpr    : '(' <expr>* ')';                         \
        expr     : <number> | <symbol> | <sexpr> ;  \
        lispy    : /^/ <expr>* /$/ ;             \
        ",
    Number, Symbol, Sexpr, Expr, Lispy);

    puts("Tosun Lisp version 0.0.0.0.3");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        char* input = readline("==> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            // lval result = eval(r.output);
            lval* x = lval_read(r.output);
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);     
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}
