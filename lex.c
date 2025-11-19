/*
Assignment :
lex - Lexical Analyzer for PL /0
Authors: < Collin Van Meter, Jadon Milne >
Language: C (only)
To Compile:
gcc -O2 -std=c11 -o lex lex.c
To Execute (on Eustis):
./lex <input file>
where:
<input file> is the path to the PL /0 source program
Notes:
- Implement a lexical analyser for the PL /0 language.
- The program must detect errors such as
- numbers longer than five digits
- identifiers longer than eleven characters
- invalid characters
- The output format must exactly match the specification.
- Tested on Eustis.
Class: COP 3402 - System Software - Fall 2025
Instructor: Dr. Jie Lin
Due Date: Friday, October 3, 2025 at 11:59 PM ET
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_ID_LEN 11
#define MAX_NUM_LEN 5
#define MAX_SOURCE_SIZE 10000
#define MAX_LEXEMES 500
FILE *fptr;
typedef enum
{
skipsym = 1, identsym, numbersym, plussym, minussym,
multsym, slashsym, eqlsym, neqsym,
lessym, leqsym, gtrsym, geqsym, lparentsym,
rparentsym, commasym, semicolonsym, periodsym, becomessym,
beginsym, endsym, ifsym, fisym, thensym, whilesym,
dosym, callsym, constsym, varsym, procsym,
writesym, readsym, elsesym, evensym
} token_type;
typedef struct
{
char lexeme[MAX_ID_LEN + 1];
int token;
int value;
} lexeme;
const char *reserved[] =
{
"const","var","procedure","call","begin","end","if","fi","then",
"else","while","do","read","write","odd"
};
const int reservedTokens[] =
{
constsym, varsym, procsym, callsym, beginsym, endsym,
ifsym, fisym, thensym, elsesym, whilesym, dosym,
readsym, writesym, evensym
};
const int numReserved = 15;
char source[MAX_SOURCE_SIZE];
lexeme table[MAX_LEXEMES];
int tableIndex = 0;
int isReserved(const char *word)
{
for (int i = 0; i < numReserved; i++)
{
if (strcmp(word, reserved[i]) == 0)
return reservedTokens[i];
}
return 0;
}
void addLexeme(const char *word, int token, int value)
{
// prevents overflow
if (tableIndex >= MAX_LEXEMES) return;
// copy word to lexeme table
strncpy(table[tableIndex].lexeme, word, MAX_ID_LEN);
table[tableIndex].token = token;
table[tableIndex].value = value;
tableIndex++;
}
void error(const int msg, const char *context)
{
if (tableIndex >= MAX_LEXEMES) return;
// copy context to lexeme table (for errors only)
strncpy(table[tableIndex].lexeme, context, MAX_ID_LEN);
table[tableIndex].token = msg;
table[tableIndex].value = 1;
tableIndex++;
}
void handleComment(const char *input, int *i)
{
*i += 2; // Skip the opening "/*"
while (input[*i] != '\0' && !(input[*i] == '*' && input[*i + 1] == '/'))
{
(*i)++;
}
if (input[*i] == '\0')
{
// Unclosed comment - handle gracefully, just return
return;
}
*i += 2;
}
void lexer(const char *input)
{
int i = 0;
// while we don't reach null terminator
while (input[i] != '\0')
{
if (isspace(input[i])) { i++; continue; }
if (input[i] == '/' && input[i + 1] == '*')
{
handleComment(input, &i);
continue;
}
// identifier or reserved word
if (isalpha(input[i]))
{
char buffer[MAX_ID_LEN + 5]; int j = 0;
while (isalnum(input[i]) && j < MAX_ID_LEN) buffer[j++] = input[i++];
buffer[j] = '\0';
// If identifier is too long, set to skipsym
if (isalnum(input[i]))
{
while (isalnum(input[i])) i++; // Skip the rest of the identifier
addLexeme(buffer, skipsym, 0); // Mark as skipsym
continue;
}
int res = isReserved(buffer);
if (res) addLexeme(buffer, res, 0);
else addLexeme(buffer, identsym, 0);
continue;
}
// number
if (isdigit(input[i]))
{
char buffer[MAX_NUM_LEN + 5]; int j = 0;
while (isdigit(input[i]) && j < MAX_NUM_LEN) buffer[j++] = input[i++];
buffer[j] = '\0';
// If number is too long, set to skipsym
if (isdigit(input[i]))
{
while (isdigit(input[i])) i++; // Skip the rest of the number
addLexeme(buffer, skipsym, 0); // Mark as skipsym
continue;
}
addLexeme(buffer, numbersym, atoi(buffer));
continue;
}
// special symbols
switch (input[i])
{
case '+': addLexeme("+", plussym, 0); i++; break;
case '-': addLexeme("-", minussym, 0); i++; break;
case '*': addLexeme("*", multsym, 0); i++; break;
case '/': addLexeme("/", slashsym, 0); i++; break;
case '=': addLexeme("=", eqlsym, 0); i++; break;
case '<':
if (input[i + 1] == '=') { addLexeme("<=", leqsym, 0); i += 2; }
else if (input[i + 1] == '>') { addLexeme("<>", neqsym, 0); i += 2;
}
else { addLexeme("<", lessym, 0); i++; }
break;
case '>':
if (input[i + 1] == '=') { addLexeme(">=", geqsym, 0); i += 2; }
else { addLexeme(">", gtrsym, 0); i++; }
break;
case ':':
if (input[i + 1] == '=') { addLexeme(":=", becomessym, 0); i +=
2; }
else { i++; } // Skip lone colon - handle gracefully
break;
case '(': addLexeme("(", lparentsym, 0); i++; break;
case ')': addLexeme(")", rparentsym, 0); i++; break;
case ',': addLexeme(",", commasym, 0); i++; break;
case ';': addLexeme(";", semicolonsym, 0); i++; break;
case '.': addLexeme(".", periodsym, 0); i++; break;
// Skip invalid symbols gracefully - don't generate error tokens
default:
addLexeme(&input[i], skipsym, 0); // Mark invalid symbol as skipsym
i++; // Move to the next character
break;
}
}
}
void printSource(const char *input)
{
printf("Source Program:\n\n%s\n", input);
}
void printLexemeTable()
{
printf("\nLexeme Table:\n");
printf("\n");
printf("lexeme\t token type\n");
// loop through and print table
for (int i=0; i<tableIndex; i++)
{
// error handling
if(table[i].token > 0)
{
printf("%-12s %d\n", table[i].lexeme, table[i].token);
}
else if(table[i].token == -1)
{
printf("%-12s %s\n", table[i].lexeme, "Indentifier too long");
}
else if(table[i].token == -2)
{
printf("%-12s %s\n", table[i].lexeme, "Number too long");
}
else if(table[i].token == -3)
{
printf("%-12s %s\n", table[i].lexeme, "Invalid Symbol");
}
}
printf("\n");
}
void printTokenList()
{
// printf("Token List:\n");
// printf("\n");
for (int i=0; i<tableIndex; i++)
{
// Only output valid tokens (positive token values)
// Do NOT output error tokens (negative values) as skipsym
if(table[i].token > 0)
{
fprintf(fptr, "%d ", table[i].token);
if (table[i].token == identsym || table[i].token == numbersym)
{
fprintf(fptr, "%s ", table[i].lexeme);
}
}
// Skip error tokens - don't output anything for them
}
fprintf(fptr, "\n");
}
int main(int argc, char *argv[])
{
fptr = fopen("tokens.txt","w");
// file input handling
if (argc != 2)
{
printf("Usage: %s <sourcefile>\n", argv[0]);
return 1;
}
FILE *fp = fopen(argv[1], "r");
if (!fp)
{
perror("File open error");
return 1;
}
size_t bytesRead = fread(source, 1, MAX_SOURCE_SIZE-1, fp);
source[bytesRead] = '\0'; // null-terminate the string
fclose(fp);
// main program flow
// printSource(source);
lexer(source);
// printLexemeTable();
printTokenList();
return 0;
}