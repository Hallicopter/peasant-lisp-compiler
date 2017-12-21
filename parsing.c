#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @file prompt.c 
 * @author Advait Raykar 
 * @date  Mon Dec 11 22:30:02 IST 2017
 * @brief Promp file for the 'Peasant' dialect of lisp.
 */

/*Preprocessors for when run on a Windows machine*/
#ifdef _WIN32
#include <string.h>

static char buffer[2048];


/*Function to substitute the readline function in windows*/
char *readline(char *prompt){
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char *cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(buffer)-1]='\0';
	return cpy;
}

/*Fake add_history funtion for windows*/
void add_history(char *dummy){}

/*Otherwise include the following for nix systems*/
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

#define LASSERT(args, cond, err) \
	if(!(cond)) {lval_del(args); return lval_err(err); }

/* Forward declerations */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


/*Enumeration for the possible lval types*/
enum {LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN};

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lenv{
	int count;
	char **syms;
	lval **vals;
};

/* This is the data structure that is going to be used
 * for storing all the input expressions.
 * The 'type' variable stores the type of data it is holding.
 * The 'num' variable holds a number if the type is a number.
 * The 'err' variable wil hold a string representing an error.
 * The 'sym' variable will hold a symbol,
 * The count will hold the the length of the cell array.
 */
struct lval{
	int type;
	double num;
	char *err;
	char *sym;
	lbuiltin fun;
	int count;
	lval** cell;
};

void lval_del(lval *v);
lenv* lenv_new(void){
	lenv *e = malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;

	return e;
}

void lenv_del(lenv *e){
	for(int i=0;i<e->count; i++){
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}

	free(e->syms);
	free(e->vals);
	free(e);
}
lval* lval_copy(lval* v);
lval* lval_err(char* m);
lval *lenv_get(lenv *e, lval *k){
	/* Iterate over all the variables in the environment */
	for(int i=0; i<e->count; i++){
		/* Return value if match is found */
		if(strcmp(e->syms[i], k->sym) == 0){
			return lval_copy(e->vals[i]);
		}
	}

	/* If symbol not found return error */
	return lval_err("Unbound err0r.");
}

void lenv_put(lenv *e, lval *k, lval *v){
	for(int i=0; i<e->count; i++){
		if(strcmp(e->syms[i], k->sym)==0){
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	/* If no matching variable is found, allocate space for a new one */
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval *) * e->count);
	e->syms = realloc(e->syms, sizeof(lval *) * e->count);

	/* Copy the contents of the lval and the symbol string into the new location */
	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(k->sym)+1);
	strcpy(e->syms[e->count-1], k->sym);
}

// Deprecated.
// Enumeration for the possible errors that can occur*/
// enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};

/*Create a new number type lval*/
lval* lval_num(double x){
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/*Create a new error type lval*/
lval* lval_err(char* m){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(sizeof(strlen(m) + 1));
	strcpy(v->err, m);

	return v;
}

/* Construct a pointer to new Symbol lval */
lval* lval_sym(char* s){
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym 	= malloc(sizeof(strlen(s)+1));
       	strcpy(v->sym, s);

	return v;	
}

/* A pointer to a new S-expression */
lval* lval_sexpr(void){
	lval* v  = malloc(sizeof(lval));
	v->type  = LVAL_SEXPR;
	v->count = 0;
	v->cell  = NULL;
	return v;
}

/* Pointer to a new Q-Expression */
lval* lval_qexpr(void){
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;

	return v;
}

/* Constructor for functions */
lval* lval_fun(lbuiltin func){
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->fun = func;
	return v;
}

/* Clean deletions for fun and profits */
void lval_del(lval* v){
	switch(v->type){
		case LVAL_NUM: break;
		case LVAL_FUN: break;
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		
		/* For Qexpr and Sexpr delete all the cells recursively */
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			for(int i=0; i< v->count; i++){
				lval_del(v->cell[i]);       
			}
		free(v->cell);
		break;
	}
	free(v);
}


/* I know this funtion is kind of of unsafe, but I like to live on the edge*/
lval* lval_read_num(mpc_ast_t* t){
	double x = atof(t->contents);
	return lval_num(x);
}

lval* lval_add(lval* v, lval* x);
lval* lval_read(mpc_ast_t* t){
	/* If Symbol or Number return an lval of that type */
	if(strstr(t->tag, "number")){
		return lval_read_num(t);
	}

	if(strstr(t->tag, "symbol")){	
		return lval_sym(t->contents);
	}

	/* create an empty list if root(>) or sexpr */
	lval* x = NULL;
	if(strcmp(t->tag, ">") == 0){
		x = lval_sexpr();
	}
	if(strstr(t->tag, "sexpr")){
		x = lval_sexpr();
	}
	if(strstr(t->tag, "qexpr")) {
		x = lval_qexpr();
	}

	/* Fill the list with valid expression contained within */
	for(int i=0; i < t->children_num; i++){
		if(strcmp(t->children[i]->contents, "(") == 0) {continue;}
		if(strcmp(t->children[i]->contents, ")") == 0) {continue;}
		if(strcmp(t->children[i]->contents, "{") == 0) {continue;}
		if(strcmp(t->children[i]->contents, "}") == 0) {continue;}
		if(strcmp(t->children[i]->tag,  "regex") == 0) {continue;}

		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

lval* lval_add(lval* v, lval* x){
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*)*v->count);
	v->cell[v->count - 1] = x;
	return v;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close){
	putchar(open);
	for(int i=0; i<v->count; i++){
		lval_print(v->cell[i]);

		if(i!= (v->count-1)){
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v){
	switch(v->type){
		case LVAL_NUM	: printf("%.3f", v->num); break;
		case LVAL_ERR	: printf("Error: %s", v->err); break;
		case LVAL_SYM	: printf("%s", v->sym); break;
		case LVAL_SEXPR	: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR	: lval_expr_print(v, '{', '}'); break;
		case LVAL_FUN	: printf("<function>"); break; 
		default: printf("I don't know why, but err0r.\n"); break;
	}
}

void lval_println(lval *v){lval_print(v); putchar('\n');}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^DEPRECATED CODE^^^^^^^^^^^^^^^^^^^^^^^^
///* Helper function for the above eval() function*/
//lval eval_op(lval x, char *op, lval y){
//	/* Return value if there is an error */
//	if(x.type == LVAL_ERR) {return x;}
//	if(y.type == LVAL_ERR) {return y;}
//
//	/* Continue with normal evalutaion if both
//	 * inputs are sane */	
//	if(strcmp(op, "+") == 0) {return lval_num(x.num + y.num);}
//	if(strcmp(op, "-") == 0) {return lval_num(x.num - y.num);}
//	if(strcmp(op, "*") == 0) {return lval_num(x.num * y.num);}
//	if(strcmp(op, "%") == 0) {return lval_num((int)x.num % (int)y.num);}
//	if(strcmp(op, "^") == 0) {int res = x.num; while(--y.num) res*=x.num; return lval_num(res);}
//	if(strcmp(op, "min") == 0) {return x.num < y.num ?x : y;}
//	if(strcmp(op, "max") == 0) {return x.num > y.num?x:y;}
//	if(strcmp(op, "/") == 0){
//		return y.num==0?lval_err(LERR_DIV_ZERO):lval_num(x.num/y.num);
//	}
//
//	return lval_err(LERR_BAD_OP);
//}


//lval eval(mpc_ast_t* t){
//	/*Base case: If tagged as a number, return the number as it is.*/
//	if(strstr(t->tag, "number")){
//		return lval_num(atof(t->contents));	
//	}
//
//	/*The operator by definitions given below will always be the second child*/
//	char *op = t->children[1]->contents;
//
//	/* Sore the third child in var x */
//	lval x = eval(t->children[2]);
//
//	/* Iterate through the remaining children */
//	int i = 3;
//	while(strstr(t->children[i]->tag, "expr")){
//		x = eval_op(x, op, eval(t->children[i]));
//		i++;
//	}
//
//	return x;
//}
/*^^^^^^^^^^^^^^^^^^^^END OF DEPRECATED CODE^^^^^^^^^^^^^^^^^^^^*/

lval* lval_eval(lenv *e, lval* v);

lval* lval_pop(lval* v, int i){
	/* First find the item at i */
	lval *x = v->cell[i];

	/* Sweet memory shifting over the previous positions */
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * v->count-1-i);
	
	/* Decrease the value of count */
	v->count--;
	
	/* Reallocate the memory used */
	v->cell = realloc(v->cell, sizeof(lval*)*v->count);
	return x;
}

lval* lval_take(lval *v, int i){
	lval *x= lval_pop(v, i);
	lval_del(v);
	return x;
}

lval* lval_copy(lval* v){
	lval *x = malloc(sizeof(lval));
	x->type = v->type;

	switch(v->type){
		/* Copy function and numbers directly */
		case LVAL_FUN: x->fun = v->fun;
			       break;
		case LVAL_NUM: x->num = v->num;
			       break;
		/* Copy strings using malloc and strcpy */
		case LVAL_ERR: x->err = malloc(strlen(v->err)+1);
			       strcpy(x->err, v->err);
			       break;
		case LVAL_SYM: x->sym = malloc(strlen(v->sym)+1);
			       strcpy(x->sym, v->sym);
			       break;
		/* Copy lists by copyting each subexpression */
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->count = v->count;
			x->cell  = malloc(sizeof(lval *) * x->count);
		       	for(int i = 0; i<x->count; i++){
				x->cell[i] = lval_copy(v->cell[i]);
			}
			break;
	}

	return x;
}

lval *builtin_head(lenv *e, lval *a){
	/* Check the error condition */
	LASSERT(a, a->count == 1, "Function 'head' has way too many arguements. Err0r.");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Incorrect type passed to function 'head'. Err0r.");
	LASSERT(a, a->cell[0]->count != 0, "Function head is passed {} which is empty. Err0r.");
	
	/* If no error, get the first arguements */
	lval *v = lval_take(a, 0);

	/* Delete all non head elements and return */
	while(v->count > 1){
		lval_del(lval_pop(v, 1));
	}

	return v;
}

lval* builtin_tail(lenv *e, lval *a){
	/* Check the error condition */
	LASSERT(a, a->count == 1, "Function 'tail' has way too many arguements. Err0r.");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Incorrect type passed to function 'tail'. Err0r.");
	LASSERT(a, a->cell[0]->count != 0, "Function 'tail' is passed {} which is empty. Err0r.");
	

	/* take the first arugument */
	lval *v = lval_take(a, 0);

	lval_del(lval_pop(v, 0));

	return v;
}

lval* builtin_list(lenv *e,lval *a){
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lenv *e, lval *a){
	LASSERT(a, a->count == 1, "Function 'eval' passed way too many arguments. Err0r.");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type. Err0r.");
	
	lval *x = lval_take(a, 0);
	x->type	= LVAL_SEXPR;
	return lval_eval(e, x);
}

lval* lval_join(lval *x, lval *y){
	/* For each cell in 'y' add it to 'x' */
	while(y->count){
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

lval* builtin_join(lenv *v, lval *a){
	for(int i=0; i< a->count; i++){
		LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type. Err0r. ");
	}

	lval* x = lval_pop(a, 0);

	while(a->count){
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);
	return x;
}


lval* builtin_op(lenv *e, lval *a, char *op){
	/* Firstly ensure that all arguements are numbers */
	for(int i=0; i< a->count; i++){
		if(a->cell[i]->type != LVAL_NUM){
			lval_del(a);
			return lval_err("Clearly you have input a non-number. Please behave yourself, this is an err0r.");
		}
	}

	/* Pop the first elements */
	lval *x = lval_pop(a, 0);

	/* If no arguments and sub expr, then perform unaty negation */
	if((strcmp(op, "-") == 0) && a->count == 0){
		x->num *= -1;
	}

	/* while there are still elements remaining */
	while(a->count > 0){
		/* pop the next element */
		lval *y = lval_pop(a, 0);
		
		if(strcmp(op, "+") == 0) {x->num += y->num; }
		if(strcmp(op, "-") == 0) {x->num -= y->num; }
		if(strcmp(op, "*") == 0) {x->num *= y->num; }
		if(strcmp(op, "%") == 0) {x->num = (int)x->num % (int)y->num;} 
		if(strcmp(op, "^") == 0) {int res = x->num; while(--y->num) res*=x->num; x->num = res;;}
		if(strcmp(op, "min") == 0) {x = x->num < y->num ?x : y;}
		if(strcmp(op, "max") == 0) {x = x->num > y->num?x:y;}
		if(strcmp(op, "/") == 0){
			if(y->num == 0){
				lval_del(x);
				lval_del(y);
				x = lval_err("Division by zero. Classic rookie err0r. ");
			}
			x->num /= y->num;
		}
		lval_del(y);
	}
	lval_del(a);
	return x;
}

lval* builtin_add(lenv *e, lval *a){
	return builtin_op(e, a, "+");
}

/* Wrappers for builtin arithmetic skillz */
lval* builtin_sub(lenv *e, lval *a){
	return builtin_op(e, a, "+");
}
lval* builtin_mul(lenv *e, lval *a){
	return builtin_op(e, a, "*");
}
lval* builtin_div(lenv *e, lval *a){
	return builtin_op(e, a, "/");
}
lval* builtin_max(lenv *e, lval *a){
	return builtin_op(e, a, "max");
}
lval* builtin_min(lenv *e, lval *a){
	return builtin_op(e, a, "min");
}
lval* builtin_mod(lenv *e, lval *a){
	return builtin_op(e, a, "%");
}
lval* builtin_pow(lenv *e, lval *a){
	return builtin_op(e, a, "^");
}
lval* builtin(lenv *e, lval* a, char* func){
	if(strcmp("list", func) == 0) {return builtin_list(e, a); }
	if(strcmp("head", func) == 0) {return builtin_head(e, a); }
	if(strcmp("tail", func) == 0) {return builtin_tail(e, a); }
	if(strcmp("join", func) == 0) {return builtin_join(e, a); }
	if(strcmp("eval", func) == 0) {return builtin_eval(e, a); }
	if(strstr("+-*/maxmin^%", func)) {return builtin_op(e, a, func); }

	lval_del(a);
	return lval_err("Unknown funtion used. Err0r!");
}

void lenv_add_builtin(lenv* e, char *name, lbuiltin func){
	lval *k = lval_sym(name);
	lval *v = lval_fun(func);
	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);
}

void lenv_add_builtins(lenv *e){
	/* List Functions */
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);

	/* Mathematical functions */

	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "^", builtin_pow);
	lenv_add_builtin(e, "max", builtin_max);
	lenv_add_builtin(e, "min", builtin_min);
	lenv_add_builtin(e, "%", builtin_mod);
}

lval* lval_eval_sexpr(lenv *e, lval *v){
	
	/*Evaluate children*/
	for(int i=0; i<v->count; i++){
		v->cell[i] = lval_eval(e, v->cell[i]);
	}

	/* Checking for errors */
	for(int i=0; i<v->count; i++){
		if(v->cell[i]->type == LVAL_ERR){return lval_take(v, i);}	
	}

	/* Empty expression */
	if(v->count == 0) return v;

	/* single expression */
	if(v->count == 1) {return lval_take(v, 0);}

	/* Ensure first element is a function */
	lval *f = lval_pop(v, 0);
	if(f->type != LVAL_FUN){
		lval_del(f);
		lval_del(v);
		return lval_err("The S-expression does not start with a function. Err0r.");
	}

	/* Call builtin_op() with the operator */
	lval *result = f->fun(e, v); 
	lval_del(f);
	return result;
}

lval* lval_eval(lenv *e, lval* v){
	if(v->type == LVAL_SYM){
		lval *x = lenv_get(e, v);
		lval_del(v);
		return x;
	}
	/* Evaluate the S-expressions */
	if (v->type == LVAL_SEXPR){ return lval_eval_sexpr(e, v);}

	/* All the other lval types are evaluates similarly */
	return v;
}

int main(int argc, char** argv){
	/* Creating the parsers for the Polish notation*/
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Symbol	= mpc_new("symbol");
	mpc_parser_t* Sexpr	= mpc_new("sexpr");
	mpc_parser_t* Qexpr 	= mpc_new("qexpr");
	mpc_parser_t* Expr	= mpc_new("expr");
	mpc_parser_t* Peasant 	= mpc_new("peasant");
	
	/*Creating the language grammars for the parsers declared above*/
	/*
	 * The reasoning behind this:
	 * Polish notation can be formalised into pieces as follows:
	 * Number	: one or more character from 0-9 with an optional '-' modifier
	 * Operator	: '+' '-' '/' or '*'
	 * Expression	: Either a number or '(' followed by an operator, 
	 * 		  one or more expressions and a ')'
	 * Program	: The start of an imput, an operator, one or more expressions
	 * 		  and the end of an input
	 */ 

	mpca_lang(MPCA_LANG_DEFAULT,
		"								\
			number	: /-?[0-9]+(\\.[0-9]*)?/;			\
			symbol	: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&^%]+/;		\
			sexpr	: '(' <expr>* ')';				\
			qexpr	: '{' <expr>* '}';				\
			expr	: <number> | <symbol> | <sexpr> | <qexpr>;	\
			peasant	: /^/ <expr>* /$/;				\
		",
		Number, Symbol, Sexpr, Qexpr, Expr, Peasant
	);
	
	lenv* e = lenv_new();
	lenv_add_builtins(e);

	/*Print Version and Exit information*/
	puts("Peasant Lisp Version 0.1");
	puts("Press ctrl+c to exit\n");
	
	/* Attempt to parse the user input*/
	while(1){
		char* input = readline("Peasant> ");
		add_history(input);
		mpc_result_t r;
		/* On success, print the AST */
		if(mpc_parse("<stdin>", input, Peasant, &r)){
			lval* x = lval_eval(e, lval_read(r.output));
			lval_println(x);
			lval_del(x);
			//mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
		}else{
			/* Else print the error messages */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}

	lenv_del(e);

	/* Undefine and Delete the parsers*/
	mpc_cleanup(4, Number, Symbol, Sexpr, Qexpr, Expr, Peasant);
	return 0;
}
