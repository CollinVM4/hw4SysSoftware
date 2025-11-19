/*
Assignment:
HW4 - Parser and Code Generator for PL/0
Author(s): Collin Van Meter, Jadon Milne
Language: C (only)
To Compile:
Scanner:
gcc -O2 -std=c11 -o lex lex.c
Parser/Code Generator:
gcc -O2 -std=c11 -o parsercodegen_complete parsercodegen_complete.c
To Execute (on Eustis):
./lex <input_file.txt>
./parsercodegen_complete
where:
<input_file.txt> is the path to the PL/0 source program
Notes:
- lex.c accepts ONE command-line argument (input PL/0 source file)
- parsercodegen_complete.c accepts NO command-line arguments
- Input filename is hard-coded in parsercodegen_complete.c
- Implements recursive-descent parser for full PL/0 grammar (with procedures and if-else)
- Generates PM/0 assembly code (see Appendix A for ISA)
- All development and testing performed on Eustis
Class: COP3402 - System Software - Fall 2025
Instructor: Dr. Jie Lin
Due Date: Friday, November 21, 2025 at 11:59 PM ET
*/

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constants
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_CODE_LENGTH 1000
#define MAX_TOKENS 1000
#define MAX_IDENT_LEN 12
#define MAX_NUMBER_LEN 5
#define TOKEN_FILENAME "tokens.txt"
#define CODE_FILENAME "elf.txt"

// Enum Definitions
enum token_type {
    skipsym = 1, identsym, numbersym, plussym, minussym,
    multsym, slashsym, eqlsym, neqsym,
    lessym, leqsym, gtrsym, geqsym, lparentsym,
    rparentsym, commasym, semicolonsym, periodsym, becomessym,
    beginsym, endsym, ifsym, fisym, thensym, whilesym,
    dosym, callsym, constsym, varsym, procsym,
    writesym, readsym, elsesym, evensym
};

enum opcode {
    LIT = 1, OPR, LOD, STO, CAL, INC, JMP, JPC, SYS
};

enum symbol_kind {
    CONSTANT = 1, VARIABLE = 2, PROCEDURE = 3
};

// Struct Definitions
typedef struct {
    int kind; // const = 1, var = 2, proc = 3
    char name[MAX_IDENT_LEN]; // symbol name
    int val; // value for constants
    int level; // scope level
    int addr; // address (or code index for procedures)
    int mark; // marked for deletion (0 = valid, 1 = invalid)
} symbol;

typedef struct {
    int op; // operation code
    int l;  // lexicographical level
    int m;  // modifier
} instruction;

typedef struct {
    int type; // token type
    char name[MAX_IDENT_LEN]; // identifier name or number string
    int value; // numeric value
} token;

// Global Variables
instruction code[MAX_CODE_LENGTH];
symbol sym_table[MAX_SYMBOL_TABLE_SIZE];
int code_index = 0; // Next available code index
int sym_index = 0;  // Next available symbol table index
int token_list[10000]; // Array to hold all tokens
char token_lexeme[10000][MAX_IDENT_LEN]; // Array to hold lexemes/values
int token_count = 0; // Total tokens read
int token_ptr = 0;   // Current token index
int error_flag = 0;  // Flag to indicate an error has occurred
FILE *code_file;     // File pointer for elf.txt

// The current token's ID, lexeme/value, and numeric value (if applicable)
int current_token;
char current_lexeme[MAX_IDENT_LEN];
int current_number_val; // For numbersym

// Function Prototypes
void read_token_list();
void advance_token();
void emit(int op, int l, int m);
void error(int code);
int find_symbol(const char *name, int level);
int add_symbol(int kind, const char *name, int val, int level, int addr);
void print_assembly_code();
void print_symbol_table();
void unmark_symbols_at_level(int level, int start_index);
void program();
void block(int level, int *data_size);
void const_declaration(int level);
void var_declaration(int level, int *data_size);
void procedure_declaration(int level);
void statement(int level);
void condition(int level);
void expression(int level);
void term(int level);
void factor(int level);

// *** CHANGED ***
// helper: convert a code[] instruction index to a VM code address (cell offset from TOP)
int code_address(int index) {
    return index * 3;
}

// Load tokens from "tokens.txt" into tokenList
void read_token_list()
{
    FILE *fp = fopen(TOKEN_FILENAME, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open input file '%s'. Ensure 'lex.c' was run successfully.\n", TOKEN_FILENAME);
        exit(EXIT_FAILURE);
    }
    token_count = 0;

    // Loop until we can't read another token ID
    while (fscanf(fp, "%d", &token_list[token_count]) == 1) {
        int token_id = token_list[token_count];
        token_lexeme[token_count][0] = '\0'; // initialize
        if (token_id == identsym) {
            if (fscanf(fp, "%s", token_lexeme[token_count]) != 1) {
                fprintf(stderr, "Error: Expected identifier after identsym at token %d\n", token_count);
                break;
            }
        }
        else if (token_id == numbersym) {
            int num_val;
            if (fscanf(fp, "%d", &num_val) != 1) {
                fprintf(stderr, "Error: Expected number after numbersym at token %d\n", token_count);
                break;
            }
            snprintf(token_lexeme[token_count], MAX_IDENT_LEN, "%d", num_val);
        }
        token_count++;
        if (token_count >= MAX_TOKENS) break;
    }
    fclose(fp);
}

// Advance to the next token in the token list
void advance_token() {
    if (error_flag) return;
    if (token_ptr < token_count) {
        current_token = token_list[token_ptr];
        if (current_token == skipsym) {
            error_flag = 1;
            error(1);
            return;
        }
        strncpy(current_lexeme, token_lexeme[token_ptr], MAX_IDENT_LEN);
        current_lexeme[MAX_IDENT_LEN-1] = '\0';
        if (current_token == numbersym) {
            current_number_val = atoi(current_lexeme);
        } else {
            current_number_val = 0;
        }
        token_ptr++;
    } else {
        current_token = skipsym;
        strcpy(current_lexeme, "");
        current_number_val = 0;
    }
}

// Error handling function
void error(int code) {
    if (error_flag) return;
    error_flag = 1;
    char *msg;
    switch (code) {
        case 1: msg = "Error: Scanning error detected by lexer (skipsym present)"; break;
        case 2: msg = "Error: const, var, read, procedure, and call keywords must be followed by identifier"; break;
        case 3: msg = "Error: symbol name has already been declared"; break;
        case 4: msg = "Error: constants must be assigned with ="; break;
        case 5: msg = "Error: constants must be assigned an integer value"; break;
        case 6: msg = "Error: constant and variable declarations must be followed by a semicolon"; break;
        case 7: msg = "Error: undeclared identifier"; break;
        case 8: msg = "Error: only variable values may be altered"; break;
        case 9: msg = "Error: assignment statements must use :="; break;
        case 10: msg = "Error: begin must be followed by end"; break;
        case 11: msg = "Error: if must be followed by then"; break;
        case 12: msg = "Error: else must be followed by fi"; break;
        case 13: msg = "Error: if statement must include else clause"; break;
        case 14: msg = "Error: while must be followed by do"; break;
        case 15: msg = "Error: condition must contain comparison operator"; break;
        case 16: msg = "Error: right parenthesis must follow left parenthesis"; break;
        case 17: msg = "Error: arithmetic equations must contain operands, parentheses, numbers, or symbols"; break;
        case 18: msg = "Error: call statement may only target procedures"; break;
        case 19: msg = "Error: procedure declaration must be followed by a semicolon"; break;
        case 20: msg = "Error: program must end with period"; break;
        default: msg = "Error: Unknown error occurred"; break;
    }
    fprintf(stderr, "%s\n", msg);
    fprintf(code_file, "%s\n", msg);
    fclose(code_file);
    exit(EXIT_SUCCESS);
}

// function to emit instructions
void emit(int op, int l, int m) {
    if (code_index >= MAX_CODE_LENGTH) {
        fprintf(stderr, "Error: Code array overflow.\n");
        exit(EXIT_FAILURE);
    }

    // Add instruction to code array
    code[code_index].op = op;
    code[code_index].l  = l;
    code[code_index].m  = m;
    code_index++; // increment code index
}

// function to print assembly code
void print_assembly_code() {
    // mnemonic def for opcodes
    char *opname[] = {"", "LIT", "OPR", "LOD", "STO", "CAL", "INC", "JMP", "JPC", "SYS"};
    // Print assembly code header
    printf("\nAssembly Code:\n");
    printf("Line\tOP\tL\tM\n");
    // loop through code array and print instructions
    for (int i = 0; i < code_index; i++) {
        printf("%d\t%s\t%d\t%d\n", i, opname[code[i].op], code[i].l, code[i].m);
    }
}

// function to print symbol table
void print_symbol_table() {
    // symbol table header
    printf("\nSymbol Table:\n");
    printf("Kind | Name        | Value | Level | Address | Mark\n");
    printf("--------------------------------------------------\n");
    //loop through symbol table and print entries
    for (int i = 0; i < sym_index; i++) {
        printf("%d    | %-11s | %5d | %5d | %7d | %4d\n",
               sym_table[i].kind, sym_table[i].name, sym_table[i].val,
               sym_table[i].level, sym_table[i].addr, sym_table[i].mark);
    }
}

// function to mark symbols from a specific level as out-of-scope
void unmark_symbols_at_level(int level, int start_index) {
    for (int i = start_index; i < sym_index; i++) {
        if (sym_table[i].level == level) {
            sym_table[i].mark = 1; // Mark as out of scope
        }
    }
}

// writes to elf.txt
void write_code_to_file() {
    // loop through code array and write instructions elf.txt
    for (int i = 0; i < code_index; i++) {
        fprintf(code_file, "%d %d %d\n", code[i].op, code[i].l, code[i].m);
    }
}

// function to find symbol in symbol table, respecting scope
int find_symbol(const char *name, int level) {
    (void)level; // level currently unused, but kept for signature compatibility
    for (int i = sym_index - 1; i >= 0; i--) {
        if (strcmp(sym_table[i].name, name) == 0 && sym_table[i].mark == 0) {
            return i;
        }
    }
    return -1; // not found
}

// function to add symbol to symbol table
int add_symbol(int kind, const char *name, int val, int level, int addr) {
    // overflow
    if (sym_index >= MAX_SYMBOL_TABLE_SIZE) {
        fprintf(stderr, "Error: Symbol table overflow.\n");
        return -1;
    }

    // check for duplicate in current scope
    for (int i = sym_index - 1; i >= 0; i--) {
        if (sym_table[i].level < level) break; // Exited current scope
        if (strcmp(sym_table[i].name, name) == 0 && sym_table[i].mark == 0) {
            error(3); // duplicate symbol
            return -1;
        }
    }

    // add symbol to table
    sym_table[sym_index].kind = kind;
    strncpy(sym_table[sym_index].name, name, MAX_IDENT_LEN);
    sym_table[sym_index].name[MAX_IDENT_LEN - 1] = '\0';
    sym_table[sym_index].val   = val;
    sym_table[sym_index].level = level;
    sym_table[sym_index].addr  = addr; // note: for procedures this is a *code index*
    sym_table[sym_index].mark  = 0;    // Mark as valid (in scope)
    return sym_index++; // increment symbol index upon return
}


// GRAMMAR DEFINITIONS AND PARSING FUNCTIONS

void program() {
    int data_size; // initialize data size
    block(0, &data_size); // parse main block at level 0
    // ensure program ends with period
    if (current_token != periodsym) {
        error(20);
    }
    emit(SYS, 0, 3); // halt instruction
}

void block(int level, int *data_size) {
    *data_size = 3; // reserve space for static link, dynamic link, return address
    int start_sym_index = sym_index;
    int jmp_addr = code_index;

    if (level == 0) {
        emit(JMP, 0, 0); // Placeholder for main
    }

    const_declaration(level);
    var_declaration(level, data_size);
    procedure_declaration(level);

    if (level == 0) {
        // *** CHANGED *** patch main JMP with VM code address
        code[jmp_addr].m = code_address(code_index); // start of main
    }

    emit(INC, 0, *data_size); // allocate space for variables
    statement(level);

    unmark_symbols_at_level(level, start_sym_index);

    if (level > 0) {
        emit(OPR, 0, 0); // RTN (Return from procedure)
    }
}

void const_declaration(int level) {
    if (current_token == constsym) {
        advance_token();
        // process constant declarations
        do {
            if (current_token != identsym) {
                error(2);
            }
            char ident_name[MAX_IDENT_LEN];
            strcpy(ident_name, current_lexeme);
            advance_token();
            if (current_token != eqlsym) {
                error(4);
            }
            advance_token();
            if (current_token != numbersym) {
                error(5);
            }
            int val = current_number_val;
            add_symbol(CONSTANT, ident_name, val, level, 0);
            advance_token();
        } while (current_token == commasym && (advance_token(), 1));
        if (current_token != semicolonsym) {
            error(6);
        }
        advance_token();
    }
}

void var_declaration(int level, int *data_size) {
    // Handle variable declarations
    if (current_token == varsym) {
        advance_token();
        do {
            if (current_token != identsym) {
                error(2);
            }
            add_symbol(VARIABLE, current_lexeme, 0, level, *data_size);
            (*data_size)++;
            advance_token();
        } while (current_token == commasym && (advance_token(), 1));
        if (current_token != semicolonsym) {
            error(6);
        }
        advance_token();
    }
}

void procedure_declaration(int level) {
    while (current_token == procsym) {
        advance_token();
        if (current_token != identsym) {
            error(2);
        }
        char proc_name[MAX_IDENT_LEN];
        strcpy(proc_name, current_lexeme);

        // Add procedure to symbol table, addr is its starting code index
        add_symbol(PROCEDURE, proc_name, 0, level, code_index);
        advance_token();

        if (current_token != semicolonsym) {
            error(19);
        }
        advance_token();

        int proc_data_size;
        block(level + 1, &proc_data_size);

        if (current_token != semicolonsym) {
            error(19);
        }
        advance_token();
    }
}

void statement(int level) {
    int sym_idx;
    int cx1, cx2;

    // Handle different statement types
    if (current_token == identsym) {
        char ident_name[MAX_IDENT_LEN];
        strcpy(ident_name, current_lexeme);
        sym_idx = find_symbol(ident_name, level);
        if (sym_idx == -1) {
            error(7);
        }
        if (sym_table[sym_idx].kind != VARIABLE) {
            error(8);
        }
        advance_token();
        if (current_token != becomessym) {
            error(9);
        }
        advance_token();
        expression(level);
        emit(STO, level - sym_table[sym_idx].level, sym_table[sym_idx].addr);
    } else if (current_token == callsym) {
        advance_token();
        if (current_token != identsym) {
            error(2);
        }
        sym_idx = find_symbol(current_lexeme, level);
        if (sym_idx == -1) {
            error(7);
        }
        if (sym_table[sym_idx].kind != PROCEDURE) {
            error(18);
        }
        // *** CHANGED ***: convert procedure code index to VM code address
        emit(CAL, level - sym_table[sym_idx].level, code_address(sym_table[sym_idx].addr));
        advance_token();
    } else if (current_token == readsym) {// read statement
        advance_token();
        if (current_token != identsym) {
            error(2);
        }
        sym_idx = find_symbol(current_lexeme, level);
        if (sym_idx == -1) {
            error(7);
        }
        if (sym_table[sym_idx].kind != VARIABLE) {
            error(8);
        }
        emit(SYS, 0, 2);
        emit(STO, level - sym_table[sym_idx].level, sym_table[sym_idx].addr);
        advance_token();
    } else if (current_token == writesym) {// write statement
        advance_token();
        expression(level);
        emit(SYS, 0, 1);
    } else if (current_token == beginsym) {// begin...end block
        advance_token();
        statement(level);
        while (current_token == semicolonsym) {
            advance_token();
            statement(level);
        }
        if (current_token != endsym) {
            error(10);
        }
        advance_token();
    } else if (current_token == ifsym) {// if...then...else...fi statement
        advance_token();
        condition(level);
        if (current_token != thensym) {
            error(11);
        }
        advance_token();
        cx1 = code_index;
        emit(JPC, 0, 0); // to be patched
        statement(level);

        cx2 = code_index;
        emit(JMP, 0, 0); // to be patched
        // *** CHANGED ***: patch JPC to ELSE-part address
        code[cx1].m = code_address(code_index);

        if (current_token != elsesym) {
            error(13);
        }
        advance_token();
        statement(level);

        // *** CHANGED ***: patch JMP to FI (after else) address
        code[cx2].m = code_address(code_index);

        if (current_token != fisym) {
            error(12);
        }
        advance_token();
    } else if (current_token == whilesym) {// while...do statement
        advance_token();
        cx1 = code_index; // loop start
        condition(level);
        if (current_token != dosym) {
            error(14);
        }
        advance_token();
        cx2 = code_index;
        emit(JPC, 0, 0); // exit test, to patch
        statement(level);
        // *** CHANGED ***: back-edge jump to start of loop
        emit(JMP, 0, code_address(cx1));
        // *** CHANGED ***: patch JPC to jump to instruction after loop body
        code[cx2].m = code_address(code_index);
    }
}

void condition(int level) {
    if (current_token == evensym) {
        advance_token();
        expression(level);
        emit(OPR, 0, 11); // EVEN per ISA
    } else {
        expression(level); // left-hand side
        int rel_op = current_token;
        if (rel_op < eqlsym || rel_op > geqsym) {
            error(15);
        }
        advance_token(); // consume relational operator
        expression(level); // right-hand side
        // Emit appropriate OPR instruction based on relational operator
        // Per ISA Table 2: EQL=5, NEQ=6, LSS=7, LEQ=8, GTR=9, GEQ=10
        switch (rel_op) {
            case eqlsym: emit(OPR, 0, 5); break; // EQL
            case neqsym: emit(OPR, 0, 6); break; // NEQ
            case lessym: emit(OPR, 0, 7); break; // LSS
            case leqsym: emit(OPR, 0, 8); break; // LEQ
            case gtrsym: emit(OPR, 0, 9); break; // GTR
            case geqsym: emit(OPR, 0, 10); break; // GEQ
        }
    }
}

void expression(int level) {
    int op;
    term(level);
    // Handle addition and subtraction
    while (current_token == plussym || current_token == minussym) {
        op = current_token;
        advance_token();
        term(level);
        if (op == plussym) {
            emit(OPR, 0, 1); // ADD
        } else {
            emit(OPR, 0, 2); // SUB
        }
    }
}

void term(int level) {
    int op;
    factor(level);
    // Handle multiplication and division
    while (current_token == multsym || current_token == slashsym) {
        op = current_token;
        advance_token();
        factor(level);
        if (op == multsym) {
            emit(OPR, 0, 3); // MUL
        } else {
            emit(OPR, 0, 4); // DIV
        }
    }
}

void factor(int level) {
    int sym_idx;
    // Handle identifier, number, or parenthesized expression
    if (current_token == identsym) {
        sym_idx = find_symbol(current_lexeme, level);
        if (sym_idx == -1) {
            error(7);
        }
        // Load constant or variable value
        if (sym_table[sym_idx].kind == CONSTANT) {
            emit(LIT, 0, sym_table[sym_idx].val);
        } else if (sym_table[sym_idx].kind == VARIABLE) {
            emit(LOD, level - sym_table[sym_idx].level, sym_table[sym_idx].addr);
        }
        advance_token();
    } else if (current_token == numbersym) {
        emit(LIT, 0, current_number_val);
        advance_token();
    } else if (current_token == lparentsym) {
        advance_token();
        expression(level);
        if (current_token != rparentsym) {
            error(16);
        }
        advance_token();
    } else {
        error(17);
    }
}

// --- MAIN FUNCTION ---
int main(void) {
    code_file = fopen(CODE_FILENAME, "w"); // Open output file
    if (!code_file) { // Check for file open error
        fprintf(stderr, "Error: Could not open output file '%s'.\n",
                CODE_FILENAME);
        return EXIT_FAILURE;
    }
    read_token_list(); // Load tokens from file

    // Check if any tokens were read
    if (token_count == 0) {
        fprintf(stderr, "Error: Token input file '%s' is empty or invalid.\n",
                TOKEN_FILENAME);
        fprintf(code_file, "Error: Token input file '%s' is empty or invalid.\n",
                TOKEN_FILENAME);
        fclose(code_file);
        return EXIT_SUCCESS;
    }
    advance_token(); // Initialize first token
    if (current_token == skipsym) {
        error(1);
    }
    program(); // Start parsing
    if (!error_flag) {
        print_assembly_code();
        print_symbol_table();
        write_code_to_file();
    }
    fclose(code_file); //Finished wooooo
    return EXIT_SUCCESS;
}
