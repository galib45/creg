#ifndef CREG_H
#define CREG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

void errmsg(const char *format, ...) {
    va_list args;
    va_start(args, format);
	fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

char* escaped_string(char *input) {
    if (!input) return NULL;
	char *output = malloc(strlen(input) * 2 + 1);
	char *start = output;
	while (*input) {
        switch (*input) {
            case '\\': *output++ = '\\'; *output++ = '\\'; break;
            case '\"': *output++ = '\\'; *output++ = '\"'; break;
            case '\n': *output++ = '\\'; *output++ = 'n'; break;
            case '\t': *output++ = '\\'; *output++ = 't'; break;
            default: *output++ = *input; break;
        }
        input++;
    }
    *output = '\0';
	return start;
}

// Define a generic dynamic array structure
typedef struct {
    void *items;
    size_t count;
    size_t capacity;
} dynamic_array;

// Generic function to append an item
static inline void da_append(void *array, size_t item_size, const void *item) {
    dynamic_array *da = (dynamic_array *)array;

    // Check if resizing is needed
    if (da->count >= da->capacity) {
        da->capacity = da->capacity == 0 ? 4 : da->capacity * 2; 
        void *new_items = realloc(da->items, da->capacity * item_size);
        if (!new_items) errmsg("Failed to reallocate memory.");
        da->items = new_items;
    }
    // Append the item
    memcpy((char *)da->items + da->count * item_size, item, item_size);
    da->count++;
}

typedef struct _Node Node;
typedef struct _Nodelist Nodelist;
typedef enum _Nodetype Nodetype;

enum _Nodetype {
	NODE_CONCAT, NODE_ALTERN, NODE_STAR,
	NODE_CHARACTER, NODE_ANY_CHARACTER,
};

struct _Nodelist {
	Node *items;
	size_t count;
	size_t capacity;
};

struct _Node {
	Nodetype node_type;
	union {
		int ivalue;
		char cvalue;
		char *svalue;
	};
	Node *parent;
	Nodelist *children;
};

Node* node_create_new(Nodetype node_type) {
	Node *node = malloc(sizeof(Node));
	if (!node) errmsg("Failed to allocate memory.");
	node->node_type = node_type;
	node->ivalue = 0;
	node->cvalue = '\0';
	node->svalue = NULL;
	node->parent = NULL;
	node->children = NULL;
	return node;
}

void node_add_child(Node *node, Node *child) {
	if (!child) return;
	if (!node->children) {
		node->children = malloc(sizeof(Nodelist));
		if (!node->children) errmsg("Failed to allocate memory.");
		node->children->items = NULL;
		node->children->count = 0;
		node->children->capacity = 0;
	}
	child->parent = node;
	da_append(node->children, sizeof(Node), child);
}

Node* node_last_child(Node *node) {
	if (node->children && node->children->items && node->children->count > 0) {
		return &((Node *)node->children->items)[node->children->count - 1];
	}
	return NULL;
}

Node* node_pop(Node *node) {
	if (node->children && node->children->items && node->children->count > 0) {
		Node *last_child = &((Node *)node->children->items)[node->children->count - 1];
		node->children->count--;
		return last_child;
	}
	return NULL;
}

void node_free(Node *node) {
    if (node == NULL) return;
    if (node->svalue != NULL) free(node->svalue);
    
    if (node->children != NULL) {
        if (node->children->items != NULL) {
			for (size_t i = 0; i < node->children->count; ++i) {
				Node *child = &((Node *)node->children->items)[i];
				node_free(child);
			}
			free(node->children->items);
		}
        free(node->children);
    }
	
	free(node);
}

// Function to print a node
void node_print(const Node *node, int depth) {
    if (node == NULL) {
        fprintf(stderr, "Error: Null node pointer.\n");
        return;
    }

    // Print indentation for readability
    for (int i = 0; i < depth; ++i) {
        fprintf(stdout, "  ");
    }

    // Print node type
    switch (node->node_type) {
        case NODE_CONCAT: fprintf(stdout, "CONCAT\n"); break;
        case NODE_ALTERN: fprintf(stdout, "ALTERN\n"); break;
        case NODE_STAR: fprintf(stdout, "STAR\n"); break;
        case NODE_ANY_CHARACTER: fprintf(stdout, "ANY_CHARACTER\n"); break;
        case NODE_CHARACTER: fprintf(stdout, "CHARACTER, '%c'\n", node->cvalue); break;
        default: fprintf(stdout, "Node Type: UNKNOWN\n"); break;
    }
    // Print children nodes, if any
    if (node->children != NULL && node->children->items != NULL) {
        for (size_t i = 0; i < node->children->count; ++i) {
            Node *child = &((Node *)node->children->items)[i];
            node_print(child, depth + 1);
        }
    }
}

// Function to print DOT format for a node
void node_print_dot(const Node *node, FILE *file) {
    if (node == NULL) return;
    if (file == NULL) return;

    // Print current node
    fprintf(file, "  node%p [fontname=\"Iosevka NF\" label=\"", (void*)node);
    switch (node->node_type) {
        case NODE_CONCAT: fprintf(file, "CONCAT"); break;
        case NODE_ALTERN: fprintf(file, "ALTERN"); break;
        case NODE_STAR: fprintf(file, "STAR"); break;
        case NODE_ANY_CHARACTER: fprintf(file, "ANY_CHAR"); break;
        case NODE_CHARACTER: 
			char *str = malloc(2); str[0] = node->cvalue; str[1] = '\0';
			fprintf(file, "CHAR, '%s'", escaped_string(str)); break;
        default: fprintf(file, "UNKNOWN"); break;
    }
    fprintf(file, "\"];\n");

	// Print children nodes, if any
    if (node->children != NULL && node->children->items != NULL) {
		for (size_t i = 0; i < node->children->count; ++i) {
            Node *child = &((Node *)node->children->items)[i];
			fprintf(file, "  node%p -> node%p;\n", (void*)node, (void*)child);
			node_print_dot(child, file);
		}
	}
	
}

Node* parse_regex(char *pattern, size_t *index) {
	if (*pattern == '\0') return NULL;
	Node *root = node_create_new(NODE_CONCAT);
	Node *current_root = root;
	Node *temp = NULL; 
	char ch; size_t length = strlen(pattern);
	bool is_escaping = false;
	while(*index < length) {
		ch = pattern[*index];
		switch(ch) {
			case '|':
				if (!is_escaping) {
					temp = node_create_new(NODE_ALTERN);
					root = temp;
					node_add_child(temp, current_root);
					(*index)++;
					node_add_child(temp, parse_regex(pattern, index));
					current_root = node_last_child(temp);
					break;
				}
				[[fallthrough]];
			case '(':
				if (!is_escaping) {
					int depth = 1; (*index)++;
					char *subpattern = malloc(strlen(pattern));
					char *start = subpattern;
					while(ch) {
						ch = pattern[*index];
						if (ch == ')') { 
							depth--; if (depth == 0) break;
						}
						else if (ch == '(') depth++;
						*subpattern++ = ch;
						(*index)++;
					}
					if (depth != 0) errmsg("Parenthesis not closed.");
					*subpattern = '\0';
					subpattern = start;
					size_t subindex = 0;
					node_add_child(current_root, parse_regex(subpattern, &subindex));
					break;
				}
				[[fallthrough]];
			case ')':
				if (!is_escaping) errmsg("Unexpected closing parenthesis.");	
				[[fallthrough]];
			case '.':
				if (!is_escaping) {
					temp = node_create_new(NODE_ANY_CHARACTER);
					node_add_child(current_root, temp);
					break;
				}	
				[[fallthrough]];
			case '*':
				if (!is_escaping) {
					temp = node_last_child(current_root);
					node_pop(current_root);
					Node *starnode = node_create_new(NODE_STAR);
					node_add_child(starnode, temp);
					node_add_child(current_root, starnode);
					break;
				} 
				[[fallthrough]];
			case '\\':
				if (is_escaping) {
					temp = node_create_new(NODE_CHARACTER);
					temp->cvalue = ch;
					node_add_child(current_root, temp);
					is_escaping = false;
				} else is_escaping = true;
				break;
			default: 
				if (is_escaping) errmsg("Undefined escape sequence.");
				temp = node_create_new(NODE_CHARACTER);
				temp->cvalue = ch;
				node_add_child(current_root, temp);
				break;
		}
		(*index)++;
	}
	return root;
}

#endif // CREG_H
