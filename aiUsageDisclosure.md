# AI Usage Disclosure Details

**Student Name:** Collin Van Meter
**Student ID:** 5580217
**Assignment:** homework 4

---

## Instructions

Complete this template with detailed information about your AI usage. Submit this file along with your signed PDF declaration form.

---

## AI Tool #1

### Tool Name
Github Copilot

### Version/Model
copilot using Claude Sonnet 4.5

### Date(s) Used
11/19/2025

### Specific Parts of Assignment
Output formatting for assembly code and symbol table in parsercodegen_complete.c
Verification of EVEN instruction implementation in vm.c
Testing and debugging of the complete compilation pipeline
Formatting adjustments to match hw4 (2).txt specification requirements

### Prompts Used
Please reference hw4.txt to validate my implementation with the packet specifications. I am also encountering a segfault error when printing the contents of the sym table/assembly code.

### AI Output/Results
AI analyzed the hw4 requirements document and identified output formatting issues
AI suggested a modified print_assembly_code() function to match specified format with tabs and headers
AI suggested updating print_symbol_table() to display Kind values (1, 2, 3) instead of string labels 
AI verified that vm.c correctly implements the EVEN instruction 
AI created test cases and verified: procedure calls, if-then-else statements, EVEN instruction, and error handling

### How Output was Verified/Edited
Tested complete pipeline with multiple test cases, procedure declaration and call test, If-then-else statement test with both branches,
EVEN instruction test with even/odd number
Verified VM execution output matched expected behavior
Confirmed output format matches hw4.txt Appendix B specification

### Learning & Reflection
ai tools are very helpful with digesting and verifying large assignment specifications, and is a big help debugging errors caused by glazed over eyes after looking at code for a while :)
