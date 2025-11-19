/*
Assignment:
vm.c - Implement a P-machine virtual machine

Authors: Collin Van Meter, Jadon Milne

Language: C ( only )

To Compile:
    gcc -O2 -Wall -std=c11 -o vm vm.c

To Execute ( on Eustis ):
    ./vm input.txt

where:
    input.txt is the name of the file containing PM/0 instructions;
    each line has three integers (OP, L, M)

Notes:
    - Implements the PM/0 virtual machine described in the homework
    instructions.
    - No dynamic memory allocation or pointer arithmetic.
    - Does not implement any VM instruction using a separate function.
    - Runs on Eustis.

Class: COP3402 - Systems Software - Fall 2025

Instructor: Dr. Jie Lin

Due Date: Friday, September 12th, 2025
*/

// libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// variables 
#define PAS_SIZE 500 // as defined by section 3 (instructions file)
#define TOP (PAS_SIZE - 1) // tracks top of code segment
int CODE_FLOOR = PAS_SIZE; // tracks bottom of code segment
int pas[PAS_SIZE] = {0}; // global program address space
const char* op_mnemonics[] = {"LIT", "OPR", "LOD", "STO", "CAL", "INC", "JMP", "JPC", "SYS"};


// Instruction Register (IR)
typedef struct instruction {
    int op;
    int l;
    int m;
} instruction;


// this is written by professor
/* Find base L levels down from the current activation record */
int base(int BP, int L) 
{
    int arb = BP;          // activation record base
    while (L > 0) 
    {
        arb = pas[arb];    // follow static link
        L--;
    }
    return arb;
}


//print function
void print_state(int PC, int BP, int SP) {
    printf("%d %d %d ", PC, BP, SP);
    int i, arb = BP;
    for (i = CODE_FLOOR - 1; i >= SP; i--) { 
        if (i == arb) { printf("| "); arb = pas[arb - 1]; }
        printf("%d ", pas[i]);
    }
    printf("\n");
}


int main(int argc, char *argv[]) 
{

    // more than 1 argument check
    if (argc != 2) {
        fprintf(stderr, "ERROR: ONLY USE 1 input.txt\n");
        return 1;
    }

    // open input file
    FILE *input = fopen(argv[1], "r");
    if (!input) 
    {
        perror("error w/ input file");
        return 1;
    }

    // variables for reading instructions
    int op; // operation code
    int L;  // level
    int M;  // modifier
    int addr = PAS_SIZE - 1;
    int instructionCount = 0;
    int lowestUsed = PAS_SIZE;    // last loaded M (track to set SP later)

    // read from file, load instructions into PAS
    while (fscanf(input, "%d %d %d", &op, &L, &M) == 3) 
    {
        pas[addr--] = op;  // OP
        pas[addr--] = L;   // L
        pas[addr--] = M;   // M
        instructionCount++;

        if (addr + 1 < lowestUsed) lowestUsed = addr + 1; // last written M address
    }
    fclose(input);

    // init registers per assignment details in section 3:
    int PC = PAS_SIZE - 1;   
    int SP = lowestUsed;    
    int BP = SP - 1;
    CODE_FLOOR = SP;

    //fetch-execute cycle
    instruction ir;
    int halt = 0;

    //print values
    printf("Initial values: %d %d %d\n", PC, BP, SP);
    
    // main execution loop
    do{
        //fetch cycle
        ir.op = pas[PC];
        ir.l = pas[PC - 1];
        ir.m = pas[PC - 2];
        PC -= 3;
        
        //print instruction before execution
        int delayFlag = (ir.op == 9); // print sys after we execute it (matches instructions formatting)
        if (!delayFlag) 
        {
            if (ir.op == 2) // OPR (arithmetic operations)
            {
                // array for OPR mnemonics
                static const char* opr_arithmetic[] = 
                {
                    "RTN","ADD","SUB","MUL","DIV","EQL","NEQ","LSS","LEQ","GTR","GEQ"
                };
                const char* name;
                if (ir.m >= 0 && ir.m <= 10) 
                {
                    name = opr_arithmetic[ir.m];
                } 
                else 
                {
                    name = "OPR";
                }
                printf("%s %d %d ", name, ir.l, ir.m);
            } 
            else if (ir.op >= 1 && ir.op <= 9) // other operations
            {
                printf("%s %d %d ", op_mnemonics[ir.op - 1], ir.l, ir.m);
            } 
            else // invalid opcode
            {
                printf("OP%d %d %d ", ir.op, ir.l, ir.m);
            }
        }
       
            
        //execution
        switch(ir.op){
            case 1: //LIT
                SP--;
                pas[SP] = ir.m;
                break;
            case 2: //OPR
                switch(ir.m){
                    case 0://RTN
                        SP = BP + 1;
                        BP = pas[SP - 2];
                        PC = pas[SP - 3];
                        break;
                    case 1://ADD
                        pas[SP + 1] += pas[SP];
                        SP++;
                        break;
                    case 2://SUB
                        pas[SP + 1] -= pas[SP];
                        SP++;
                        break;
                    case 3://MUL
                        pas[SP + 1] *= pas[SP];
                        SP++;
                        break;
                    case 4://DIV
                        pas[SP + 1] /= pas[SP];
                        SP++;
                        break;
                    case 5: //EQL
                        pas[SP + 1] = (pas[SP + 1] == pas[SP]);
                        SP++;
                        break;
                    case 6: //NEQ
                        pas[SP + 1] = (pas[SP + 1] != pas[SP]);
                        SP++;
                        break;
                    case 7://LSS
                        pas[SP + 1] = (pas[SP + 1] < pas[SP]);
                        SP++;
                        break;
                    case 8: //LEQ
                        pas[SP + 1] = (pas[SP + 1] <= pas[SP]);
                        SP++;
                        break;
                    case 9: //GTR
                        pas[SP + 1] = (pas[SP + 1] > pas[SP]);
                        SP++;
                        break;
                    case 10: //GEQ
                        pas[SP + 1] = (pas[SP + 1] >= pas[SP]);
                        SP++;
                        break;
                }
                break;
            case 3: //LOD
                SP--;
                pas[SP] = pas[base(BP, ir.l) - ir.m];
                break;
            case 4: //STO
                pas[base(BP, ir.l) - ir.m] = pas[SP];
                SP++;
                break;
            case 5: //CAL
                pas[SP - 1] = base(BP, ir.l); //SL
                pas[SP - 2] = BP;             //DL
                pas[SP - 3] = PC;             //RA
                BP = SP - 1;
                PC = TOP - ir.m;
                break;
            case 6: //INC
                SP -= ir.m;
                break;
            case 7: //JMP
                PC = (TOP - ir.m);
                break;
            case 8: //JPC
                if (pas[SP] == 0) {
                    PC = TOP - ir.m;
                }
                SP++;
                break;
            case 9: // SYS
                switch (ir.m) {
                    case 1: // output
                        printf("Output result is: %d\n", pas[SP]);
                        SP++;
                        break;
                    case 2: // read
                        printf("Please Enter an Integer: ");
                        SP--;
                        if (scanf("%d", &pas[SP]) != 1) 
                        {
                            fprintf(stderr, "failure to read integer\n");
                            return 1;
                        }
                        break;
                    case 3: // hlt
                        halt = 1;
                        break;
                    default:
                        fprintf(stderr, "runtime error: invalid SYS m=%d\n", ir.m);
                        return 1;
                }
                // print delayed for SYS
                printf("SYS %d %d ", ir.l, ir.m);
                print_state(PC, BP, SP);
                continue;  
            }       
            print_state(PC, BP, SP); // print state for current execution
        } while (!halt);
    return 0;
}
