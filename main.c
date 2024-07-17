#include <stdio.h>

#define CREG_H_IMPLEMENTATION
// #define CREG_DEBUG false
#include "creg.h"

int main() {
	char *pattern = "[_a-zA-Z][a-zA-Z0-9]*";
	char *string = "**_index2";
	Regex *regex = compile_regex(pattern);
	match(regex, string);
	printf("%zu matches found\n", regex->matches.count);
	for (size_t i = 0; i < regex->matches.count; i++) {
		printf("Match %zu: \"%s\"\n", i+1, regex->matches.items[i]);
	}
	return EXIT_SUCCESS;
}
