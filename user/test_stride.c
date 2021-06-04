#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]){
	int pid;
	int start, end;
	start = uptime();
	if((pid = fork()) == 0){
		change_tickets(10);
		while(1){
			end = uptime();
			if((end-start) > 100)
				break;
		}
		printf("\nchild\nmy tickets=%d\nmy pass = %d\ncalledtimes=%d\n", return_tickets(), return_pass(), return_called_times());
		exit(1);

	} else if(pid > 0){
		change_tickets(20);
		while(1){
			end = uptime();
			if((end-start) > 100)
				break;
		}
		printf("\nparent\nmy tickets=%d\nmy pass = %d\ncalledtimes=%d\n", return_tickets(), return_pass(), return_called_times());
		exit(1);
		
	} else {
		exit(1);
	}

	wait(0);
	exit(1);
}


