#ifndef CREG_H
#define CREG_H
// declarations
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

typedef struct Regex Regex;
typedef struct Matches Matches;
typedef struct Operations Operations;
typedef struct Operation Operation;
typedef enum Opcode Opcode;
typedef union Operand Operand;

void errmsg(const char *format, ...);
void print_operations(Operations *ops);
Regex* compile_regex(char *pattern);

#endif // CREG_H
#ifdef CREG_H_IMPLEMENTATION

#ifndef CREG_DEBUG
#define CREG_DEBUG true
#endif

#define COLOR_RED_BOLD "\x1b[1;31m"
#define COLOR_GREEN_BOLD "\x1b[1;32m"
#define COLOR_GRAY "\x1b[38;5;240m"
#define COLOR_RESET "\x1b[0m"

#define COLOR_ERROR COLOR_RED_BOLD
#define COLOR_DEBUG COLOR_GRAY

#define NOB_ASSERT assert
#define NOB_REALLOC realloc
#define NOB_FREE free
#define NOB_DA_INIT_CAP 256
#define nob_da_append(da, item)                                                          \
    do {                                                                                 \
        if ((da)->count >= (da)->capacity) {                                             \
            (da)->capacity = (da)->capacity == 0 ? NOB_DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = NOB_REALLOC((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            NOB_ASSERT((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                                \
                                                                                         \
        (da)->items[(da)->count++] = (item);                                             \
    } while (0)

enum Opcode {
	OP_CHARACTER,
	OP_ANY_CHARACTER,
	OP_CHOICE,
	OP_ZERO_OR_MORE,
};

union Operand {
	int intval;
	char charval;
	char *strval;
};

struct Operation {
	Opcode opcode;
	Operand operand;
};

struct Operations {
	Operation *items;
	size_t count;
	size_t capacity;
};

struct Matches {
	char **items;
	size_t count;
	size_t capacity;
};

struct Regex {
	Operations ops;
	Matches matches;
};

void errmsg(const char *format, ...) {
    va_list args;
    
    // Initialize variadic arguments
    va_start(args, format);
    
    // Print error message to stderr
    fprintf(stderr, COLOR_ERROR "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n" COLOR_RESET);
    
    // Clean up variadic arguments
    va_end(args);
    
    // Exit with EXIT_FAILURE
    exit(EXIT_FAILURE);
}

void debug(const char *format, ...) {
    va_list args;
    
    // Initialize variadic arguments
    va_start(args, format);
    
    // Print error message to stderr
    fprintf(stdout, COLOR_DEBUG);
    vfprintf(stdout, format, args);
    fprintf(stdout, COLOR_RESET);
    
    // Clean up variadic arguments
    va_end(args);    
}


void push_char(char **str, char ch) {
    if (*str == NULL) {
        *str = malloc(sizeof(char));
        if (*str == NULL) errmsg("Failed to allocate memory.");
		(*str)[0] = '\0'; // Initialize as an empty string
    }
    
    size_t len = strlen(*str);
	*str = realloc(*str, sizeof(char)*(len+1));
    (*str)[len] = ch;
    (*str)[len + 1] = '\0'; // Null-terminate the string after adding the new character
}

void push_str(char **dest, const char *src) {
    if (src == NULL) {
        // Nothing to append
        return;
    }

    if (*dest == NULL) {
        *dest = malloc(sizeof(char) * (strlen(src) + 1));
        if (*dest == NULL) errmsg("Failed to reallocate memory.");
        strcpy(*dest, src);
    } else {
        size_t dest_len = strlen(*dest);
        size_t src_len = strlen(src);
        
        char *new_dest = realloc(*dest, sizeof(char) * (dest_len + src_len + 1));
        if (new_dest == NULL) errmsg("Failed to reallocate memory.");
        *dest = new_dest;
        strcpy(*dest + dest_len, src);
    }
}

void reset_str(char **str) {
	if (*str != NULL) {
		free(*str);
		*str = NULL;
	}
}

void print_operations(Operations *ops) {
	for (size_t i = 0; i < ops->count; i++) {
		Operation op = ops->items[i];
		debug("Operation(");
		switch(op.opcode) {
			case OP_CHARACTER: debug("OP_CHARACTER, %c", op.operand.charval); break;
			case OP_ANY_CHARACTER: debug("OP_ANY_CHARACTER"); break;
			case OP_CHOICE: debug("OP_CHOICE, %s", op.operand.strval); break;
			case OP_ZERO_OR_MORE: debug("OP_ZERO_OR_MORE"); break;
			default: errmsg("Unknown opcode.");
		}
		debug(")\n");
	}
}

Regex* compile_regex(char *pattern) {
	size_t counter = 0;
	Regex *regex = malloc(sizeof(Regex));
	size_t length = strlen(pattern);
	if (length == 0) errmsg("Empty pattern.");
	if (CREG_DEBUG) debug("Pattern: \"%s\"\n", pattern);
	char ch; Operation op;
	while(counter < length) {
		ch = pattern[counter++];
		switch(ch) {
			case '.': 
				op.opcode = OP_ANY_CHARACTER; 
				nob_da_append(&regex->ops, op);
				break;
			case '[': 
				op.opcode = OP_CHOICE; op.operand.strval = NULL;
				size_t choice_start = counter-1; bool choice_ended = false;
				while(ch != ']' && counter < length) {
					ch = pattern[counter++];
					switch(ch) {
						case ']': 
							choice_ended = true; 
							nob_da_append(&regex->ops, op);
							break;
						case '-':
							if (choice_start > counter-3) errmsg("Unexpected '-'.");
							char prev = pattern[counter-2];
							ch = pattern[counter++];
							if (ch <= prev) errmsg("Invalid range.");
							for (char x = prev + 1; x <= ch; x++) {
								push_char(&op.operand.strval, x);
							}
							break;
						default:
							push_char(&op.operand.strval, ch);
							break;
					}
				}
				if (!choice_ended) errmsg("Expected ']', found %c", ch);
				continue;
			case '*':
				op.opcode = OP_ZERO_OR_MORE;
				nob_da_append(&regex->ops, op);
				break;
			default: 
				op.opcode = OP_CHARACTER; 
				op.operand.charval = ch;
				nob_da_append(&regex->ops, op);
				break;
		}
		//counter++;
	}
	if (CREG_DEBUG) print_operations(&regex->ops);
	return regex;
}

#define reset_match 								\
	if (success) { 									\
		if (opcounter < oplength - 1 				\
		&& regex->ops.items[opcounter+1].opcode == OP_ZERO_OR_MORE) {	\
			nob_da_append(&regex->matches, strdup(match));				\
		}											\
		strcounter--;  								\
	} 												\
	success = false; 								\
	reset_str(&match); opcounter = 0;				\

void match(Regex *regex, char *string) {
	size_t strcounter = 0, opcounter = 0;
	size_t strlength = strlen(string);
	if (strlength == 0) errmsg("Empty string.");
	if (CREG_DEBUG) debug("String: \"%s\"\n", string);
	size_t oplength = regex->ops.count;
	char ch; Operation op;
	char *match = NULL;
	bool success = false, finished = false; 
	size_t repeat = 0;
	while (strcounter < strlength && opcounter < oplength) {
		ch = string[strcounter++];
		if (CREG_DEBUG) debug("string[%zu] = %c, op=%zu, ", strcounter-1, ch, opcounter);
		op = regex->ops.items[opcounter];
		switch(op.opcode) {
			case OP_CHARACTER: 
				if (!(success || opcounter == 0)) break;
				else if (ch == op.operand.charval) {
					push_char(&match, ch);
					opcounter++; success = true;
				} else { reset_match } 
				break;
			case OP_ANY_CHARACTER:
				if (!(success || opcounter == 0)) break;
				if (ch != '\n') {
					if (!success) free(match);
					push_char(&match, ch);
					opcounter++; success = true;
				} else { reset_match } 
				break;
			case OP_CHOICE:
				if (!(success || opcounter == 0)) break;
				if (strchr(op.operand.strval, ch)) {
					push_char(&match, ch);
					opcounter++; success = true;
				} else { reset_match }
				break;
			case OP_ZERO_OR_MORE:
				if (!(success || opcounter == 0)) break;
				if (success) {
					repeat++; opcounter--; strcounter--; 
				} else { reset_match }
				break;
			default: errmsg("Unknown opcode.");
		}
		if (CREG_DEBUG) debug("success=%s, repeat=%zu, match=%s\n", success?"true":"false", repeat, match);
		if (success && opcounter == oplength) finished = true;
		// else if (opcounter == oplength - 1) {
		// 	if (regex->ops.items[opcounter].opcode == OP_ZERO_OR_MORE) finished = true;
		// }
		if (finished) {
			opcounter = 0; success = false; finished = false; 
			nob_da_append(&regex->matches, strdup(match)); 
			reset_str(&match); 
		}
	}
	if (success) nob_da_append(&regex->matches, strdup(match));
}

#endif // CREG_H_IMPLEMENTATION
