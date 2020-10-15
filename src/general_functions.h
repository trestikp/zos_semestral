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

#define OUT_OF_MEMORY_ERROR 1
#define FILE_REOPEN_ERROR_MESSAGE "Failed to reopen file for writing."


/**************************************/
/* 				      */
/*	Macros  		      */
/*				      */
/**************************************/

#define print_error(message)\
	printf("ERROR: %s\n", message);\

#define print_error_on_condition(condition, message)\
	if(condition)\
		printf("ERROR: %s\n", message);\
	


#endif
