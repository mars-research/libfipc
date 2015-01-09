#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define BETA_GET_CPU_AFFINITY 1<<1
#define BETA_CONNECT_SHARED_MEM 1<<2
#define BETA_UNPARK_THREAD 1<<3
#define BETA_ALLOC_MEM 1<<4
#define BETA_GET_MEM 1<<5

#define printf_v(fmt, ...) \
	if(verbose)	   \
		printf(fmt,##__VA_ARGS__);
		

static int verbose;

static int open_beta_drivers(int* beta1, int* beta2)
{
	
	printf_v("[*] Opening \"betaIPC\"\n");

	*beta1 = open("/dev/betaIPC", O_RDWR);
	if(*beta1 == -1) {
		printf("Failed to open /dev/betaIPC with %s\n", strerror(errno));
		return 1;
	}

	printf_v("[*] Opening \"betaIPC2\"\n");

	*beta2 = open("/dev/betaIPC2", O_RDWR);
	if(*beta2 == -1) {
		printf("Failed to open /dev/betaIPC with %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

static int alloc_mem_beta1(int fd)
{
	long ret = -1;
	ret = ioctl(fd, BETA_ALLOC_MEM, NULL);
	if(ret != 0) {
		printf("Failed to alloc mem, read dmesg!\n");
		return 1;
	}
	return 0;
}


static int get_mem_region_beta1(int fd, unsigned long **ptr) 
{
	
	long ret = -1;
	ret = ioctl(fd, BETA_GET_MEM, ptr);
	if(ret != 0 || *ptr == NULL) {
		printf("Failed to get mem region!, read dmesg?\n");
		return 1;
	}
	return 0;
}

static int connect_mem_regions(int fd2, unsigned long *ptr)
{
	long ret = -1;
	ret = ioctl(fd2, BETA_CONNECT_SHARED_MEM, ptr);
	if(ret != 0) {
		printf("Failed to connect mem with beta2, dmesg!\n");
		return 1;
	}
	return 0;
}

static int unpark_threads(int fd, int fd2)
{
	
	long ret = -1;
	ret = ioctl(fd2, BETA_UNPARK_THREAD, NULL);
	if(ret != 0) {
		printf("Couldn't unpark beta2!\n");
		return 1;
	}

	ret = ioctl(fd, BETA_UNPARK_THREAD, NULL);
	if(ret != 0) {
		printf("Couldn't unpark beta1!\n");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{

	verbose = atoi(argv[1]);
	int beta1, beta2;
	beta1 = beta2 = -1;
	unsigned long *ptr = NULL;
	if(open_beta_drivers(&beta1, &beta2)){
		printf("Failed to open one of the beta drivers!\n");
		exit(EXIT_FAILURE);
	}
	
	printf_v("[*] Allocating mem on IPCbeta (1)\n");

	if(alloc_mem_beta1(beta1)) 
		goto cleanup;	
	printf_v("[*] Getting memory region of alloc'd kernel mem from IPCBeta (1)\n");
	if(get_mem_region_beta1(beta1, &ptr)) 
		goto cleanup;	
	
	printf_v("[*] Got ptr %lx asking kland to take ptr from uland: %lx\n", ptr, &ptr);
	if(connect_mem_regions(beta2, &ptr)) 
		goto cleanup;	
	
	printf_v("[*] Unparking both threads PREPARE FOR PANIC!\n");
	if(unpark_threads(beta1,beta2)) 
		goto cleanup;

	/* PROBBALY KERNEL PANICKING HARD BY NOW OR STALLED 2 CPUS*/
	
	printf_v("[*] THREADS UNPARKED, SLEEPING FOR 2 SEC \n");
	sleep(2);
	printf_v("[*] AWAKE CLOSING AND EXITING \n");
 cleanup:
	close(beta1);
	close(beta2);
	exit(EXIT_FAILURE);
	
}	





