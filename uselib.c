#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <syscall.h>
#include <limits.h>
//#define __SSE__
//#include <xmmintrin.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#include "ski-barriers.h"
#include "ski-hyper.h"

#define __NR_sys_gettimeofday	__NR_gettimeofday
#define __NR_sys_sched_yield	__NR_sched_yield
#define __NR_sys_madvise	__NR_madvise
#define __NR_sys_uselib		__NR_uselib
#define __NR_sys_mmap2		__NR_mmap2
#define __NR_sys_munmap		__NR_munmap
#define __NR_sys_mprotect	__NR_mprotect
#define __NR_sys_mremap		__NR_mremap

inline _syscall6(int, sys_mmap2, int, a, int, b, int, c, int, d, int, e, int, f);

inline _syscall5(int, sys_mremap, int, a, int, b, int, c, int, d, int, e);

inline _syscall3(int, sys_madvise, void*, a, int, b, int, c);
inline _syscall3(int, sys_mprotect, int, a, int, b, int, c);
inline _syscall3( int, modify_ldt, int, func, void *, ptr, int, bytecount );

inline _syscall2(int, sys_gettimeofday, void*, a, void*, b);
inline _syscall2(int, sys_munmap, int, a, int, b);

inline _syscall1(int, sys_uselib, char*, l);

inline _syscall0(void, sys_sched_yield);

void systemf(char*, ...);

char stack1[1][4096*4];

extern int STD_SKI_ENABLED, STD_SKI_FORK_ENABLED, STD_SKI_WAIT_FOR_RESULTS, STD_SKI_CPU_AFFINITY, STD_SKI_HYPERCALLS, STD_SKI_SOFT_EXIT_BARRIER, STD_SKI_USER_BARRIER, STD_SKI_TOTAL_CPUS, STD_SKI_TEST_NUMBER, STD_SKI_PROFILE_ENABLED;

int ski_test_start(int current_cpu, int total_cpus, int dry_run);
void ski_test_finish(int current_cpu, int dry_run);
void hypercall_debug(int current_cpu, char *format, ...);
void hypercall_debug_quiet(int current_cpu, char *format, ...);
char* ski_parse_env(void);

void print_affinity(FILE* file){
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	sched_getaffinity(0, &cpus);
	unsigned i;
	for(i=0; i!=sizeof(cpus.__bits)/sizeof(cpus.__bits[0]); ++i){
		fprintf(file, "%u: %lx\n",(unsigned)(i * sizeof(cpus.__bits[0]) * 8), cpus.__bits[i]);
	}
}

static volatile unsigned low = 0x50000000 - 4096;
static volatile unsigned high = 0xb0000000;
int child(void* arg){
	char* lib = (char*)arg;

	hypercall_debug_quiet(STD_SKI_CPU_AFFINITY + 1, "Child thread started pid=%d lib=%s\n", (int)getpid(), lib);
	//dry run
	ski_test_start(STD_SKI_CPU_AFFINITY + 1, STD_SKI_TOTAL_CPUS, 1);

	int ski_ret = ski_test_start(STD_SKI_CPU_AFFINITY + 1, STD_SKI_TOTAL_CPUS, 0);
	(void)ski_ret;
	int ret = sys_uselib(lib);
	unsigned local_low = low;
	unsigned local_high = high;
	ski_test_finish(STD_SKI_CPU_AFFINITY + 1, 0);
	hypercall_debug_quiet(STD_SKI_CPU_AFFINITY + 1, "Child finished low=%x, high=%x\n", local_low, local_high);
	exit(ret);
}

void map_a_page_pair(void){
	static int prot = PROT_READ;
	sys_mmap2(low+=4096, 4096, prot ^= PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, 0, 0);
	sys_mmap2(high-=4096, 4096, prot, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, 0, 0);		
}

int main(int argc, char** argv){
	if(mlockall(MCL_CURRENT) == -1)
	{
		fprintf(stderr, "mlockall failed reason %s\n", strerror(errno));
		return -1;
	}
	if(munlockall() == -1)
	{
		fprintf(stderr, "munlockall failed reason %s\n", strerror(errno));
		return -1;
	}

	ski_parse_env();
	
	int i, n = atoi(argv[1]);

	//dry run
	ski_test_start(STD_SKI_CPU_AFFINITY, STD_SKI_TOTAL_CPUS, 1);
	hypercall_debug_quiet(STD_SKI_CPU_AFFINITY, "Begin test parent pid %d\n", (int)getpid());
	
	clone(child, stack1+1, CLONE_VM, argv[2]);
    for(i=0;i<n/2;++i){
		map_a_page_pair();
	}
	int ski_ret = ski_test_start(STD_SKI_CPU_AFFINITY, STD_SKI_TOTAL_CPUS, 0);
	for(i=n/2;i!=n;++i){
		map_a_page_pair();
	}
	
	ski_test_finish(STD_SKI_CPU_AFFINITY, 0);

	hypercall_debug_quiet(STD_SKI_CPU_AFFINITY, "Parent finished\n");
	
	systemf("cat /proc/%d/maps | ./debug", getpid());

	int status;
	pid_t child_id = waitpid(-1, &status, __WCLONE);
	hypercall_debug_quiet(STD_SKI_CPU_AFFINITY, (char*)"wait for child %d finish, err=%d", child_id, status);
	hypercall_debug_quiet(STD_SKI_CPU_AFFINITY, (char*)"END INFO");

	return 0;
}
