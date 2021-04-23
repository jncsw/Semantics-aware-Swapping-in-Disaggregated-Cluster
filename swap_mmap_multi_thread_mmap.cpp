
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>  
#include <sys/errno.h>
#include <pthread.h>

#include "stdint.h"
#include "stdio.h"
#include "time.h"
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



/**
 * Reserve memory at fixed address 
 */
static char* reserve_anon_memory(char* requested_addr, unsigned long bytes, bool fixed) {
	char * addr;
	int flags;

	flags = MAP_PRIVATE | MAP_NORESERVE | MAP_ANONYMOUS;   
	if (fixed == true) {
			printf("Request fixed addr 0x%lx ", (unsigned long)requested_addr);

		flags |= MAP_FIXED;
	}

	// Map reserved/uncommitted pages PROT_NONE so we fail early if we
	// touch an uncommitted page. Otherwise, the read/write might
	// succeed if we have enough swap space to back the physical page.
	addr = (char*)mmap(requested_addr, bytes, PROT_NONE,
											 flags, -1, 0);

	return addr == MAP_FAILED ? NULL : addr;
}



/**
 * Commit memory at reserved memory range.
 *  
 */
char* commit_anon_memory(char* start_addr, unsigned long size, bool exec) {
	int prot = (exec == true) ? PROT_READ|PROT_WRITE|PROT_EXEC : PROT_READ|PROT_WRITE;
	unsigned long res = (unsigned long)mmap(start_addr, size, prot,
																		 MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0);   // MAP_FIXED will override the old mapping
	
	// commit memory successfully.
	if (res == (unsigned long) MAP_FAILED) {

		// print errno here.
		return NULL;
	}

	return start_addr;
}



void scan_array_sequential_overleap(){

    char* user_buff;
	size_t array_slice = ARRAY_BYTE_SIZE/sizeof(unsigned long); // the entire array
	size_t array_start = 0; 	// scan from start
	size_t i=0, sum=0;


	int type = 0x1;
	unsigned long request_addr 	= ARRAY_START_ADDR; // start of RDMA meta space, 1GB not exceed the swap partitio size.
	unsigned long size  		= ARRAY_BYTE_SIZE;	// 512MB, for unsigned long, length is 0x4,000,000
	pthread_t threads[online_cores];
	struct thread_args args[online_cores]; // pass value by reference, so keep a copy for each thread
	int ret =0;
	srand(time(NULL));		// generate a random interger

	//online_cores = sysconf(_SC_NPROCESSORS_ONLN);

	// 1) reserve space by mmap
	user_buff = reserve_anon_memory((char*)request_addr, size, true );
	if(user_buff == NULL){
		printf("Reserve user_buffer, 0x%lx failed. \n", (unsigned long)request_addr);
	}else{
		printf("Reserve user_buffer: 0x%lx, bytes_len: 0x%lx \n",(unsigned long)user_buff, size);
	}

	// 2) commit the space
	user_buff = commit_anon_memory((char*)request_addr, size, false);
	if(user_buff == NULL){
		printf("Commit user_buffer, 0x%lx failed. \n", (unsigned long)request_addr);
	}else{
		printf("Commit user_buffer: 0x%lx, bytes_len: 0x%lx \n",(unsigned long)user_buff, size);
	}


    // user_buff = (char *) malloc(ARRAY_BYTE_SIZE);
    // if (user_buff == NULL){
    //     printf("Dynamic malloc %lu bytes failed. \n", size);
    //     exit(0);
    // } else {
    //     printf("Dynamic malloc %lu bytes succeed. \n", size);
    // }

	printf("Writing %lu element to %lu bytes array \n", array_slice, size);
	unsigned long * buf_ul_ptr = (unsigned long*)user_buff;
	int tmp;
	printf("Start \n");
	getchar();
	// scanf("%d", &tmp);
	printf("Stage 1 \n");
	
	for( i = array_start  ; i < array_start + array_slice/2  ; i++ ){
		buf_ul_ptr[i] = i;  // the max value.
	}
	getchar();
	// scanf("%d", &tmp);
	printf("Stage 2 \n");

	for( i = array_start+array_slice/2  ; i < array_start + array_slice  ; i++ ){
		buf_ul_ptr[i] = i;  // the max value.
	}
	// Swap 500HEAD - mem 500 TAIL
	getchar();
	// scanf("%d", &tmp);
	printf("Stage 3 \n");
	//long sum = 0;
	for( i = array_start  ; i < array_start + array_slice/2  ; i++ ){
		sum += buf_ul_ptr[i];  // the max value.
	}
	getchar();
	// scanf("%d", &tmp);
	printf("Stage 4 \n");
	for( i = array_start+array_slice/2  ; i < array_start + array_slice  ; i++ ){
		sum += buf_ul_ptr[i];  // the max value.
	}
// Swap 500HEAD - mem 500 TAIL
	printf("Finished. Start to access one page.\n");
	getchar();
	// int tmp;
	// int tmp2;
	// scanf("%d", &tmp2);
	// printf("Value = %d\n",tmp2);
	// scanf("start: %d\n", &tmp);
	// pause();
	// while(1);
	// for( i = array_start  ; i < array_start + 1000  ; i++ ){
		
	// 	printf("The first element on the 2nd page is: %lu \n", buf_ul_ptr[i] );
	// }
    i = array_start + (PAGE_SIZE / sizeof(unsigned long));
	printf("Reading 1st element of the array on the 2nd page \n");

	printf("The first element on the 2nd page is: %lu \n", buf_ul_ptr[i] );
	// scanf("%d", &tmp2);

	printf("Next....Start to update page...\n");
	getchar();
	for( i = array_start  ; i < array_start + (PAGE_SIZE / sizeof(unsigned long))*10  ; i++ ){
		buf_ul_ptr[i] += 1;
	}
	// scanf("%d", &tmp2);
	printf("Finished....\n");
	getchar();
}


// g++ swap_mmap_multi_thread.cpp -static -o swap_cgroup
// ./swap_cgroup
int main(){

    scan_array_sequential_overleap(); 

	return 0; //useless ?
}
