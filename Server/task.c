/* $Id$ */

#include "rbs_task.h"

#include "rbs_server.h"
#include "rbs_config.h"

#include <stddef.h>
#include <stdlib.h>

#include <cm_string.h>
#include <cm_bool.h>

void rbs_push(char*** stack, const char* str) {
	char** oldstack = *stack;
	int i;
	for(i = 0; oldstack[i] != NULL; i++)
		;
	*stack = malloc(sizeof(**stack) * (i + 2));
	for(i = 0; oldstack[i] != NULL; i++) {
		(*stack)[i] = oldstack[i];
	}
	free(oldstack);
	(*stack)[i] = cm_strdup(str);
	(*stack)[i + 1] = NULL;
}

void rbs_stack_free(char** stack) {
	int i;
	for(i = 0; stack[i] != NULL; i++) free(stack[i]);
	free(stack);
}

char** rbs_parse_args(const char* arg) {
	char* str = cm_strdup(arg);
	char** stack = malloc(sizeof(*stack));
	int i;
	int incr = 0;
	stack[0] = NULL;
	for(i = 0;; i++) {
		if(str[i] == 0 || str[i] == ' ') {
			char oldc = str[i];
			char* got = str + incr;
			str[i] = 0;

			rbs_push(&stack, got);

			incr = i + 1;
			if(oldc == 0) break;
		}
	}
	free(str);
	for(i = 0; stack[i] != NULL; i++) {
		printf("[%s]\n", stack[i]);
	}
	return stack;
}

CMBOOL rbs_task(int sock, const char* section, const char* cmd, const char* arg) {
	char** args = rbs_parse_args(arg);
	rbs_stack_free(args);
	return CMFALSE;
}
