#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>

#define __NR_sys_uselib		__NR_uselib

inline _syscall1(int, sys_uselib, char*, l);

//void make_lib(const char* libname, unsigned long lib_addr, size_t LIB_SIZE);

void systemf(char*, ...);

int main(int argc, char** argv){
	sys_uselib(argv[1]);
	systemf("cat /proc/%d/maps", getpid());
	return 0;
}
