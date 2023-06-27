#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 	500
#define DEVICE_NAME	"/dev/chfifo" // name of the char device


int main(int argc, char* argv[])
{
	char write_buf[BUF_SIZE];
	char read_buf[BUF_SIZE];
        int fd;  // device fd
        char option;

	FILE *fp;

        int str_size = 0;


	if(argc == 2){
	    fp = fopen(argv [1], "r");
	    if (fp == NULL) {
		perror ("Error: Cannot open test input file...\n");
		return 2;
	    }

	    fd = open(DEVICE_NAME, O_RDWR); // open device file
	    if(fd < 0) {
		printf("Error: Cannot open device file...\n");
		return 2;
	    }

	    int count, res;
	    while( ( count = fread(&write_buf, sizeof(char), BUF_SIZE, fp )) > 0 ){
		printf("\n ====> Read test file:  read bytes = %d\n", count);

		printf("\n ====> Write bytes to char dev file ... \n");
	        res = write(fd, write_buf, count);

		printf("\n ====> Reads bytes from char dev file ... \n");
	        res = read(fd, read_buf, count);


		printf("\n ====> Print bytes. res = %d\n", res);
	        int i;
		for(i=0; i < count; ++i){
		    printf("%c",read_buf[i]);
		}
		memset(read_buf, 0, BUF_SIZE);
		memset(write_buf, 0, BUF_SIZE);
	    }
	    fclose(fp);
	    close(fd);

	    return 1;
	}else{
	// If no file input provided interactive mode with the user
	    fd = open(DEVICE_NAME, O_RDWR);
	    if(fd < 0) {
		printf("Cannot open device file...\n");
		return 0;
	    }
	    while(1) {
		printf("****Please Enter the Option******\n");
		printf("        1. Write               \n");
		printf("        2. Read                 \n");
		printf("        3. Exit                 \n");
		printf("*********************************\n");
		scanf(" %c", &option);
	        switch(option) {
	                case '1':
	                        printf("Enter the string to write into driver :");
	                        scanf("  %[^\t\n]s", write_buf);
				str_size = strlen((const char*)write_buf)+1; 
	                        //printf("Data Writing ... str size = %d\n", str_size);
	                        int res = write(fd, write_buf, str_size);
	                        printf("Done!  Written bytes = %d\n", res);
	                        break;
	                case '2':
	                        printf("Data Reading ...");
				if(str_size > 0){
			            int res = read(fd, read_buf, str_size);
			            printf("Done!  Read bytes = %d\n\n", res);
			            printf("Data = %s\n\n", read_buf);
				    str_size = 0;
				}
	                        break;
	                case '3':
	                        close(fd);
	                        exit(1);
	                        break;
	                default:
	                        printf("Enter Valid option = %c\n",option);
	                        break;
		  }
	      }
	      close(fd);
	}
}
