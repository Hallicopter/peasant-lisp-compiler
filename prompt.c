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

int main(int argc, char** argv){
	/*Print Version and Exit information*/
	puts("Peasant Lisp Version 0.1");
	puts("Press ctrl+c to exit\n");
	
	while(1){
		char* input = readline("Peasant> ");
		
		add_history(input);
		printf("You just input %s\n", input);
		/*Input has to be freed as for all inputs
		 * same pointer is used
		 */	
		free(input);	
	}
	return 0;
}
