#!/bin/bash

echo "[dmesg refresh]" | sudo tee /var/log/dmesg
echo "[syslog refresh]" | sudo tee /var/log/syslog

error_msg(){
	echo [SKI-TESTCASE-RUN.SH] ERROR: $1
	echo [SKI-TESTCASE-RUN.SH] ERROR: Exiting!!
	exit 1
}

log_msg(){
	echo [SKI-TESTCASE-RUN.SH] $1
}
		

export USC_SKI_HYPERCALLS=${USC_SKI_HYPERCALLS-0}
export USC_SKI_ENABLED=${USC_SKI_ENABLED-1}
export USC_SKI_TOTAL_CPUS=4

VM_TEST_QUIT=${VM_TEST_QUIT-1}
EMPTY_TEST_FILENAME=${EMPTY_TEST_FILENAME-./empty}

exec >> msg.log
exec 2>> err.log

log_msg "mklib"

#generate a elf library which is based @ 0x80000000 of length 8k
./mklib /dev/shm/mylib 0x80000000 8192
./loadlib /dev/shm/mylib

log_msg "Running uselib and empty process"

USELIB_ARGS="100 /dev/shm/mylib"

SKI_ENABLE=1
(
	echo "Process 1 & 2 (with FORK):"
	USC_SKI_ENABLED=${SKI_ENABLE} USC_SKI_FORK_ENABLED=1 USC_SKI_CPU_AFFINITY=0 USC_SKI_TEST_NUMBER=1 USC_SKI_SOFT_EXIT_BARRIER=1 USC_SKI_USER_BARRIER=0 USC_SKI_HYPERCALLS=1 ./uselib ${USELIB_ARGS}
	RES=$?
	if [ "$RES" -ne 0 ]
	then
		echo "Error Running Testcase"
	fi
						
)&


# Call the empty process for the other two CPUs 
echo "Process 3: ${EMPTY_TEST_FILENAME}"
USC_SKI_ENABLED=${SKI_ENABLE} USC_SKI_FORK_ENABLED=0 USC_SKI_CPU_AFFINITY=2 USC_SKI_TEST_NUMBER=1 USC_SKI_SOFT_EXIT_BARRIER=1 USC_SKI_USER_BARRIER=0 USC_SKI_HYPERCALLS=1 ${EMPTY_TEST_FILENAME} &

echo "Process 4: ${EMPTY_TEST_FILENAME}"
USC_SKI_ENABLED=${SKI_ENABLE} USC_SKI_FORK_ENABLED=0 USC_SKI_CPU_AFFINITY=3 USC_SKI_TEST_NUMBER=1 USC_SKI_SOFT_EXIT_BARRIER=1 USC_SKI_USER_BARRIER=0 USC_SKI_HYPERCALLS=1 ${EMPTY_TEST_FILENAME} &

wait

exec > >(./debug)

log_msg "[[log]]"
cat msg.log
log_msg "[[err]]"
cat err.log

log_msg "[[syslog]]"
cat /var/log/syslog
log_msg "[[dmesg]]"
cat /var/log/dmesg

#terminate SKI
echo "END INFO"
