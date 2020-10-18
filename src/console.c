#include <stdio.h>
#include <string.h>
#include "console.h"
#include "fs_manager.h"

#define TOKEN_LIMIT 50

/*
	Calls apropriate function for entered command if recognized.

	returns 0: on success
		1: on exit command -> program exit
		2: unrecognized command
		3: too few arguments (args 1/2 is NULL with a command)
		4: too many arguments (arg 2 is not NULL with some commands)
*/
uint8_t process_command(char *command_parts[3]) { 
	if(!strcmp(command_parts[0], "exit")) return 1;

	if(!strcmp(command_parts[0], "cp")) {
		//validate param 1
		//validate param 2
		//do cp
	}
	else if(!strcmp(command_parts[0], "mv")) {
	}
	else if(!strcmp(command_parts[0], "rm")) {
	}
	else if(!strcmp(command_parts[0], "mkdir")) {
		if(!command_parts[1]) return 3;
		if(command_parts[2] != NULL) return 4;
		mkdir(command_parts[1]);

	}
	else if(!strcmp(command_parts[0], "rmdir")) {
	}
	else if(!strcmp(command_parts[0], "ls")) {
	}
	else if(!strcmp(command_parts[0], "cat")) {
	}
	else if(!strcmp(command_parts[0], "cd")) {
	}
	else if(!strcmp(command_parts[0], "pwd")) {
	}
	else if(!strcmp(command_parts[0], "info")) {
	}
	else if(!strcmp(command_parts[0], "incp")) {
	}
	else if(!strcmp(command_parts[0], "outcp")) {
	}
	else if(!strcmp(command_parts[0], "load")) {
	}
	else if(!strcmp(command_parts[0], "format")) {
		if(!command_parts[1]) return 3;
		if(command_parts[2] != NULL) return 4;
		format(command_parts[1]);
	}
	else if(!strcmp(command_parts[0], "slink")) {
	} else {
		return 2;
	}

	return 0;
}

uint8_t run_console() {
	char input[80] = {0}, *token, *saveptr;
	char *command_parts[3];
	uint8_t running = 1, token_count = 0, i = 0;

	//load_filesystem();

	while(running) {
		//memset(input, 0, 80); 
		token_count = 0;
		saveptr = input;

		// NULL command parts so they dont carry over to next cmd
		// done here because of the first run
		for(i = 0; i < 3; i++) {
			command_parts[i] = NULL;
		}

		printf("> ");
		scanf(" %[^\n]s", input);

		while((token = strtok_r(saveptr, " ", &saveptr)) && 
		      (token_count < TOKEN_LIMIT)) {
			if(token_count < 3)
				command_parts[token_count] = token;
			token_count++;
			//printf("%s\n", token);
		}

		if(token_count > 3) {
			printf("ERROR: too many arguments. There is no command\
 with more than 2 arguments.\n");
			continue;
		}

		switch(process_command(command_parts)) {
			case 1: running = 0; break;
			case 2: printf("%s: Command not found\n",
				command_parts[0]); break;
			case 3: print_error("Command requires 1 argument.");
				break;
			case 4: print_error("Too many arguments."); break;
		}
	}

	return 0;
}
