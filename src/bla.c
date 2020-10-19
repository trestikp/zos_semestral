#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <inttypes.h>

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

	/*
	FILE *f = fopen("superlongstring.txt", "ab+");
	int i = 0;
	for(i = 0; i < 100; i++)
		fputc(0x30, f);
	*/

	//FILE *f = fopen("../myFS.dat", "r");

	/*
	struct statvfs *vfs = malloc(sizeof(struct statvfs));
	statvfs("../myFS.dat", vfs);
	printf("blocksize: %ld \n", vfs->f_bsize);
	printf("free blocks: %ld \n", vfs->f_bavail);
	printf("free size: %ld \n", ((vfs->f_bsize * vfs->f_bavail)) / (1024 * 1024 * 1024));
	*/

	int i = 0;
	uint8_t n = 0xE0;
	//uint8_t n = 0x00;
	FILE *f = fopen("./pokus.txt", "r");
	printf("%d\n", fgetc(f));
	printf("%d\n", fgetc(f));
	printf("%d\n", fgetc(f));
	printf("%d\n", fgetc(f));
	printf("%d\n", fgetc(f));
	/*
	fputc(0x40, f);
	fputc(0x40, f);
	fputc(0x40, f);
	fputc(0xE0, f);
	*/
	/*
	for(i = 0; i < sizeof(uint8_t) * 8; i++) {
		//printf("%u \n", (1 << (sizeof(uint8_t) * 8 - 1 - i)));
		if(!(n & (1 << (sizeof(uint8_t) * 8 - 1 - i)))) {
			printf("%d\n", i);
			break;
		}
	}
	*/
	/*
	fseek(f, 3, SEEK_SET);
	n = fgetc(f);
	printf("n: %d\n", n);
	for(i = (sizeof(uint8_t) * 8 - 1); i >= 0; i--) {
		if(!(n & (1 << i))) {
			printf("%d\n", i);
			n |= (1 << i);
			break;
		} else {
			printf("nope %d\n", i);
		}
	}
	printf("n: %d\n", n);
	//fputc(n, f);
	*/

	freopen(NULL, "rb+", f);
	fputc(0xFF, f);
	fputc(0xFF, f);
	fputc(0xFF, f);
	fputc(0xFF, f);
	
	return 0;
}
