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
	
	printf("\nOpcode = %08x \n",opcode);
	
	//knowing the type of the opcode
	switch (opcode) {
		
		//R-Type
		case 0x00000000:
		printf("R-type");
		//get the address value of rs 
		uint32_t rs = (0x03E00000 & instruction) >> 21;//Masking with 0000 0011 1110 0000 0000 0000 0000 0000 (03E00000)
		printf("\n rs = %x", rs);
		
		//get the address value of rt
	    uint32_t rt = (0x001F0000 & instruction) >> 16;//Masking with 0000 0000 0001 1111 0000 0000 0000 0000 (001F0000)
		printf("\n rt = %x", rt);
		
		//get the address value of rd
		uint32_t rd = (0x0000F800 & instruction) >> 11;//Masking with 0000 0000 0000 0000 1111 1000 0000 0000 (0000F800)
		printf("\n rd = %x", rd);
		
		//get the value of shift amount
		uint32_t sa = (0x000007C0 & instruction) >> 6;//Masking with 0000 0000 0000 0000 0000 0111 1100 0000 (000007C0) //doubt
		printf("\n sa = %x", sa);
		
		//decoding the function bit
		uint32_t function = (0x0000003F & instruction); //Masking with 0000 0000 0000 0000 0000 0000 0011 1111 (0000003F)
		printf("\n function = %x", function);
		
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
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
					break;
					
					//SUB function
					case 0x00000022:
					printf("\n This is an SUB function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
					break;
					
					//SUBU function
					case 0x00000023:
					printf("\n This is an SUBU function");
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
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
						NEXT_STATE.REGS[rd] = 1;
					}
					else 
					{
						NEXT_STATE.REGS[rd] = 0;
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
					PC = CURRENT_STATE.REGS[rs];             //doubt
					break;
					
					//JALR function (Jump And Link Register)
					case 0x00000009:
					printf("\n This is an JALR function"); 
					NEXT_STATE.REGS[rd] = PC + 8;            //doubt
					PC = CURRENT_STATE.REGS[rs]              //doubt
					break;
					
					
					//Terminate
					case 0x0000000C:
					printf("\n Terminate the program");
					RUN_FLAG = FALSE;
					break;
		}
		
		break;
	
		
		//I-Type
		case  
		
		break;

		//J-Type
		case
		
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
