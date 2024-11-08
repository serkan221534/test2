#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "ozu-riscv32.h"


/***************************************************************/
/* Print out a list of commands available                      */
/***************************************************************/
void help() {        
    printf("------------------------------------------------------------------\n\n");
    printf("\t**********OZU-RV32 Disassembler and Simulator Help MENU**********\n\n");
    printf("sim\t-- simulate program to completion \n");
    printf("run <n>\t-- simulate program for <n> instructions\n");
    printf("rdump\t-- dump register values\n");
    printf("reset\t-- clears all registers/memory and re-loads the program\n");
    printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
    printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
    printf("print\t-- print the program loaded into memory\n");
    printf("?\t-- display help menu\n");
    printf("quit\t-- exit the simulator\n\n");
    printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                              */
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
/* Write a 32-bit word to memory                               */
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
/* Execute one cycle                                           */
/***************************************************************/
void cycle() {                                                
    handle_instruction();
    CURRENT_STATE = NEXT_STATE;
    INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate RISC-V for n cycles                                */
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
/* simulate to completion                                      */
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

/**************************************************************************************/ 
/* Dump region of memory to the terminal (make sure provided address is word aligned) */
/**************************************************************************************/
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
/* Dump current values of registers to the teminal             */   
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
    for (i = 0; i < RISCV_REGS; i++){
        printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
    }
    printf("-------------------------------------\n");
    
}

/***************************************************************/
/* Read a command from standard input.                         */  
/***************************************************************/
void handle_command() {                         
    char buffer[20];
    uint32_t start, stop, cycles;
    uint32_t register_no;
    int register_value;

    printf("OZU-RISCV SIM:> ");

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
            printf("Exiting OZU-RISCV! Good Bye...\n");
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
/* reset registers/memory and reload program                   */
/***************************************************************/
void reset() {   
    int i;
    /*reset registers*/
    for (i = 0; i < RISCV_REGS; i++){
        CURRENT_STATE.REGS[i] = 0;
    }
    
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
/* Allocate and set memory to zero                             */
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
/* load program into memory                                   */
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
/* decode and execute instruction                           */ 
/************************************************************/
void handle_instruction()
{
    uint32_t current_ins = mem_read_32(CURRENT_STATE.PC);
    uint32_t opcode = current_ins & 0x7F;
    uint32_t rd, funct3, rs1, rs2, funct7, imm, shamt;
    int32_t imm_sext;


    NEXT_STATE.PC = CURRENT_STATE.PC + 4; 

    if (opcode == 0x33) { //R-type instructions
        uint32_t rd = (current_ins >> 7) & 0x1F; 
        uint32_t funct3 = (current_ins >> 12) & 0x07; 
        uint32_t rs1 = (current_ins >> 15) & 0x1F; 
        uint32_t rs2 = (current_ins >> 20) & 0x1F; 
        uint32_t funct7 = (current_ins >> 25) & 0x7F; 

        if (funct7 == 0x00) { 
            if (funct3 == 0x0) { // ADD
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] + CURRENT_STATE.REGS[rs2];
            } else if (funct3 == 0x1) { // SLL or shift left logical   
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] << (CURRENT_STATE.REGS[rs2] & 0x1F);
            } else if (funct3 == 0x2) { // SLT or set less than
                NEXT_STATE.REGS[rd] = ((int32_t)CURRENT_STATE.REGS[rs1] < (int32_t)CURRENT_STATE.REGS[rs2]) ? 1 : 0;
            } else if (funct3 == 0x4) { // XOR
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] ^ CURRENT_STATE.REGS[rs2];
            } else if (funct3 == 0x5) { // SRL or shift right logical
                 NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] >> (CURRENT_STATE.REGS[rs2] & 0x1F);
            } else if (funct3 == 0x6) { // OR
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] | CURRENT_STATE.REGS[rs2];
            } else if (funct3 == 0x7) { // AND
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] & CURRENT_STATE.REGS[rs2];
            }
        } else if (funct7 == 0x20) {
            if (funct3 == 0x0) { // SUB
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] - CURRENT_STATE.REGS[rs2];
            } else if (funct3 == 0x5) { // SRA or shift right arithmetic
                NEXT_STATE.REGS[rd] = ((int32_t)CURRENT_STATE.REGS[rs1]) >> (CURRENT_STATE.REGS[rs2] & 0x1F);
            }
        } else if (funct7 == 0x01) {
            if (funct3 == 0x0) { // MUL
                NEXT_STATE.REGS[rd] = (int32_t)CURRENT_STATE.REGS[rs1] * (int32_t)CURRENT_STATE.REGS[rs2];
            } else if (funct3 == 0x4) { // DIV
                    NEXT_STATE.REGS[rd] = (int32_t)CURRENT_STATE.REGS[rs1] / (int32_t)CURRENT_STATE.REGS[rs2];
            } else if (funct3 == 0x5) { // DIVU
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] / CURRENT_STATE.REGS[rs2];
            }
        }
    }
    
    else if (opcode == 0x13) {// I-type instructions
        rd = (current_ins >> 7) & 0x1F;
        funct3 = (current_ins >> 12) & 0x07;
        rs1 = (current_ins >> 15) & 0x1F;
        imm = (current_ins >> 20); 
        imm_sext = ((int32_t)current_ins) >> 20; 

        if (funct3 == 0x0) { // ADDI
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] + imm_sext;
        } else if (funct3 == 0x2) { // SLTI
            NEXT_STATE.REGS[rd] = ((int32_t)CURRENT_STATE.REGS[rs1] < imm_sext) ? 1 : 0;
        } else if (funct3 == 0x4) { // XORI
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] ^ imm_sext;
        } else if (funct3 == 0x6) { // ORI
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] | imm_sext;
        } else if (funct3 == 0x7) { // ANDI
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] & imm_sext;
        } else if (funct3 == 0x1) { // SLLI
            shamt = imm & 0x1F;
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] << shamt;
        } else if (funct3 == 0x5) {
            shamt = imm & 0x1F;
            funct7 = (current_ins >> 25) & 0x7F;
            if (funct7 == 0x00) { // SRLI 
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] >> shamt;
            } else if (funct7 == 0x20) { // SRAI
                NEXT_STATE.REGS[rd] = ((int32_t)CURRENT_STATE.REGS[rs1]) >> shamt;
            }
        }
    }

    else if (opcode == 0x03) {// I-type load instructions
        rd = (current_ins >> 7) & 0x1F;
        funct3 = (current_ins >> 12) & 0x07;
        rs1 = (current_ins >> 15) & 0x1F;
        imm_sext = ((int32_t)current_ins) >> 20; // sign extend immediate
        uint32_t addr = CURRENT_STATE.REGS[rs1] + imm_sext;
        uint32_t data = mem_read_32(addr); // Read data once

        if (funct3 == 0x0) { // LB
            NEXT_STATE.REGS[rd] = (int8_t)data;
        } else if (funct3 == 0x1) { // LH
            NEXT_STATE.REGS[rd] = (int16_t)data;
        } else if (funct3 == 0x2) { // LW
            NEXT_STATE.REGS[rd] = (int32_t)data;
        } else if (funct3 == 0x4) { // LBU
            NEXT_STATE.REGS[rd] = (uint8_t)data;
        } else if (funct3 == 0x5) { // LHU
            NEXT_STATE.REGS[rd] = (uint16_t)data;
        }    
    }

    else if (opcode == 0x23) {// S-type store instructions
        funct3 = (current_ins >> 12) & 0x07;
        rs1 = (current_ins >> 15) & 0x1F;
        rs2 = (current_ins >> 20) & 0x1F;

        uint32_t imm = ((current_ins >> 7) & 0x1F) | (((current_ins >> 25) & 0x7F) << 5); 
        int32_t imm_sext = ((int32_t)(imm << 20)) >> 20; 
        uint32_t addr = CURRENT_STATE.REGS[rs1] + imm_sext; 
        uint32_t data = CURRENT_STATE.REGS[rs2]; 

        if (funct3 == 0x0) { // SB: store byte
            uint32_t byte_offset = addr % 4; 
            uint32_t word = mem_read_32(addr & ~0x3); 
            word = (word & ~(0xFF << (byte_offset * 8))) | ((data & 0xFF) << (byte_offset * 8)); 
            mem_write_32(addr & ~0x3, word); 
        } else if (funct3 == 0x1) { // SH: store half-word
            uint32_t byte_offset = addr % 4;
            uint32_t word = mem_read_32(addr & ~0x3); 
            word = (word & ~(0xFFFF << (byte_offset * 8))) | ((data & 0xFFFF) << (byte_offset * 8));
            mem_write_32(addr & ~0x3, word);
        } else if (funct3 == 0x2) { // SW: store word
            mem_write_32(addr, data);
        }
    }
    
    else if (opcode == 0x63) { // SB-type branch instructions
        funct3 = (current_ins >> 12) & 0x7;
        rs1 = (current_ins >> 15) & 0x1F;
        rs2 = (current_ins >> 20) & 0x1F;

        int32_t imm_12 = ((current_ins >> 31) & 0x1) << 12; 
        int32_t imm_11 = ((current_ins >> 7)  & 0x1) << 11; 
        int32_t imm_10_5 = ((current_ins >> 25) & 0x3F) << 5; 
        int32_t imm_4_1 = ((current_ins >> 8)  & 0xF)  << 1; 

        int32_t imm = imm_12 + imm_11 + imm_10_5 + imm_4_1; 

        imm = (imm << 19) >> 19; 

        uint32_t target_pc = CURRENT_STATE.PC + imm;

        if (funct3 == 0x0) { // BEQ
            if (CURRENT_STATE.REGS[rs1] == CURRENT_STATE.REGS[rs2]) {
                NEXT_STATE.PC = target_pc;
            }
        } else if (funct3 == 0x1) { // BNE
            if (CURRENT_STATE.REGS[rs1] != CURRENT_STATE.REGS[rs2]) {
                NEXT_STATE.PC = target_pc;
            }
        } else if (funct3 == 0x4) { // BLT
            if ((int32_t)CURRENT_STATE.REGS[rs1] < (int32_t)CURRENT_STATE.REGS[rs2]) {
                NEXT_STATE.PC = target_pc;
            }
        } else if (funct3 == 0x5) { // BGE
            if ((int32_t)CURRENT_STATE.REGS[rs1] >= (int32_t)CURRENT_STATE.REGS[rs2]) {
                NEXT_STATE.PC = target_pc;
            }
        } else if (funct3 == 0x6) { // BLTU
            if (CURRENT_STATE.REGS[rs1] < CURRENT_STATE.REGS[rs2]) {
                NEXT_STATE.PC = target_pc;
            }
        } else if (funct3 == 0x7) { // BGEU
            if (CURRENT_STATE.REGS[rs1] >= CURRENT_STATE.REGS[rs2]) {
                NEXT_STATE.PC = target_pc;
            }
        }
    }
    
    else if (opcode == 0x6F) { // JAL
        uint32_t rd = (current_ins >> 7) & 0x1F;
        int32_t imm = 0;
        int32_t imm20 = ((current_ins >> 31) & 0x1) << 20;   
        int32_t imm10_1 = ((current_ins >> 21) & 0x3FF) << 1; 
        int32_t imm11 = ((current_ins >> 20) & 0x1) << 11;    
        int32_t imm19_12 = ((current_ins >> 12) & 0xFF) << 12; 
        imm = imm20 + imm19_12 + imm11 + imm10_1;
        imm = (imm << 11) >> 11; 
        NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 4;
        NEXT_STATE.PC = CURRENT_STATE.PC + imm;
    }

    else if (opcode == 0x67) { // JALR
        uint32_t rd = (current_ins >> 7) & 0x1F;
        uint32_t funct3 = (current_ins >> 12) & 0x7;
        uint32_t rs1 = (current_ins >> 15) & 0x1F;
        int32_t imm = (int32_t)current_ins >> 20; // Bits [31:20]
        if (funct3 == 0x0) {
            uint32_t temp = CURRENT_STATE.PC + 4;
            NEXT_STATE.PC = (CURRENT_STATE.REGS[rs1] + imm) & ~1; 
            NEXT_STATE.REGS[rd] = temp;
        }
    }

    else if (opcode == 0x37) { // LUI
        uint32_t rd = (current_ins >> 7) & 0x1F;
        int32_t imm = current_ins & 0xFFFFF000; //lower 12 bits are zeros
        NEXT_STATE.REGS[rd] = imm;
    }

    else if (opcode == 0x17) { // AUIPC
        uint32_t rd = (current_ins >> 7) & 0x1F;
        int32_t imm = current_ins & 0xFFFFF000; //makes lower 12 bits zeros
        NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + imm;
    }

    else if (opcode == 0x73) { // ECALL
        CURRENT_STATE.REGS[17] = 0x5D;
        RUN_FLAG = FALSE;
    }  
    NEXT_STATE.REGS[0] = 0;

    /*YOU NEED TO IMPLEMENT THIS*/
    /* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
}


/************************************************************/
/* Initialize Memory                                        */ 
/************************************************************/
void initialize() { 
    init_memory();
    CURRENT_STATE.PC = MEM_TEXT_BEGIN;
    NEXT_STATE = CURRENT_STATE;
    RUN_FLAG = TRUE;
}

/**********************************************************************/
/* Print the program loaded into memory (in RISC-V assembly format)   */ 
/**********************************************************************/
void print_program(){
    int i;
    uint32_t addr;
    
    for(i=0; i<PROGRAM_SIZE; i++){
        addr = MEM_TEXT_BEGIN + (i*4);
        printf("[0x%x]\t", addr);
        print_instruction(addr);
    }
}

/******************************************************************************/
/* Print the instruction at given memory address (in RISC-V assembly format)  */
/******************************************************************************/
void print_instruction(uint32_t addr){
    /*YOU NEED TO IMPLEMENT THIS FUNCTION*/
    uint32_t current_ins = mem_read_32(addr);
    uint32_t opcode = current_ins & 0x7F;
    uint32_t rd, funct3, rs1, rs2, funct7, imm, shamt;
    int32_t imm_sext;

    
    if (opcode == 0x33) {// R-type instructions
        rd = (current_ins >> 7) & 0x1F;
        funct3 = (current_ins >> 12) & 0x07;
        rs1 = (current_ins >> 15) & 0x1F;
        rs2 = (current_ins >> 20) & 0x1F;
        funct7 = (current_ins >> 25) & 0x7F;

        if (funct7 == 0x00) {
            if (funct3 == 0x0) {
                printf("add x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x1) {
                printf("sll x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x2) {
                printf("slt x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x4) {
                printf("xor x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x5) {
                printf("srl x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x6) {
                printf("or x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x7) {
                printf("and x%d, x%d, x%d\n", rd, rs1, rs2);
            }
        } else if (funct7 == 0x20) {
            if (funct3 == 0x0) {
                printf("sub x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x5) {
                printf("sra x%d, x%d, x%d\n", rd, rs1, rs2);
            }
        } else if (funct7 == 0x01) {
            if (funct3 == 0x0) {
                printf("mul x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x4) {
                printf("div x%d, x%d, x%d\n", rd, rs1, rs2);
            } else if (funct3 == 0x5) {
                printf("divu x%d, x%d, x%d\n", rd, rs1, rs2);
            }
        }
    }
    
    else if (opcode == 0x13) {// I-type instructions
        rd = (current_ins >> 7) & 0x1F;
        funct3 = (current_ins >> 12) & 0x07;
        rs1 = (current_ins >> 15) & 0x1F;
        imm_sext = ((int32_t)current_ins) >> 20;

        if (funct3 == 0x0) {
            printf("addi x%d, x%d, %d\n", rd, rs1, imm_sext);
        } else if (funct3 == 0x2) {
            printf("slti x%d, x%d, %d\n", rd, rs1, imm_sext);
        } else if (funct3 == 0x4) {
            printf("xori x%d, x%d, %d\n", rd, rs1, imm_sext);
        } else if (funct3 == 0x6) {
            printf("ori x%d, x%d, %d\n", rd, rs1, imm_sext);
        } else if (funct3 == 0x7) {
            printf("andi x%d, x%d, %d\n", rd, rs1, imm_sext);
        } else if (funct3 == 0x1) {
            shamt = imm_sext & 0x1F;
            printf("slli x%d, x%d, %d\n", rd, rs1, shamt);
        } else if (funct3 == 0x5) {
            shamt = imm_sext & 0x1F;
            funct7 = (current_ins >> 25) & 0x7F;
            if (funct7 == 0x00) {
                printf("srli x%d, x%d, %d\n", rd, rs1, shamt);
            } else if (funct7 == 0x20) {
                printf("srai x%d, x%d, %d\n", rd, rs1, shamt);
            }
        }
    }
    
    else if (opcode == 0x03) { // I-type load instructions
        rd = (current_ins >> 7) & 0x1F;
        funct3 = (current_ins >> 12) & 0x07;
        rs1 = (current_ins >> 15) & 0x1F;
        imm_sext = ((int32_t)current_ins) >> 20;

        if (funct3 == 0x0) {
            printf("lb x%d, %d(x%d)\n", rd, imm_sext, rs1);
        } else if (funct3 == 0x1) {
            printf("lh x%d, %d(x%d)\n", rd, imm_sext, rs1);
        } else if (funct3 == 0x2) {
            printf("lw x%d, %d(x%d)\n", rd, imm_sext, rs1);
        } else if (funct3 == 0x4) {
            printf("lbu x%d, %d(x%d)\n", rd, imm_sext, rs1);
        } else if (funct3 == 0x5) {
            printf("lhu x%d, %d(x%d)\n", rd, imm_sext, rs1);
        }
    }
    
    else if (opcode == 0x23) {// S-type store instructions
        funct3 = (current_ins >> 12) & 0x07;
        rs1 = (current_ins >> 15) & 0x1F;
        rs2 = (current_ins >> 20) & 0x1F;
        imm = ((current_ins >> 7) & 0x1F) | (((current_ins >> 25) & 0x7F) << 5);
        imm_sext = ((int32_t)(imm << 20)) >> 20;

        if (funct3 == 0x0) {
            printf("sb x%d, %d(x%d)\n", rs2, imm_sext, rs1);
        } else if (funct3 == 0x1) {
            printf("sh x%d, %d(x%d)\n", rs2, imm_sext, rs1);
        } else if (funct3 == 0x2) {
            printf("sw x%d, %d(x%d)\n", rs2, imm_sext, rs1);
        }
    }
    
    else if (opcode == 0x63) {// SB-type branch instructions
        funct3 = (current_ins >> 12) & 0x7;
        rs1 = (current_ins >> 15) & 0x1F;
        rs2 = (current_ins >> 20) & 0x1F;

        int32_t imm_12 = ((current_ins >> 31) & 0x1) << 12;
        int32_t imm_11 = ((current_ins >> 7) & 0x1) << 11;
        int32_t imm_10_5 = ((current_ins >> 25) & 0x3F) << 5;
        int32_t imm_4_1 = ((current_ins >> 8) & 0xF) << 1;
        int32_t imm = imm_12 + imm_11 + imm_10_5 + imm_4_1;
        imm = (imm << 19) >> 19;

        if (funct3 == 0x0) {
            printf("beq x%d, x%d, %d\n", rs1, rs2, imm);
        } else if (funct3 == 0x1) {
            printf("bne x%d, x%d, %d\n", rs1, rs2, imm);
        } else if (funct3 == 0x4) {
            printf("blt x%d, x%d, %d\n", rs1, rs2, imm);
        } else if (funct3 == 0x5) {
            printf("bge x%d, x%d, %d\n", rs1, rs2, imm);
        } else if (funct3 == 0x6) {
            printf("bltu x%d, x%d, %d\n", rs1, rs2, imm);
        } else if (funct3 == 0x7) {
            printf("bgeu x%d, x%d, %d\n", rs1, rs2, imm);
        }
    }
    
    else if (opcode == 0x6F) {//JAL
        uint32_t rd = (current_ins >> 7) & 0x1F;
        int32_t imm20 = ((current_ins >> 31) & 0x1) << 20;
        int32_t imm10_1 = ((current_ins >> 21) & 0x3FF) << 1;
        int32_t imm11 = ((current_ins >> 20) & 0x1) << 11;
        int32_t imm19_12 = ((current_ins >> 12) & 0xFF) << 12;
        int32_t imm = imm20 + imm19_12 + imm11 + imm10_1;
        imm = (imm << 11) >> 11;

        printf("jal x%d, %d\n", rd, imm);
    }
    
    else if (opcode == 0x67) {// JALR 
        uint32_t rd = (current_ins >> 7) & 0x1F;
        uint32_t funct3 = (current_ins >> 12) & 0x7;
        uint32_t rs1 = (current_ins >> 15) & 0x1F;
        int32_t imm = (int32_t)current_ins >> 20;

        if (funct3 == 0x0) {
            printf("jalr x%d, x%d, %d\n", rd, rs1, imm);
        }
    }
    
    else if (opcode == 0x37) {// LUI 
        uint32_t rd = (current_ins >> 7) & 0x1F;
        int32_t imm = current_ins & 0xFFFFF000;
        printf("lui x%d, %d\n", rd, imm >> 12);
    }
    
    else if (opcode == 0x17) {// AUIPC 
        uint32_t rd = (current_ins >> 7) & 0x1F;
        int32_t imm = current_ins & 0xFFFFF000;
        printf("auipc x%d, %d\n", rd, imm >> 12);
    }
    
    else if (opcode == 0x73) {// ECALL Instruction
        printf("ecall\n");
    }
}


/***************************************************************/
/* main()                                                      */
/***************************************************************/
int main(int argc, char *argv[]) {                              
    printf("\n********************************\n");
    printf("Welcome to OZU-RISCV SIMULATOR...\n");
    printf("*********************************\n\n");
    
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