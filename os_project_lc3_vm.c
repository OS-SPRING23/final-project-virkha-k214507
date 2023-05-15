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
					      Fizza Rashid  (21K-3390)
					      Virkha Kumari (21K-4507)
*/
//sana khalid
#include <stdio.h>
#include <stdint.h> //to support specified bits in integer
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h> //to handle i/o operation
#include <sys/types.h>
#include <sys/termios.h> //to handle i/o operation
#include <sys/mman.h>

//order do matter in the register initialization -> each register will be of 16 bits
enum{
	r_r0=0,
	r_r1,
	r_r2,
	r_r3,
	r_r4,
	r_r5,
	r_r6,
	r_r7,
	r_pc, //program counter
	r_cond, //condition reg
	r_count
};

//flags=> neg,zero,pos to store result produced
enum{
	flpos=1<<0, //1<<0 -> 2^0*1 -> 1
	flzero=1<<1, //1<<1 -> 2^1*1 -> 2
	flneg=1<<2 //1<<2 -> 2^2*1 -> 4
};
/*
NOW LETS DEFINE MEMORY AND REGISTERS:
	lc-3 can access total 65536 memory locations and each location consists of 16 bits so
	65536 -> 1<<16 -> 2^16*1 (left shift operator)
	total memory taken by VM => 2^16*2(16 bits each) => 131072 bytes => '128KB' memory.
*/

#define max_mem (1<<16)
uint16_t memory[max_mem]; //as shown that 2^16 mem spaces and each of 16 bits
uint16_t reg[r_count]; //each register is of 16 bits too

//termios contains set of attributes which control terminal's I/O behavior
struct termios original_tio;
//two flags cleared => ICANON-canonical input mode, ECHO-echo input character and set so changes do take effect immediately
void disable_input_buffering(){
	tcgetattr(STDIN_FILENO, &original_tio);
	struct termios new_tio=original_tio;
	new_tio.c_lflag &= ~ICANON & ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}
//restores original terminal attributes and store in original_tio
void restore_input_buffering(){
	tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}
//check if there's keypress waiting in input buffer. STDIN_FILENO->standard input. select function blocks until data is available to be read from file descriptor in readfds or timeout occurs. if keypress in input buffer, return non-zero value else zero.
uint16_t check_key(){
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=0;
	return select(1,&readfds,NULL,NULL, &timeout)!=0;
}
//after interrupt occured restore original attributes of terminal
void handle_interrupt(int signal){
	restore_input_buffering();
	printf("\n");
	exit(-2);
}
//sign extension to retain the sign of the variable to 16 bits 
uint16_t extend_sign(uint16_t val, int bitcount){
	if ((val>>(bitcount-1))&1){ //means if msb is 1 then extend sign
		val=val|(0xFFFF<<bitcount);
	}
	return val;
}

//swap bits of value rightmost 8 bits to leftmost 8 bits and vice versa -> to support little endian
uint16_t swap_16(uint16_t val){
	return (val<<8) | (val>>8);
}

//update flags after each instruction execution
void update_flags(uint16_t re){
	if (reg[re]==0){
		reg[r_cond]=flzero;
	}
	else if (reg[re]>>15==1){ //if msb is 1
		reg[r_cond]=flneg;
	}
	else{
		reg[r_cond]=flpos;
	}
}

//READ DATA FROM THE FILE TO BE USED E.G.,2048.obj IS READ
void read_image_file(FILE* file){
	//origin (or) used to determine where in memory to place the image
	uint16_t or;
	fread(&or, sizeof(or), 1, file);
	or=swap_16(or);
	//already know max size of file so only one fread required
	uint16_t max_read=max_mem-or;
	uint16_t *p=memory+or; //take place in memory of vm
	size_t read=fread(p,sizeof(uint16_t),max_read,file);
	//swap to little endian as mostly modern processors support this endian system
	while (read-- > 0){
		*p=swap_16(*p);
		++p;
	}
}
//check existence if exist-read else return 0
int read_image(const char* image_path){
	FILE* file=fopen(image_path,"rb");
	if (!file){
		return 0;
	}
	read_image_file(file);
	fclose(file);
	return 1;
}

//write in memory
void mem_write(uint16_t addr, uint16_t val){
	memory[addr]=val;
}

//read in memory
uint16_t mem_read(uint16_t addr){
	//if take data from keyboard -> means that address of keyboard is given
	if (addr==mr_kbsr){
		if (check_key()){
			memory[mr_kbsr]=(1<<15);
			memory[mr_kbdr]=getchar();
		}
		else{
			memory[mr_kbsr]=0;
		}
	}
	//now return memory data which is read
	return memory[addr];
}

//------------------------------------------------MAIN DRIVER FUNCTION------------------------------------------------
int main(int argc, const char* argv[]){
	//argc and argv[] -> program's parameters
	if (argc<2){
		//show usage string
		printf("LC-3 [image-file1]..\n");
		exit(2);
	}
	for (int j=1; j<argc;++j){
		if (!read_image(argv[j])){
			printf("Failed to load image: %s\nEXITING PROGRAM!\n",argv[j]);
			exit(1);
		}
	}
	signal(SIGINT, handle_interrupt);
	disable_input_buffering();
	
	//exactly one condition flag should always be set so set zero flag
	reg[r_cond]=flzero;
	
	//set 'PROGRAM COUNTER' pc to starting positions and 0x3000 is default
	enum{ pc_start=0x3000 };
	reg[r_pc]=pc_start;

	printf("\nWELCOME TO OUR VIRTUAL MACHINE FOR LC-3 ARCHITECTURE!\nUSE AWSD KEYS\n A-> left, W->up, S->down, D->right\n\n");
	int run=1;
	while (run){
		//fetch instructions
		uint16_t inst=mem_read(reg[r_pc]);
		reg[r_pc]++; //increment pc
		uint16_t opcode=inst>>12; //opcode first of 4 bits so rightshift as here only opcode->12 to 15 bits
		switch(opcode){
			case op_add:
				{
				  //find destination reg->9 to 11 inclusive->shift 9 right->only leftmost 3 reqd so and 111->7
				  uint16_t r0=(inst>>9) & 0x7;
				  //first operand -> 6 to 8 inclusive
				  uint16_t r1=(inst>>6) & 0x7;
				  //5 bit to check whether immediate mode or reg
				  uint16_t imm_check=(inst>>5) & 0x1;
				  if (imm_check==1){
				  	//immediate mode immediate bits->0 to 4
				  	uint16_t imm5=extend_sign(inst&0x1F,5); //change immediate 5 bits to 16 bits
				  	reg[r0]=reg[r1]+imm5;
				  }
				  else{
				  	//reg 2 -> 0 to 2 bits
				  	uint16_t r2=inst & 0x7;
				  	reg[r0]=reg[r1]+reg[r2];
				  }
				  update_flags(r0);
				}
				break;
			case op_and:
				{
				  //same as we did in the 'add' instruction
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t r1=(inst>>6) & 0x7;
				  uint16_t imm_check=(inst>>5) & 0x1;
				  if (imm_check==1){
				  	//immediate mode
				  	uint16_t imm5=extend_sign(inst & 0x1F, 5);
					reg[r0]=reg[r1]&imm5;
				  }
				  else{
				  	uint16_t r2=inst & 0x7;
				  	reg[r0]=reg[r1]&reg[r2];
				  }
				  update_flags(r0);
				}
				break;
			case op_not:
				{
				  //find destination reg and source reg like we did before
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t r1=(inst>>6) & 0x7;
				  reg[r0]=~reg[r1];
				  update_flags(r0);
				}
				break;
			case op_br:
				{
				  //branch instruction where jump to offset from 0 to 8 -> 9 bits
				  uint16_t pc_offset=extend_sign(inst & 0x1FF, 9);
				  //condition flags from 9 to 11 -> 3 bits
				  uint16_t cond_flag=(inst>>9) & 0x7;
				  if (cond_flag & reg[r_cond]){
				  	//if condition reqd by branch is met by the result's condition produced by the result
				  	reg[r_pc]+=pc_offset;
				  }
				}
				break;
			case op_jmp:
				{
				  //jump from 6 to 8 unconditionally -> 3 bits
				  //handles return as well
				  uint16_t r1=(inst>>6) & 0x7;
				  reg[r_pc]=reg[r1];
				}
				break;
			case op_jsr:
				{
				  //check for bit 11 flag -> 0=jump to subroutine 1=offset given
				  uint16_t fl_11=(inst>>11) & 1;
				  reg[r_r7]=reg[r_pc]; //to retain the state of vm
				  if (fl_11==1){
				  	//0 to 10 offset given -> 11 bits
				  	uint16_t pc_offset=extend_sign(inst & 0x7FF,11);
				  	reg[r_pc]=reg[r_pc]+pc_offset;
				  }
				  else{
				  	//subroutine addr -> 6 to 8 bits -> 3 bits
				  	uint16_t r1=(inst>>6)&0x7;
				  	reg[r_pc]=reg[r1];
				  }
				}
				break;
			case op_ld:
				{
				  //load direct
				  //9 to 11->destination reg->3 bits
				  //0 to 8->offset->9 bits
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t pc_offset=extend_sign(inst & 0x1FF,9);
				  reg[r0]=mem_read(reg[r_pc]+pc_offset);
				  update_flags(r0);
				}
				break;
			case op_ldi:
				{
				  //load indirect
				  //9 to 11->destination reg->3 bits
				  //0 to 8->offset->9 bits
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t pc_offset=extend_sign(inst & 0x1FF, 9);
				  //add pc_offset to current_pc, look at mem location to get the final address of the value to be fetched
				  reg[r0]=mem_read( mem_read(reg[r_pc]+pc_offset) );
				  update_flags(r0);
				}
				break;
			case op_ldr:
				{
				  //load base+offset
				  //0 to 5->offset->6 bits
				  //8 to 6->base->3 bits
				  //9 to 11->destination reg->3 bits
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t r1=(inst>>6) & 0x7;
				  uint16_t offset=extend_sign(inst & 0x3F, 6);
				  reg[r0]=mem_read(reg[r1]+offset);
				  update_flags(r0);
				}
				break;
			case op_lea:
				{
				  //load effective address
				  //destination reg->9 to 11->3 bits
				  //offset->0 to 8->9 bits
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t pc_offset=extend_sign(inst & 0x1FF,9);
				  reg[r0]=reg[r_pc]+pc_offset;
				  update_flags(r0);
				}
				break;
			case op_st:
				{
				  //store
				  //0 to 8->offset->9 bits
				  //9 to 11->source reg->3 bits
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t pc_offset=extend_sign(inst & 0x1FF,9);
				  mem_write(reg[r_pc]+pc_offset, reg[r0]);
				}
				break;
			case op_sti:
				{
				  //store indirect
				  //0 to 8->offset->9 bits
				  //9 to 11->source reg->3 bits
				  uint16_t r0=(inst>>9) & 0x7;
				  uint16_t pc_offset=extend_sign(inst & 0x1FF, 9);
				  mem_write( mem_read(reg[r_pc]+pc_offset) ,reg[r0]);
				}
				break;
			case op_str:
				{
				  //store base+offset
				  //0 to 5->offset->6 bits
				  //8 to 6->base->3 bits
				  //9 to 11->source reg->3 bits
				  uint16_t r0=(inst>>9) & 0x7; //source reg
				  uint16_t r1=(inst>>6) & 0x7;
				  uint16_t offset=extend_sign(inst & 0x3F, 6);
				  mem_write(reg[r1]+offset,reg[r0]);
				}
				break;
			case op_trap:
				reg[r_r7]=reg[r_pc];
				switch(inst & 0xFF) //0 to 7 trap vector
				{
					case trap_getc:
						//read single ASCII character
						reg[r_r0]=(uint16_t)getchar();
						update_flags(r_r0);
						break;
					case trap_out:
						//output a single character onto screen
						putc((char)reg[r_r0],stdout);
						fflush(stdout);
						break;
					case trap_puts:
						{
						  //one character displayed per word/string in register r_r0
						  uint16_t* ch=memory+reg[r_r0];
						  while (*ch){
							putc((char)*ch,stdout);
							++ch;
						  }
						  fflush(stdout);
						}
						break;
					case trap_in:
						{
						  //input a character with echo onto terminal
						  printf("Enter a character: ");
						  char c=getchar();
						  putc(c,stdout);
						  fflush(stdout);
						  reg[r_r0]=(uint16_t)c;
						  update_flags(r_r0);
						}
						break;
					case trap_putsp:
						{
						  //output byte string 
						  //one char per byte (two bytes per word). we need to swap back to big endian format to print string
						  uint16_t* c=memory+reg[r_r0];
						  while(*c){
							char char1=(*c)&0xFF;
							putc(char1,stdout);
							char char2=(*c)>>8;
							if (char2) putc(char2,stdout);
							++c;
						  }
						  fflush(stdout);
						}
						break;
					case trap_halt:
						puts("HALT");
						puts("THANK YOU!");
						fflush(stdout);
						run=0;
						break;
				}
				break;
			case op_res:
			case op_rti:
			default:
				//unused intructions
				abort();
				break;

		}
	}
	restore_input_buffering();
}
