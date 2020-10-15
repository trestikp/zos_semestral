#include <stdio.h>
#include <string.h>
#include "console.h"
#include "fs_manager.h"

#define TOKEN_LIMIT 50

uint8_t process_command(char *command_parts[3]) { 
	if(!strcmp(command_parts[0], "exit")) return 2;

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
			format(command_parts[1]);
	}
	else if(!strcmp(command_parts[0], "slink")) {
	} else {
		return 1;
	}

	return 0;
}

uint8_t run_console() {
	char input[80] = {0}, *token, *saveptr;
	char *command_parts[3];
	uint8_t running = 1, token_count = 0;

	load_filesystem();

	while(running) {
		//memset(input, 0, 80); 
		token_count = 0;
		saveptr = input;

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
			case 2: running = 0; break;
			case 1: printf("%s: command not found\n",
				command_parts[0]); break;
		}
	}

	return 0;
}
