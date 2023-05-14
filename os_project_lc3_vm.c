/*
	    PROJECT NAME: LIGHTWEIGHT VIRTUAL MACHINE TO SUPPORT LC-3 ARCHITECTURE AND TRAP HANDLING (SYSTEM CALLS USED)
			               TESTING METHOD: rogue.obj and 2048.obj games
    		         This VM supports lc-3 architecture and converts lc-3 instructions into a 
		        computer without any external support, just C compiler is needed on computer
		                   Memory used=>128KB shown how it is allotted in code.

						   We have handled: 
		  * management of interrupts/traps including I/O operations onto terminal through system calls
		  * running vm as a child process and parent will wait until it's executed
		  * thru child we can run multiple vms->multiple childs (will be done in future)
		  * management of memory between instructions, registers
		  * system calls to read object file with proper handling of exceptions
		  * handled relevant mathematical operations thru opcodes which are used by lc-3

		         LC-3 ARCHITECTURE => RISC so smaller instruction set of 10 opcodes supported
						  GROUP MEMBERS:
					      Sana Khalid   (21K-3421)
					      Fizza Rashid  (21K-3409)
					      Virkha Kumari (21K-4507)
*/
