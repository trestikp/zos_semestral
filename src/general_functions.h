#ifndef _GENERAL_FUNCTIONS_H_
#define _GENERAL_FUNCTIONS_H_

/**************************************/
/* 				      */
/*	Includes		      */
/*				      */
/**************************************/



/**************************************/
/* 				      */
/*	Constants		      */
/*				      */
/**************************************/

#define GENERAL_FS_SIGNATURE "trestikp"
#define GENERAL_FS_DESCRIPTION "This is a ZOS 2020 semestral work. Author is\
 trestikp"

#define COMMAND_SUCCES "OK"
#define FORMAT_CMD_ERROR "CANNOT CREATE FILE"
#define CMD_ERR_FNF "FILE NOT FOUND"
#define CMD_ERR_SOURCE_FNF "FILE NOT FOUND (missing source)"
#define CMD_ERR_TARGET_PNF "PATH NOT FOUND (non-existent target path)"
#define CMD_ERR_ENTERED_PNF "PATH NOT FOUND (non-existent entered path)"
#define CMD_ERR_DIR_PNF "PATH NOT FOUND (non-existent directory)"
#define CMD_ERR_PATH_PNF "PATH NOT FOUND (non-existent path)"
#define CMD_ERR_EXIST "EXIST (cannot create, already exists)"
#define CMD_ERR_NOT_EMPTY "NOT EMPTY (directory contains subdirectories\
 of files)"


#define OUT_OF_MEMORY_ERROR 1
#define FILE_OPEN_ERROR "Failed to open file."
#define FILE_REOPEN_ERROR_MESSAGE "Failed to reopen file for writing."
#define MEMORY_ALLOCATION_ERROR_MESSAGE "Failed to allocate RAM."


/**************************************/
/* 				      */
/*	Macros  		      */
/*				      */
/**************************************/

#define print_command_result(result)\
	printf("%s\n", result);

#define print_error(message)\
	printf("ERROR: %s\n", message);\

#define print_error_on_condition(condition, message)\
	if(condition)\
		printf("ERROR: %s\n", message);\
	
#define print_info(message, ...)\
	printf("INFO: %s\n", message);\

#define return_error_on_condition(condition, message, ret_value)\
	if(condition) {\
		printf("ERROR: %s\n", message);\
		return ret_value;\
	}

#endif
