#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024
#define MAX_TOKENS 128
#define NUM_REGISTERS 4
typedef struct {
  char *name;
  char type;
  int opcode;
} Instruction;

const Instruction INSTRUCTIONS[] = {
    {"add", 'r', 0}, {"sub", 'r', 1}, {"mul", 'r', 2}, {"and", 'r', 3},
    {"or", 'r', 4},  {"xor", 'r', 5}, {"ldi", 'i', 6}, {"jmp", 'j', 7}};
const size_t INSTRUCTIONS_COUNT = sizeof(INSTRUCTIONS) / sizeof(*INSTRUCTIONS);

int print_instruction(int token_count, char *tokens[token_count]);

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <assembly-file>\n", argv[0]);
    return 1;
  }

  FILE *file = fopen(argv[1], "r");
  if (!file) {
    fprintf(stderr, "Couldn't open file\n");
    return 2;
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
        return 3;
      }
      token = strtok(NULL, delim);
    }
    print_instruction(token_count, tokens);
  }
}

int instruction_r(const Instruction *instruction, int token_count,
                  char *tokens[token_count]);
int instruction_i(const Instruction *instruction, int token_count,
                  char *tokens[token_count]);
int instruction_j(const Instruction *instruction, int token_count,
                  char *tokens[token_count]);

int print_instruction(int token_count, char *tokens[token_count]) {
  if (token_count < 1)
    return -1;
  for (int i = 0; i < INSTRUCTIONS_COUNT; i++) {
    if (!strcmp(tokens[0], INSTRUCTIONS[i].name)) {
      switch (INSTRUCTIONS[i].type) {
      case 'r':
        return instruction_r(&INSTRUCTIONS[i], token_count, tokens);
      case 'i':
        return instruction_i(&INSTRUCTIONS[i], token_count, tokens);
      case 'j':
        return instruction_j(&INSTRUCTIONS[i], token_count, tokens);
      default:
        return -2;
      }
    }
  }
  fprintf(stderr, "Invalid instruction: got %s\n", tokens[0]);
  return -3;
}

int read_integer_token(char *token, int range_min, int range_max) {
  char *end;
  long num = strtol(token, &end, 10);
  if (end == token) {
    fprintf(stderr, "Syntax error: integer is NaN.\n");
    exit(4);
    return -2;
  }

  if (num <= range_min || num >= range_max || errno == ERANGE) {
    fprintf(stderr, "Invalid argument: integer out of range (%d <= x <= %d).\n",
            range_min, range_max);
    exit(4);
    return -2;
  }

  return (int)num;
}

int read_register_token(char *token) {
  switch (token[0]) {
  case 'r':
  case 'R':
  case '$':
    break;
  default:
    fprintf(stderr, "Syntax error: register token not starting with r or $.\n");
    exit(4);
    return -1;
  }
  return read_integer_token(token + 1, -1, NUM_REGISTERS);
}

int instruction_r(const Instruction *instruction, int token_count,
                  char *tokens[token_count]) {
  if (token_count < 4) {
    fprintf(
        stderr,
        "Invalid arguments for R Instruction. Expected 4 arguments, got %d.\n",
        token_count);
    exit(4);
    return -2;
  }

  int opcode = instruction->opcode & 7;
  int des = read_register_token(tokens[1]) & 3;
  int src1 = read_register_token(tokens[2]) & 3;
  int src2 = read_register_token(tokens[3]) & 3;

  int result = opcode << 6 | des << 4 | src1 << 2 | src2;
  printf("%03x\n", result);
  return result;
}

int instruction_i(const Instruction *instruction, int token_count,
                  char *tokens[token_count]) {
  if (token_count < 3) {
    fprintf(
        stderr,
        "Invalid arguments for I Instruction. Expected 3 arguments, got %d.\n",
        token_count);
    exit(4);
    return -2;
  }

  int opcode = instruction->opcode & 7;
  int des = read_register_token(tokens[1]) & 3;
  int data = (read_integer_token(tokens[2], -8 - 1, 7 + 1));
  if (data < 0) data += 8;

  int result = opcode << 6 | des << 4 | data;
  printf("%03x\n", result);
  return result;
}

int instruction_j(const Instruction *instruction, int token_count,
                  char *tokens[token_count]) {
  if (token_count < 2) {
    fprintf(
        stderr,
        "Invalid arguments for J Instruction. Expected 2 arguments, got %d.\n",
        token_count);
    exit(4);
    return -2;
  }

  int opcode = instruction->opcode & 7;
  int prog_addr = read_integer_token(tokens[1], -1, 63 + 1) & 63;

  int result = opcode << 6 | prog_addr;
  printf("%03x\n", result);
  return result;
}
