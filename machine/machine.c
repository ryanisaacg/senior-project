#include "machine.h"

#include "io.h"
#include "instructions.h"
#include <stdbool.h>
#include <string.h>

void initialize_hardware();
void load_disk();
void write_disk();
void execute_bytecode(ubyte *data, size_t bytecode_length);
void execute_statement(ubyte *data, size_t position, size_t *new_position, bool *keep_going);
number get_value(ubyte *instruction);
void set_value(ubyte *instruction, number value);
number get_number(ubyte *bytes);

int main() {
	initialize_hardware();
	load_disk();
	execute_bytecode(bios, bios_size);
	write_disk();
	fclose(disk);
}

void initialize_hardware() {
	disk = fopen("harddisk", "r+");
	ram = malloc(RAM_SIZE);
	if(disk == NULL) {
		fprintf(stderr, "Failed to initialize hard disk.");
		exit(-1);
	}
	if(ram == NULL) {
		fprintf(stderr, "Failed to initialize RAM.");
		exit(-1);
	}
	disk_buffer = NULL;
	disk_size = 0;
	FILE* bios_file = fopen("bios", "r");
	bios = read_file(bios_file, &bios_size);
	fclose(bios_file);
}

void load_disk() {
	if(disk_buffer != NULL) {
		free(disk_buffer);
	}
	disk_buffer = malloc(1024);
	if(disk_buffer == NULL) {
		fprintf(stderr, "Memory allocation failed in load_disk\n");
	}
	disk_buffer = read_file(disk, &disk_size);
	fseek(disk, 0, SEEK_SET);
}

void write_disk() {
	fwrite(disk_buffer, 1, disk_size, disk);
}

number get_number(ubyte *bytes) {
	number value = 0;
	value += bytes[3];
	value += bytes[2] * 16;
	value += bytes[1] * 256;
	value += bytes[0] % 8;
	if(bytes[0] != 0 && bytes[0] % 8 == 0) {
		value *= -1;
	}
	return value;
}

number get_value(ubyte *instruction) {
	switch(instruction[0]) {
		case REGISTER:
			return registers[get_number(instruction + 1)];
		case REGISTER_VALUE:
		case POINTER:
			return get_number(ram + get_number(instruction + 1));
		case CONSTANT:
			return get_number(instruction + 1);
		default:
			fprintf(stderr, "Failed switched statement in get_value");
			exit(-1);
	}
}

void set_value(ubyte *instruction, int value) {
	switch(instruction[0]) {
		case REGISTER:
			registers[get_number(instruction + 1)] = value;
			break;
		case REGISTER_VALUE:
			memcpy(ram + registers[get_number(instruction + 1)], &value, 4);
			break;
		case POINTER:
			memcpy(ram + get_number(instruction + 1), &value, 4);
			break;
		case CONSTANT:
			fprintf(stderr, "Cannot set the value of a constant.\n");
			exit(-1);
			break;
	}
}

void execute_bytecode(ubyte *data, size_t bytecode_length) {
	size_t i = 0;
	bool keep_going = true;
	while(i < bytecode_length && keep_going) {
		execute_statement(data, i, &i, &keep_going);
	}
}

size_t command_length(ubyte command) {
	switch(command) {
		case MOV:
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case AND:
		case IOR:
		case XOR:
		case CMP:
		case EXE:
			return 10;
		case BRN:
		case RFI:
		case WTO:
		case RHD:
		case WHD:
			return 5;
	}
}

bool fulfills_condition(ubyte condition) {
	switch(condition) {
		case EQ:
			return register_compare == 0;
		case NE:
			return register_compare != 0;
		case GR:
			return register_compare > 0;
		case NG:
			return register_compare <= 0;
		case LS:
			return register_compare < 0;
		case NL:
			return register_compare >= 0;
	}
}

#define ASM_OPERATION(instr, op) case instr: { number a = get_value(arguments); number b = get_value(arguments + 5); \
	set_value(arguments + 10, a op b); *new_position = position + 11; } break;

void execute_statement(ubyte *data, size_t position, size_t *new_position, bool *keep_going) {
	data += position;
	ubyte command = data[0];
	ubyte condition = data[1];
	*new_position = position + 2 + command_length(command);
	*keep_going = true;
	if(!fulfills_condition(condition))
		return;
	ubyte *arguments = data + 2;
	switch(command) {
		case MOV: {
			number source = get_value(arguments);
			set_value(arguments + 5, source);
		} break;
		ASM_OPERATION(ADD, +);
		ASM_OPERATION(SUB, -);
		ASM_OPERATION(MUL, *);
		ASM_OPERATION(DIV, /);
		ASM_OPERATION(MOD, %);
		case RFI: {
			number input = getc(stdout);
			set_value(arguments, input);
		} break;
		case WTO: {
			number source = get_value(arguments);
			putc(source, stdout);
		} break;
		ASM_OPERATION(AND, &);
		ASM_OPERATION(IOR, |);
		ASM_OPERATION(XOR, ^);
		case CMP: {
			number a = get_value(arguments);
			number b = get_value(arguments + 5);
			register_compare = a - b;
		} break;
		case BRN: {
			*new_position = get_value(arguments);
		} break;
		case RHD: {
			number disk_spot = get_value(arguments);
			set_value(arguments + 5, get_number(disk_buffer + disk_spot));
		} break;
		case WHD: {
			number value = get_value(arguments);
			number disk_spot = get_value(arguments + 5);
			memcpy(disk_buffer + disk_spot, &value, 4);
		} break;
		case EXE: {
			number pointer = get_value(arguments);
			number length = get_value(arguments + 5);
			execute_bytecode(ram + pointer, length);
		} break;
	}
}
