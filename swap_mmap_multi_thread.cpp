
//#include <linux/kernel.h>
//#include <sys/syscall.h>
#include <unistd.h>
//#include <sys/mman.h>  
//#include <sys/errno.h>
//#include <pthread.h>
#include <string.h>
//#include "stdint.h"
#include "stdio.h"
//#include "time.h"
#include "stdlib.h"

// c++ stl multithread random number generator
#include <random>

/**
 * Warning : the tradition srand() is a sequential generator. 
 * 	The multiple threads will be bound at the random number generation.
 */



#define ONE_MB    1048576UL				// 1024 x 2014 bytes
#define ONE_GB    1073741824UL   	// 1024 x 1024 x 1024 bytes

#define PAGE_SIZE  4096
#define PAGE_SHIFT 12

//typedef enum {true, false} bool;

extern errno;

//#define ARRAY_BYTE_SIZE 0xc0000000UL

// #define ARRAY_START_ADDR	0x400000000000UL - 3*ONE_GB	- ONE_MB
#define ARRAY_START_ADDR	4*ONE_GB 
// #define ARRAY_BYTE_SIZE   15*ONE_GB  // 
#define ARRAY_BYTE_SIZE   ONE_GB  // 

int online_cores = 8;

struct thread_args{
	size_t	thread_id; // thread index, start from 0
	char* 	user_buf;
};


void scan_array_sequential_overleap(){

    char* user_buff;
    unsigned long size = ARRAY_BYTE_SIZE;
	size_t array_slice = ARRAY_BYTE_SIZE/sizeof(unsigned long); // the entire array
	size_t array_start = 0; 	// scan from start
	size_t i, sum;

    user_buff = (char *) malloc(ARRAY_BYTE_SIZE);
    if (user_buff == NULL){
        printf("Dynamic malloc %lu bytes failed. \n", size);
        exit(0);
    } else {
        printf("Dynamic malloc %lu bytes succeed. \n", size);
    }

	printf("Writing %lu element to %lu bytes array \n", array_slice, size);
	unsigned long * buf_ul_ptr = (unsigned long*)user_buff;
	for( i = array_start  ; i < array_start + array_slice  ; i++ ){
		buf_ul_ptr[i] = i;  // the max value.
	}

    i = array_start + (PAGE_SIZE / sizeof(unsigned long));
	printf("Reading 1st element of the array on the 2nd page \n");

	printf("The first element on the 2nd page is: %lu \n", buf_ul_ptr[i] );

}



int main(){

    scan_array_sequential_overleap(); 

	return 0; //useless ?
}
