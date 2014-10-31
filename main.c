#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>


int mode = 0;
void print_usage(char* program){
	ERROR("Usage:");
	ERRORF("\t- %s /dev/ttyS0 send|rec <filename>", program);
}


int main(int argc, char argv[]) {

	if (argc != 4 || ((strcmp("/dev/ttyS0", argv[1]) != 0))) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
	else if(strcmp(argv[2], "send") == 0 || strcmp(argv[2], "rec") == 0){
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	mode = strcmp(argv[2],"send")

	if(mode){
		//receber
	}
	else{
		//enviar
	}

	return EXIT_SUCESS;  
}
