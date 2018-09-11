#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	
	//Gives the current instruction in hexadecimal
	uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
	
	//print the instruction
	printf("\nInstruction = %08x \n", instruction);
	
	//Decode the opcode from the instruction
	uint32_t opcode = (0xFC000000 & instruction); //masking the instruction with 1111 1100 0000 0000 0000 0000 0000 0000 (FC000000)
	
	printf("\nOpcode = 0x%08x \n",opcode);
	
	//knowing the type of the opcode
	switch (opcode) {
		
		//R-Type
		case 0x00000000:
		printf("R-type");
		//get the address value of rs 
		uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
		printf("\n rs = 0x%08x", rs);
		
		//get the address value of rt
	    uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
		printf("\n rt = 0x%08x", rt);
		
		//get the address value of rd
		uint32_t rd = (0x0000F800 & instruction) >> 11;//Masking with 0000 0000 0000 0000 1111 1000 0000 0000 (0000F800)
		printf("\n rd = 0x%08x", rd);
		
		//get the value of shift amount
		uint32_t sa = (0x000007C0 & instruction) >> 6;//Masking with 0000 0000 0000 0000 0000 0111 1100 0000 (000007C0) //doubt
		printf("\n sa = %08x", sa);
		
		//decoding the function bit
		uint32_t function = (0x0000003F & instruction); //Masking with 0000 0000 0000 0000 0000 0000 0011 1111 (0000003F)
		printf("\n function = 0x%08x", function);
		
				switch (function) {
					
					//ALU instruction
					
					//ADD function
					case 0x00000020:
					printf("\n This is an ADD function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
					break;
					
					//ADDU function
					case 0x00000021:
					printf("\n This is an ADDU function");
					(uint32_t) NEXT_STATE.REGS[rd] = (uint32_t) CURRENT_STATE.REGS[rs] + (uint32_t) CURRENT_STATE.REGS[rt]; // we need to state it is unsigned
					break;
					
					//SUB function
					case 0x00000022:
					printf("\n This is an SUB function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
					break;
					
					//SUBU function
					case 0x00000023:
					printf("\n This is an SUBU function");
					(uint32_t) NEXT_STATE.REGS[rd] = (uint32_t) CURRENT_STATE.REGS[rs] - (uint32_t) CURRENT_STATE.REGS[rt];
					break;
					
					//AND function
					case 0x00000024:
					printf("\n This is an AND function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
					break;
					
					//OR function
					case 0x00000025:
					printf("\n This is an OR function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
					break;
					
					//XOR function (if rs and rt is different, rd = 1 else if rs and rt are same then rd is 0)
					case 0x00000026:
					printf("\n This is an XOR function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
					break;
					
					//NOR function ( !(rs or rt) )
					case 0x00000027:
					printf("\n This is an NOR function");
					NEXT_STATE.REGS[rd] = !(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
					break;
					
					//SLT function (Set on Less Than, if rs < rt then rd = 1, if rs > rt then rd = 0)
					case 0x0000002A:
					printf("\n This is an SLT function");
					if (CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt])
					{
						NEXT_STATE.REGS[rd] = 0x00000001;
					}
					else 
					{
						NEXT_STATE.REGS[rd] = 0x00000000;
					}						
					
					break;
					
					//SLL function (Shift Left Logical)
					case 0x00000000:
					printf("\n This is an SLL function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << sa; //doubt
					break;
					
					//SRL function (Shit Right Logical)
					case 0x00000002:
					printf("\n This is an SRL function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa; //doubt
					break;
					
					//SRA function (Shift Right Arithmetic)
					case 0x00000003:
					printf("\n This is an SRA function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa; //doubt
					break;
					
					//MULT function (Multiply)
					case 0x00000018:
					printf("\n This is a multiply function");
					int64_t mult_HI = (((int32_t (CURRENT_STATE.REGS[rs] )) *  (int32_t(CURRENT_STATE.REGS[rt]) ))& uint64_t(0xffffffff00000000));
					NEXT_STATE.HI =  int32_t (mult_HI);
					
					int64_t mult_L0 = (((int32_t (CURRENT_STATE.REGS[rs] )) *  (int32_t(CURRENT_STATE.REGS[rt]) ))& uint64_t(0x00000000ffffffff));
					NEXT_STATE.LO = int32_t (mult_LO);
					break;
					
					//MULTU (Multiply unisgned)
					case 0x00000019:
					printf("\n This is a multiply unsinged function");
					uint64_t mult_HI = (((uint32_t (CURRENT_STATE.REGS[rs] )) *  (uint32_t(CURRENT_STATE.REGS[rt]) ))& uint64_t(0xffffffff00000000));
					NEXT_STATE.HI =  uint32_t (mult_HI);
						
					uint64_t mult_L0 = (((uint32_t (CURRENT_STATE.REGS[rs] )) *  (uint32_t(CURRENT_STATE.REGS[rt]) ))& uint64_t(0x00000000ffffffff));
					NEXT_STATE.LO = uint32_t (mult_LO);
					break;
					
					//DIV (Divide function)
					case 0x0000001A:
					printf("\n This is a divide function");
					int32_t (NEXT_STATE.LO) = int32_t(CURRENT_STATE.REGS[rs]) / int32_t(CURRENT_STATE.REGS[rt]);
					int32_t (NEXT_STATE.H1) = int32_t(CURRENT_STATE.REGS[rs]) % int32_t(CURRENT_STATE.REGS[rt]);
					break;
					
					//DIVU (Divide unsigned function)
					case 0x0000001B:
					printf("\n This is a divide unsigned function");
					NEXT_STATE.LO = (CURRENT_STATE.REGS[rs]) / (CURRENT_STATE.REGS[rt]);
					NEXT_STATE.H1 = (CURRENT_STATE.REGS[rs]) % (CURRENT_STATE.REGS[rt]);
					break;
				
					//Load/Store instruction 
					
					//MFHI function (Move from HI)
					case 0x00000010:
					printf("\n This is an MFHI function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.HI; //doubt
					break;
					
					//MFLO function (Move From LO)
					case 0x00000012:
					printf("\n This is an MFLO function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.LO; //doubt
					break;
					
					//MTHI function (Move To HI)
					case 0x00000011:
					printf("\n This is an MTHI function");
					CURRENT_STATE.HI = CURRENT_STATE.REGS[rs]; //doubt
					break;
					
					//MTLO function (Move To LO)
					case 0x00000013:
					printf("\n This is an MTLO function");
					CURRENT_STATE.LO = CURRENT_STATE.REGS[rs]; //doubt
					break;
					
					//Control Flow Instructions
					//JR function (Jump Register)
					case 0x00000008:
					printf("\n This is an JR function"); 
					NEXT_STATE.PC = CURRENT_STATE.REGS[rs];             //doubt
					break;
					
					//JALR function (Jump And Link Register)
					case 0x00000009:
					printf("\n This is an JALR function"); 
					NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 8;            //doubt
					NEXT_STATE.PC = CURRENT_STATE.REGS[rs]              //doubt
					break;
					
					
					//Terminate (They said we should but 10 into $v0 when executing system call)
					case 0x0000000C:
					printf("\n Terminate the program");
					RUN_FLAG = FALSE;
					break;
		}
		
		break;
	
		
					//I-Type
					
					// (ADDI - Add immediate )
					case  0x20000000:
					
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					//get the immediate value and sign extend it
					int32_t immediate_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					
					NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + immediate_value;
						break;
						
						
					//(ADDIU- Add immediate unisigned)
					case 0x24000000:
					
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					uint32_t immediate_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					
					NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + immediate_value;
						break;
						
						
					//(ANDI- AND immediate)
					case 0x30000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					uint32_t immediate_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & immediate_value;
						break;
						
		
					//(ORI- OR immediate)
					case 0x34000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					uint32_t immediate_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | immediate_value;
						break;
		
		
					//(XORI- Exclusive OR immediate)
					case 0x38000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					uint32_t immediate_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] ^ immediate_value;
						break;
						
					
					//(SLTI- Set on less than immediate)
					case 0x28000000:
					
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t immediate_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					if (CURRENT_STATE.REGS[rs] < immediate_value )
					{
						NEXT_STATE.REGS[rt] = 0x00000001;
					}
					else 
					{
						NEXT_STATE.REGS[rt] = 0x00000000;
					}
		
						break;
					
					
					//(LW- Load Word)
					case 0x8C000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					NEXT_STATE.REGS[rt] = int32_t mem_read_32(CURRENT_STATE.REGS[rs] + offset_value);
						break;
					

					
					//(LB- Load Byte)
					case 0x80000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					int8_t(NEXT_STATE.REGS[rt]) = int8_t mem_read_32(CURRENT_STATE.REGS[rs] + offset_value);
						break;
						
					
						
					//(LH- Load half word)
					case 0x84000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					int16_t(NEXT_STATE.REGS[rt]) = int16_t mem_read_32(CURRENT_STATE.REGS[rs] + offset_value);
						break;
					
					
					
					//(LUI- Load upper immediate)
					case 0x3C000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t immediate_value = (0x0000FFFF & instruction)<< 16; // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					int32_t(NEXT_STATE.REGS[rt]) = immediate_value;
						break;
						
					
					//(SW- Store word)
					case 0xAC000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111
					int32_t mem_write_32(CURRENT_STATE.REGS[rs] + offset_value) = int32_t(CURRENT_STATE.REGS[rt]) ;
						break;
						
					
						
					//(SB- Store Byte)
					case 0xA0000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					int8_t mem_write_32(CURRENT_STATE.REGS[rs] + offset_value) = int8_t(CURRENT_STATE.REGS[rt]) ;
						break;
						
					
					
					//(SH- Store Half word)
					case 0xA4000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction); // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					int16_t mem_write_32(CURRENT_STATE.REGS[rs] + offset_value) = int16_t(CURRENT_STATE.REGS[rt]) ;
						break;
					
					
					
					//(BEQ- Branch on Equal)
					case 0x10000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction)<< 2; // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					if (CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt])
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + offset_value) ;
					}
					else 
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + 4) ;
					}						
					
					break;
					
					
					//(BNE- Branch on not Equal)
					case 0x14000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction)<< 2; // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					if (CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt])
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + offset_value) ;
					}
					else 
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + 4) ;
					}						
					
					break;
					
					
					//(BLEZ- Branch on less than or Equal to zero)
					case 0x18000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction)<< 2; // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					if (CURRENT_STATE.REGS[rs] < =  uint32_t(0))
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + offset_value) ;
					}
					else 
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + 4) ;
					}						
					
					break;
					
					
					//(BLTZ- Branch on less than zero)
					case 0x04000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction)<< 2; // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					if (CURRENT_STATE.REGS[rs] < uint32_t(0) && uint32_t rt == 0x00000000)
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + offset_value) ;
					}
					else 
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + 4) ;
					}						
					
					break;
					
					
					//(BGEZ- Branch on Greater than or equal to zero)
					case 0x04000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction)<< 2; // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					if (CURRENT_STATE.REGS[rs] > =  uint32_t(0) && uint32_t rt == 0x00000001 )
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + offset_value) ;
					}
					else 
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + 4) ;
					}						
					
					break;
					
					
					//(BGTZ- Branch on Greater than zero)
					case 0x1C000000:
					//get the address value of rs 
					uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
					printf("\n rs = 0x%08x", rs);
					
					//get the address value of rt
					uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
					printf("\n rt = 0x%08x", rt);
					
					int32_t offset_value = (0x0000FFFF & instruction)<< 2; // Masking with 0000 0000 0000 0000 1111 1111 1111 1111(sign extended)
					if (CURRENT_STATE.REGS[rs] > =  uint32_t(0) && uint32_t rt == 0x00000001 )
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + offset_value) ;
					}
					else 
					{
						NEXT_STATE.PC = (CURRENT_STATE.PC + 4) ;
					}						
					
					break;
		
		break;

		//J-Type
		
					//(Jump Instruction)
					case: 0x08000000
					// determine the upper(31-28)opecode bit 
					uint32_t Op_upper =  (0xf00000000 & instruction); // This is to get the upper 4 bit of the opcode
					
					// determine the target value ie 26 down to 0 bit
					uint32_t target_value = (0x07FFFFFF & instruction)<< 2; // Masking with 0000 0111 1111 1111 1111 1111 1111 1111. (This is to get the 26 bit of the instruction)
					 
					NEXT_STATE.PC = (op_upper | target_value);
					break;
					
					
					//(Jump and link instruction)
					case: 0x0C000000
					NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 8;
					// determine the upper(31-28)opecode bit 
					uint32_t Op_upper =  (0xf00000000 & instruction); // This is to get the upper 4 bit of the opcode
					
					// determine the target value ie 26 down to 0 bit
					uint32_t target_value = (0x07FFFFFF & instruction)<< 2; // Masking with 0000 0111 1111 1111 1111 1111 1111 1111. (This is to get the 26 bit of the instruction)
					 
					NEXT_STATE.PC = (op_upper | target_value);
					break;
		
		
		break;
	}
	
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	/*IMPLEMENT THIS*/
	
	uint32_t instruction = mem_read_32(addr);
	
	//print the instruction
	printf("\nInstruction = %08x \n", instruction);
	
	//Decode the opcode from the instruction
	uint32_t opcode = (0xFC000000 & instruction); //masking the instruction with 1111 1100 0000 0000 0000 0000 0000 0000 (FC000000)
	
	printf("\nOpcode = 0x%08x \n",opcode);
	
	switch (opcode) {
		//R-Type
		case 0x00000000:
		printf("R-type");
	
		//decoding the function bit
		uint32_t function = (0x0000003F & instruction); //Masking with 0000 0000 0000 0000 0000 0000 0011 1111 (0000003F)
		printf("\n function = 0x%08x", function);
		
				switch (function) {
		
				//ADD Instruction
				case 0x00000020:
				printf("Add $Rd $Rs $Rt");
					
				//ADDU function
				case 0x00000021:
				printf("\n This is an ADDU function");
					
					
			    //SUB function
				case 0x00000022:
				printf("\n This is an SUB function");
					
					
				//SUBU function
				case 0x00000023:
				printf("\n This is an SUBU function");
					
					
				//AND function
				case 0x00000024:
				printf("\n This is an AND function");
				break;
					
				//OR function
				case 0x00000025:
				printf("\n This is an OR function");
				break;
					
				//XOR function (if rs and rt is different, rd = 1 else if rs and rt are same then rd is 0)
				case 0x00000026:
				printf("\n This is an XOR function");
				break;
					
				//NOR function ( !(rs or rt) )
				case 0x00000027:
				printf("\n This is an NOR function");
				break;
					
				//SLT function (Set on Less Than, if rs < rt then rd = 1, if rs > rt then rd = 0)
				case 0x0000002A:
				printf("\n This is an NOR function");
				break;	
					
				//SLL function (Shift Left Logical)
				case 0x00000000:
				printf("\n This is an SLL function");
				break;
					
				//SRL function (Shit Right Logical)
				case 0x00000002:
				printf("\n This is an SRL function");
				break;
					
				//SRA function (Shift Right Arithmetic)
				case 0x00000003:
				printf("\n This is an SRA function");
				break;
					
			    //MULT function (Multiply)
				case 0x00000018:
				printf("\n This is a multiply function");
				break;
					
				//MULTU (Multiply unisgned)
				case 0x00000019:
				printf("\n This is a multiply unsinged function");
		        break;
					
				//DIV (Divide function)
				case 0x0000001A:
				printf("\n This is a divide function");
				break;
					
				//DIVU (Divide unsigned function)
				case 0x0000001B:
				printf("\n This is a divide unsigned function");
				break;
				
				//Load/Store instruction 
					
				//MFHI function (Move from HI)
				case 0x00000010:
				printf("\n This is an MFHI function");
				break;
					
				//MFLO function (Move From LO)
				case 0x00000012:
				printf("\n This is an MFLO function");
				break;
					
				//MTHI function (Move To HI)
				case 0x00000011:
				printf("\n This is an MTHI function");
				break;
					
				//MTLO function (Move To LO)
				case 0x00000013:
				printf("\n This is an MTLO function");
				break;
					
				//Control Flow Instructions
				//JR function (Jump Register)
				case 0x00000008:
				printf("\n This is an JR function"); 
				break;
					
				//JALR function (Jump And Link Register)
				case 0x00000009:
				printf("\n This is an JALR function"); 
				break;
						
			}
		break;
       		
		//I-Type
					
				// (ADDI - Add immediate )
				case  0x20000000:
				printf("\n This is an ADDI function"); 	
				break;
						
						
				//(ADDIU- Add immediate unisigned)
				case 0x24000000:
				printf("\n This is an ADDIU function");
				break;
						
						
				//(ANDI- AND immediate)
				case 0x30000000:
				printf("\n This is an AND function");	
				break;
						
		
				//(ORI- OR immediate)
				case 0x34000000:
				printf("\n This is an ORI function");
				break;
		
		
				//(XORI- Exclusive OR immediate)
				case 0x38000000:
				printf("\n This is an XOR function");
				break;
						
					
				//(SLTI- Set on less than immediate)
				case 0x28000000:
				printf("\n This is an SLTI function");	
				break;
					
					
				//(LW- Load Word)
				case 0x8C000000:
				printf("\n This is an LW function");
				break;
					

					
				//(LB- Load Byte)
				case 0x80000000:
				printf("\n This is an LB function");
				break;
					
						
				//(LH- Load half word)
				case 0x84000000:
				printf("\n This is an LH function");
				break;	
					
					
					
				//(LUI- Load upper immediate)
				case 0x3C000000:
				printf("\n This is an LU function");
				break;	
						
					
				//(SW- Store word)
				case 0xAC000000:
				printf("\n This is an SW function");
				break;	
						
					
						
				//(SB- Store Byte)
				case 0xA0000000:
				printf("\n This is an SB function");
				break;	
						
					
				//(SH- Store Half word)
				case 0xA4000000:
				printf("\n This is an SH function");
				break;	
					
					
				//(BEQ- Branch on Equal)
				case 0x10000000:
				printf("\n This is an BEQ function");
				break;	
					
					
				//(BNE- Branch on not Equal)
				case 0x14000000:
				printf("\n This is an BNE function");
				break;	

					
					
				//(BLEZ- Branch on less than or Equal to zero)
				case 0x18000000:
				printf("\n This is an BLEZ function");
				break;
					
					
					
				//(BLTZ- Branch on less than zero)
				case 0x04000000:
				printf("\n This is an BLTZ function");
				break;	
					
					
				//(BGEZ- Branch on Greater than or equal to zero)
				case 0x04000000:
				printf("\n This is an BGEZ function");
				break;
					
					
				//(BGTZ- Branch on Greater than zero)
				case 0x1C000000:
				printf("\n This is an BGTZ function");
				break;	
				
				//J-Type
		
				//(Jump Instruction)
				case: 0x08000000
				printf("\n This is an J function");
				break;	
					
					
				//(Jump and link instruction)
				case: 0x0C000000
				printf("\n This is an JAL function");
				break;	
		
	
	}
	break;
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
