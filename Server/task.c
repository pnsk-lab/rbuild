/* $Id$ */

#include "rbs_task.h"

#include "rbs_server.h"
#include "rbs_config.h"

#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __MINGW32__
#else
#include <sys/wait.h>
#endif

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

char** rbs_parse_args(const char* cmd, const char* arg) {
	char* str = cm_strdup(arg);
	char** stack = malloc(sizeof(*stack));
	int i;
	int incr = 0;
	CMBOOL dq = CMFALSE;
	stack[0] = NULL;
	rbs_push(&stack, cmd);
	for(i = 0;; i++) {
		if(str[i] == 0 || (str[i] == ' ' && !dq)) {
			char oldc = str[i];
			char* got = str + incr;
			str[i] = 0;

			if(strlen(got) > 2 && got[0] == '-' && got[1] == 'o') {
				rbs_push(&stack, "-o");
				rbs_push(&stack, got + 2);
			} else if(strlen(got) > 2 && got[0] == '-' && got[1] == 'L') {
				rbs_push(&stack, "-L");
				rbs_push(&stack, got + 2);
			} else if(strlen(got) > 2 && got[0] == '-' && got[1] == 'I') {
				rbs_push(&stack, "-I");
				rbs_push(&stack, got + 2);
			} else {
				rbs_push(&stack, got);
			}

			incr = i + 1;
			if(oldc == 0) break;
		} else if(str[i] == '"') {
			int oldi = i;
			dq = !dq;
			for(; str[i] != 0; i++) {
				str[i] = str[i + 1];
			}
			i = oldi - 1;
		} else if(str[i] == '\\') {
			int oldi = i;
			for(; str[i] != 0; i++) {
				str[i] = str[i + 1];
			}
			i = oldi;
		}
	}
	free(str);
	return stack;
}

#ifdef __MINGW32__
#else
int outpipe[2];
pid_t pid;

CMBOOL rbs_start_process(const char* cmd, char** args) {
	pipe(outpipe);
	pid = fork();
	if(pid == 0) {
		dup2(outpipe[1], 1);
		dup2(outpipe[1], 2);
		close(outpipe[0]);
		execvp(cmd, args);
		_exit(-1);
	} else if(pid == -1) {
		return CMFALSE;
	}
	close(outpipe[1]);
	return CMTRUE;
}

CMBOOL rbs_wait_process(int sock) {
	int status;
	unsigned char c = 0;
	rbs_write(sock, "+", 1);
	while(1) {
		char oldc = c;
		int len = read(outpipe[0], &c, 1);
		if(len <= 0) break;
		if(c == '\n') {
			if(oldc == '\n') rbs_write(sock, "+", 1);
			rbs_write(sock, "\n", 1);
		} else {
			if(oldc == '\n') rbs_write(sock, "+", 1);
			rbs_write(sock, &c, 1);
		}
	}
	if(c != '\n') {
		rbs_write(sock, "\n", 1);
	}
	waitpid(pid, &status, 0);
	close(outpipe[0]);
	return WEXITSTATUS(status) == 0;
}
#endif

CMBOOL rbs_task(int sock, const char* section, const char* cmd, const char* arg) {
	char** args = rbs_parse_args("gcc", arg);
	CMBOOL status = rbs_start_process("gcc", args);
	rbs_stack_free(args);
	if(!status) return CMFALSE;
	rbs_write(sock, "SUCCESS\n", 8);
	return rbs_wait_process(sock);
}
