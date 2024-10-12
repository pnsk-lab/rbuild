/* $Id$ */

#include "rbs_task.h"

#include "rbs_server.h"
#include "rbs_config.h"

#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

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

CMBOOL rbs_start_process(const char* cmd, char** args, const char* tmpdir) {
	pipe(outpipe);
	pid = fork();
	if(pid == 0) {
		chdir(tmpdir);
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

char* rbs_get_program(const char* section, const char* cmd) {
	char* type = rbs_config_get(section, "type");
	if(strcmp(type, "gnu") == 0) {
		char* prefix = rbs_config_get(section, "prefix");
		if(strcmp(cmd, "CC") == 0) {
			return cm_strcat(prefix, "gcc");
		} else if(strcmp(cmd, "LD") == 0) {
			return cm_strcat(prefix, "ld");
		} else if(strcmp(cmd, "AS") == 0) {
			return cm_strcat(prefix, "as");
		} else if(strcmp(cmd, "CXX") == 0) {
			return cm_strcat(prefix, "g++");
		} else if(strcmp(cmd, "AR") == 0) {
			return cm_strcat(prefix, "ar");
		}
	}
	return cm_strdup("");
}

char** rbs_translate_args(const char* section, const char* cmd, char** args, int sock, char* tmpdir, char** output) {
	int i;
	char* type = rbs_config_get(section, "type");
	char** r = malloc(sizeof(*r));
	r[0] = NULL;
	rbs_push(&r, args[0]);
	for(i = 1; args[i] != NULL; i++) {
		if(strcmp(type, "gnu") == 0 && (strcmp(cmd, "CC") == 0 || strcmp(cmd, "CXX") == 0)) {
			if(strcmp(args[i], "-o") == 0 || strcmp(args[i], "-I") == 0 || strcmp(args[i], "-L") == 0) {
				if(strcmp(args[i], "-o") == 0) {
					*output = cm_strdup(args[i + 1]);
				}
				rbs_push(&r, args[i]);
				rbs_push(&r, args[i + 1]);
				i++;
			} else if(strcmp(args[i], "-c") == 0 || strcmp(args[i], "-fcommon") == 0 || strcmp(args[i], "-fno-common") == 0 || strcmp(args[i], "-fPIC") == 0 || strcmp(args[i], "-shared") == 0) {
				rbs_push(&r, args[i]);
			} else if(args[i][0] != '-') {
				/* file input */
				char* l;
				char* path;
				unsigned long bytes;
				unsigned char buffer[512];
				FILE* fout;
				rbs_write(sock, "GIMME ", 6);
				rbs_write(sock, args[i], strlen(args[i]));
				rbs_write(sock, "\n", 1);
				l = rbs_readline(sock);
				if(l == NULL) {
					rbs_stack_free(r);
					return NULL;
				}
				bytes = atol(l);
				if(bytes == 0) {
					rbs_stack_free(r);
					return NULL;
				}
				path = cm_strcat3(tmpdir, "/", args[i]);
				fout = fopen(path, "wb");
				while(bytes != 0) {
					int rb = rbs_read(sock, buffer, bytes < 512 ? bytes : 512);
					fwrite(buffer, 1, rb, fout);
					bytes -= rb;
				}
				fclose(fout);
				free(path);
				rbs_push(&r, args[i]);
			}
		}
	}
	return r;
}

char* rbs_create_tmpdir(void) {
#ifdef __MINGW32__
#else
	char* path = malloc(512);
	sprintf(path, "/tmp/%lu-%lu", (unsigned long)time(NULL), (unsigned long)getpid());
	mkdir(path, 0700);
	return path;
#endif
}

CMBOOL rbs_task(int sock, const char* section, const char* cmd, const char* arg) {
	char* tmpdir = rbs_create_tmpdir();
	char* program = rbs_get_program(section, cmd);
	char** args = rbs_parse_args(program, arg);
	char* output = NULL;
	char** translate_args = rbs_translate_args(section, cmd, args, sock, tmpdir, &output);
	CMBOOL status;
	if(translate_args == NULL) {
		free(tmpdir);
		rbs_stack_free(args);
		free(program);
		return CMFALSE;
	}
	status = rbs_start_process(program, translate_args, tmpdir);
	rbs_stack_free(translate_args);
	rbs_stack_free(args);
	if(!status) return CMFALSE;
	rbs_write(sock, "SUCCESS\n", 8);
	free(program);
	status = rbs_wait_process(sock);
	if(status) {
		char* outp = cm_strcat3(tmpdir, "/", output);
		struct stat s;
		free(tmpdir);
		if(stat(outp, &s) == 0 && s.st_size > 0) {
			char msg[128];
			unsigned long bytes = s.st_size;
			unsigned char data[512];
			FILE* fin = fopen(outp, "rb");
			rbs_write(sock, "FILE ", 5);
			sprintf(msg, "%lu", (unsigned long)s.st_mode);
			rbs_write(sock, msg, strlen(msg));
			rbs_write(sock, " ", 1);
			sprintf(msg, "%lu", (unsigned long)bytes);
			rbs_write(sock, msg, strlen(msg));
			rbs_write(sock, " ", 1);
			rbs_write(sock, output, strlen(output));
			rbs_write(sock, "\n", 1);
			while(bytes != 0) {
				int rd = fread(data, 1, bytes < 512 ? bytes : 512, fin);
				rbs_write(sock, data, rd);
				bytes -= rd;
			}
			fclose(fin);
			return CMTRUE;
		} else {
			return CMFALSE;
		}
	} else {
		free(tmpdir);
		return CMFALSE;
	}
}
