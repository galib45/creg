#include "creg.h"

void print_usage(char *program_name) {
	printf("Usage:\n");
	printf("  %s PATTERN FILE.DOT\n", program_name);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	if (argc < 3) print_usage(argv[0]);

    char *pattern = argv[1];
	size_t index = 0;
	Node *root = parse_regex(pattern, &index);
	printf("Pattern: %s\n", pattern);

	node_print(root, 0);
	
	FILE *file = fopen(argv[2], "w");
	fprintf(file, "digraph {\n");
	fprintf(file, "label=\"Pattern = %s\"\n", escaped_string(pattern));
	fprintf(file, "fontname=\"Iosevka NF\"\n");
	fprintf(file, "fontsize=\"20pt\"\n");
	fprintf(file, "graph [pad=0.2]\n");
	node_print_dot(root, file);

	fprintf(file, "}\n");
	fclose(file);

	node_free(root);
    return 0;
}
