#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024
#define MAX_TOKENS 128

// General Config
#define NUM_REGISTERS 4
#define BITS_INSTRUCTION 9
#define BITS_OPCODE 3
// R- & I-Instruction Config
#define BITS_DES 2
// R-Instruction Config
#define BITS_SRC1 2
#define BITS_SRC2 2
// I-Instruction Config
#define BITS_DATA 4
// J-Instruction Config
#define BITS_PROG_ADDR 6

// Error Codes
#define ERROR_USAGE 1
#define ERROR_FILE 2
#define ERROR_TOKENS 3
#define ERROR_INSTRUCTION 4

typedef struct {
  char *name;
  char type;
  int opcode;
} Instruction;

const Instruction INSTRUCTIONS[] = {
    {"add", 'r', 0x0}, {"sub", 'r', 0x1}, {"mul", 'r', 0x2}, {"and", 'r', 0x3},
    {"or", 'r', 0x4},  {"xor", 'r', 0x5}, {"ldi", 'i', 0x6}, {"jmp", 'j', 0x7}};

const size_t INSTRUCTIONS_COUNT = sizeof(INSTRUCTIONS) / sizeof(*INSTRUCTIONS);

int read_instruction(int token_count, char *tokens[token_count]);

char *instruction_to_string(int instruction, int destination_length,
                            char *destination) {
  // ⌈BITS_INSTRUCTION / 4⌉
  const int print_hex_digits = (BITS_INSTRUCTION + 3) / 4;
  snprintf(destination, destination_length, "%0*x", print_hex_digits,
           instruction);
  return destination;
}
int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <assembly-file> [<output-file>]\n", argv[0]);
    return ERROR_USAGE;
  }
  FILE *output = argc < 3 ? stdout : fopen(argv[2], "w");
  if (!output) {
    fprintf(stderr, "Couldn't open output file");
    return ERROR_FILE;
  }

  FILE *file = fopen(argv[1], "r");
  if (!file) {
    fprintf(stderr, "Couldn't open assembly file\n");
    return ERROR_FILE;
  }

  char line[MAX_LINE];
  char *token;
  const char *delim = " \t,\r\n";

  char *tokens[MAX_TOKENS];
  int token_count = 0;

  while (fgets(line, sizeof(line), file)) {
    token = strtok(line, delim);
    token_count = 0;
    while (token) {
      tokens[token_count++] = token;
      if (token_count >= MAX_TOKENS) {
        fprintf(stderr, "Too many tokens\n");
        return ERROR_TOKENS;
      }
      token = strtok(NULL, delim);
    }
    int instruction = read_instruction(token_count, tokens);
    if (instruction == -1) continue; // Comment
    char str[4];
    fprintf(output, "0x%s\n", instruction_to_string(instruction, 4, str));
  }
}

char *string_lower(char *string) {
  for (int i = 0; string[i]; i++) {
    string[i] = tolower(string[i]);
  }
  return string;
}

int instruction_r(const Instruction *instruction, int token_count,
                  char *tokens[token_count]);
int instruction_i(const Instruction *instruction, int token_count,
                  char *tokens[token_count]);
int instruction_j(const Instruction *instruction, int token_count,
                  char *tokens[token_count]);

int read_instruction(int token_count, char *tokens[token_count]) {
  if (token_count < 1)
    return -1;
  for (int i = 0; i < INSTRUCTIONS_COUNT; i++) {
    if (!strcmp(string_lower(tokens[0]), INSTRUCTIONS[i].name)) {
      switch (INSTRUCTIONS[i].type) {
      case 'r':
        return instruction_r(&INSTRUCTIONS[i], token_count, tokens);
      case 'i':
        return instruction_i(&INSTRUCTIONS[i], token_count, tokens);
      case 'j':
        return instruction_j(&INSTRUCTIONS[i], token_count, tokens);
      default:
        fprintf(stderr, "Instruction error: instruction type not valid: got %c",
                INSTRUCTIONS[i].type);
        exit(ERROR_INSTRUCTION);
      }
    }
  }
  if (tokens[0][0] == '#') {
      return -1;
  }
  fprintf(stderr, "Invalid instruction: got %s\n", tokens[0]);
  exit(ERROR_INSTRUCTION);
}

int read_integer_token(char *token, int range_min, int range_max) {
  char *end;
  long num = strtol(token, &end, 10);
  if (end == token) {
    fprintf(stderr, "Syntax error: integer is NaN.\n");
    exit(ERROR_INSTRUCTION);
  }

  if (num <= range_min || num >= range_max || errno == ERANGE) {
    fprintf(stderr, "Invalid argument: integer out of range (%d <= x <= %d).\n",
            range_min, range_max);
    exit(ERROR_INSTRUCTION);
  }

  return (int)num;
}

int read_register_token(char *token) {
  switch (tolower(token[0])) {
  case 'r':
  case '$':
    return read_integer_token(token + 1, -1, NUM_REGISTERS);
  default:
    fprintf(stderr, "Syntax error: register token not starting with r or $.\n");
    exit(ERROR_INSTRUCTION);
  }
}

void check_token_count(int token_count, int min_count, char instruction_type) {
  if (token_count < min_count) {
    fprintf(stderr,
            "Invalid arguments for %c Instruction. Expected %d arguments, got "
            "%d.\n",
            instruction_type, min_count, token_count);
    exit(ERROR_INSTRUCTION);
  }
}

int instruction_r(const Instruction *instruction, int token_count,
                  char *tokens[token_count]) {
  check_token_count(token_count, 4, 'R');

  const int mask_opcode = (1 << BITS_OPCODE) - 1;
  const int mask_des = (1 << BITS_DES) - 1;
  const int mask_src1 = (1 << BITS_SRC1) - 1;
  const int mask_src2 = (1 << BITS_SRC2) - 1;
  const int offset_src1 = BITS_SRC2;
  const int offset_des = offset_src1 + BITS_SRC1;
  const int offset_opcode = offset_des + BITS_DES;

  int opcode = instruction->opcode & mask_opcode;
  int des = read_register_token(tokens[1]) & mask_des;
  int src1 = read_register_token(tokens[2]) & mask_src1;
  int src2 = read_register_token(tokens[3]) & mask_src2;

  return opcode << offset_opcode | des << offset_des | src1 << offset_src1 |
         src2;
}

int instruction_i(const Instruction *instruction, int token_count,
                  char *tokens[token_count]) {
  check_token_count(token_count, 3, 'R');

  const int mask_opcode = (1 << BITS_OPCODE) - 1;
  const int mask_des = (1 << BITS_DES) - 1;
  const int mask_data = (1 << BITS_DATA) - 1;
  const int offset_des = BITS_DATA;
  const int offset_opcode = offset_des + BITS_DES;

  const int min_data = -(1 << (BITS_DATA - 1));
  // Could also be an unsigned number
  const int max_data = (1 << (BITS_DATA - 1)) - 1;

  int opcode = instruction->opcode & mask_opcode;
  int des = read_register_token(tokens[1]) & mask_des;

  int data = (read_integer_token(tokens[2], min_data - 1, max_data + 1));
  if (data < 0)
    data = data & mask_data | (1 << (BITS_DATA - 1));
  else
    data = data & mask_data;

  return opcode << offset_opcode | des << offset_des | data;
}

int instruction_j(const Instruction *instruction, int token_count,
                  char *tokens[token_count]) {
  check_token_count(token_count, 2, 'R');

  const int mask_opcode = (1 << BITS_OPCODE) - 1;
  const int mask_des = (1 << BITS_DES) - 1;
  const int mask_prog_addr = (1 << BITS_PROG_ADDR) - 1;
  const int offset_opcode = BITS_PROG_ADDR;

  const int min_data = 0;
  const int max_data = (1 << BITS_PROG_ADDR) - 1;

  int opcode = instruction->opcode & mask_opcode;
  int prog_addr = read_integer_token(tokens[1], min_data - 1, max_data + 1) &
                  mask_prog_addr;

  return opcode << offset_opcode | prog_addr;
}
