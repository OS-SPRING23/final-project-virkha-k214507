/*
	    PROJECT NAME: LIGHTWEIGHT VIRTUAL MACHINE TO SUPPORT LC-3 ARCHITECTURE AND TRAP HANDLING (SYSTEM CALLS USED)
			               TESTING METHOD: rogue.obj and 2048.obj games
		In this file, we basically run the lc-3 vm as a child process thru execution file of the lc-3
		virtual machine. 
		                   RUN ON TERMINAL: gcc os_project_run_vm.c -o run_vm
				                    gcc -Wall -std=c11 os_project_lc3_vm.c
				                    ./run_vm a.out rogue.obj #for rogue game
				                    ./run_vm a.out 2048.obj #for 2048 game
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char *argv[]) {
    if (argc<2) {
        printf("Usage: %s <lc-3 vm>\n", argv[0]);
        return 1;
    }
    int pid=fork();
    if (pid<0) {
        printf("Failed creation of child process.\n");
        return 1;
    } 
    else if (pid==0) {
        //Child process -> here the vm will be executed as child process independtly thru 'execv' system call -> if error then 
	//exception handled as well
        if ( execv(argv[1], argv+1)==-1 ) {
            printf("Error executing user program.\n");
            return 1;
        }
    } 
    else{
        //parent process
	//will wait for the child process-vm to execute
	//here we can run multiple vms as other childs independently -> future work        
	int status;
        waitpid(pid, &status, 0);
	//makes sure that vm-child process executed was successfully or terminated
        if (WIFEXITED(status)) {
            printf("Child process exited with status %d.\n", WEXITSTATUS(status));
        } 
	else if (WIFSIGNALED(status)) {
            printf("Child process terminated by signal %d.\n", WTERMSIG(status));
        }
    }
    return 0;
}
