#!/bin/bash


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
DEBUG_BINARY=./debug

exec >  >(tee -a log-vm-test.txt | ${DEBUG_BINARY})
exec 2> >(tee -a log-vm-test.txt | ${DEBUG_BINARY} >&2)

log_msg "mklib"

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
		echo "Error running fsstress" | ${DEBUG_BINARY}
	fi
						
)&


# Call the empty process for the other two CPUs 
echo "Process 3: ${EMPTY_TEST_FILENAME}"
USC_SKI_ENABLED=${SKI_ENABLE} USC_SKI_FORK_ENABLED=0 USC_SKI_CPU_AFFINITY=2 USC_SKI_TEST_NUMBER=1 USC_SKI_SOFT_EXIT_BARRIER=1 USC_SKI_USER_BARRIER=0 USC_SKI_HYPERCALLS=1 ${EMPTY_TEST_FILENAME} &

echo "Process 4: ${EMPTY_TEST_FILENAME}"
USC_SKI_ENABLED=${SKI_ENABLE} USC_SKI_FORK_ENABLED=0 USC_SKI_CPU_AFFINITY=3 USC_SKI_TEST_NUMBER=1 USC_SKI_SOFT_EXIT_BARRIER=1 USC_SKI_USER_BARRIER=0 USC_SKI_HYPERCALLS=1 ${EMPTY_TEST_FILENAME} &

wait

log_msg "All test processes finished!"

log_msg "Executing post-test ps command"
ps -All -f



if [ "$VM_TEST_QUIT" -eq 1 ]
then
	# Special command to signal to SKI the end of the snapshot process
	echo "Guest finished snapshot" | ${DEBUG_BINARY}
fi
