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

/* Helper function for the above eval() function*/
double eval_op(double x, char *op, double y){
	if(strcmp(op, "+") == 0) return x+y;
	if(strcmp(op, "-") == 0) return x-y;
	if(strcmp(op, "*") == 0) return x*y;
	if(strcmp(op, "%") == 0) return (int)x%(int)y;
	if(strcmp(op, "^") == 0) {int res = x; while(--y) res*=x; return res;}
	if(strcmp(op, "min") == 0) return x<y?x:y;
	if(strcmp(op, "max") == 0) return x>y?x:y;

	return 0;
}


double eval(mpc_ast_t* t){
	/*Base case: If tagged as a number, return the number as it is.*/
	if(strstr(t->tag, "number")){
		return atof(t->contents);	
	}

	/*The operator by definitions given below will always be the second child*/
	char *op = t->children[1]->contents;

	/* Sore the third child in var x */
	double x = eval(t->children[2]);

	/* Iterate through the remaining children */
	int i = 3;
	while(strstr(t->children[i]->tag, "expr")){
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char** argv){
	/* Creating the parsers for the Polish notation*/
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Operator	= mpc_new("operator");
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
		"									\
			number	: /-?[0-9]+(\\.[0-9]*)?/;				\
			operator: '+' | '-' | '*' | '/' | '^' | /min/ | /max/ | '%';	\
			expr	: <number> | '(' <operator> <expr>+ ')';		\
			peasant	:/^/ <operator> <expr>+ /$/ ;				\
		",
		Number, Operator, Expr, Peasant
	);


	/*Print Version and Exit information*/
	puts("Peasant Lisp Version 0.1");
	puts("Press ctrl+c to exit\n");
	
	/* Attempt to parse the user input*/
	while(1){
		char* input = readline("Peasant> ");
		mpc_result_t r;
		/* On success, print the AST */
		if(mpc_parse("<stdin>", input, Peasant, &r)){
			double result = eval(r.output);
			printf("%f\n", result);

			//mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
		}else{
			/* Else print the error messages */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
	}

	/* Undefine and Delete the parsers*/
	mpc_cleanup(4, Number, Operator, Expr, Peasant);
	return 0;
}
