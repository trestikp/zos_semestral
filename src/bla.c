#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct bla {
	int a;
};

int main() {
	/*
	char in[20];
	scanf("%s", in);

	if(!strcmp(in, "ahoj"))
		printf("Jo\n");
		#define AHOJ

#ifdef AHOJ
	#define VAR 5
#else
	#define VAR 1024
#endif
	*/

	/*
	int a = 1, b = 10;
	while ((a + 2) <= b) {
		a += 2;
	}

	printf("%d\n", a);
	*/
	/*
	struct bla *var = malloc(sizeof(struct bla));

	//int a = 0;
	do{
		printf("%d\n", var->a);
		var->a++;
	} while(var->a % 8);
	while((a + 1) % 9) {
		printf("%d\n", a);
		a++;
	}
	*/

	FILE *f = fopen("superlongstring.txt", "ab+");
	int i = 0;
	for(i = 0; i < 100; i++)
		fputc(0x30, f);
	
	return 0;
}
