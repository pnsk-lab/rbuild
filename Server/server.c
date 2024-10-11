/* $Id$ */

#include "../config.h"

#include "rbs_server.h"
#include "rbs_config.h"
#include "rbs_auth.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef __MINGW32__
#define WINSOCK
#include <winsock2.h>
#include <process.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#endif

#include <cm_string.h>
#include <cm_bool.h>

char* ready;
CMBOOL run_inetd = CMFALSE;
int server_socket;
int port = 7980;

CMBOOL rbs_server_init(void) {
	ready = cm_strdup(RBUILD_VERSION);
	if(run_inetd) {
		return CMTRUE;
	} else {
		struct sockaddr_in server_address;
		int yes = 1;
#ifdef WINSOCK
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 0), &wsa);
#endif
		server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef WINSOCK
		if(server_socket == INVALID_SOCKET) return CMFALSE;
#else
		if(server_socket < 0) return CMFALSE;
#endif
		if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0) {
			close(server_socket);
			return CMFALSE;
		}
		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(port);
		if(bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
			close(server_socket);
			fprintf(stderr, "Bind fail\n");
			return CMFALSE;
		}
		if(listen(server_socket, 128) < 0) {
			close(server_socket);
			fprintf(stderr, "Listen fail\n");
			return CMFALSE;
		}
		printf("Ready\n");
		return CMTRUE;
	}
}

void rbs_write(int sock, unsigned char* data, unsigned int size) {
	if(run_inetd) {
		fwrite(data, 1, size, stdout);
		fflush(stdout);
	} else {
		send(sock, data, size, 0);
	}
}

void rbs_close(int sock) {
#ifdef WINSOCK
	closesocket(sock);
#else
	close(sock);
#endif
}

char* rbs_readline(int sock) {
	char cbuf[2];
	char* line;
	cbuf[1] = 0;
	line = cm_strdup("");
	do {
		fd_set rfds;
		struct timeval tv;
		int ret;

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		ret = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
		if(ret <= 0) {
			free(line);
			return NULL;
		}
		if(run_inetd) {
			fread(cbuf, 1, 1, stdin);
		} else {
			recv(sock, cbuf, 1, 0);
		}
		if(cbuf[0] != '\n' && cbuf[0] != '\r') {
			char* tmp = line;
			line = cm_strcat(tmp, cbuf);
			free(tmp);
		}
	} while(cbuf[0] != '\n');
	return line;
}

void rbs_server_handler(void* sockptr) {
	int sock = 0;
	char* user = NULL;
	char* pass = NULL;
	char* section = NULL;
	CMBOOL authed = CMFALSE;
	if(sockptr != NULL) {
		sock = *(int*)sockptr;
		free(sockptr);
	}
	rbs_write(sock, "READY ", 6);
	rbs_write(sock, ready, strlen(ready));
	rbs_write(sock, "\n", 1);

	while(1) {
		fd_set rfds;
		struct timeval tv;
		int ret;

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		ret = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);

		if(ret < 0) {
			break;
		} else if(ret == 0) {
			break;
		} else {
			char* line = rbs_readline(sock);
			int i;
			char* arg = NULL;
			char* cmd = line;
			if(line == NULL) {
				break;
			}
			for(i = 0; line[i] != 0; i++) {
				if(line[i] == ' ') {
					line[i] = 0;
					arg = line + i + 1;
					break;
				}
			}
			if(strcmp(cmd, "QUIT") == 0) {
				free(line);
				break;
			} else if(strcmp(cmd, "SECTION") == 0 && arg != NULL) {
				if(strcmp(rbs_config_get(arg, "auth"), "") == 0 || section != NULL) {
					free(line);
					break;
				}
				section = cm_strdup(arg);
			} else if(strcmp(cmd, "USER") == 0 && arg != NULL) {
				if(section == NULL || user != NULL) {
					free(line);
					break;
				}
				user = cm_strdup(arg);
			} else if(strcmp(cmd, "PASS") == 0 && arg != NULL) {
				if(user == NULL || pass != NULL) {
					free(line);
					break;
				}
				pass = cm_strdup(arg);
				if(rbs_auth(section, user, pass)) {
					rbs_write(sock, "SUCCESS\n", 8);
					authed = CMTRUE;
				} else {
					rbs_write(sock, "FAIL\n", 5);
					free(line);
					break;
				}
			} else {
				free(line);
				break;
			}
			free(line);
		}
	}

	rbs_close(sock);
#ifdef WINSOCK
	_endthread();
#endif
}

CMBOOL rbs_server_loop(void) {
	if(run_inetd) {
		setvbuf(stdin, NULL, _IONBF, 0);
		rbs_server_handler(NULL);
	} else {
#ifndef WINSOCK
		signal(SIGCHLD, SIG_IGN);
#endif
		while(1) {
			struct sockaddr_in claddr;
			int clen = sizeof(claddr);
			int sock = accept(server_socket, (struct sockaddr*)&claddr, &clen);
#ifdef WINSOCK
			int* sockptr = malloc(sizeof(*sockptr));
			*sockptr = sock;
			_beginthread(rbs_server_handler, 0, sockptr);
#else
			pid_t p = fork();
			if(p == 0) {
				int* sockptr = malloc(sizeof(*sockptr));
				*sockptr = sock;
				rbs_server_handler(sockptr);
				_exit(0);
			} else {
				rbs_close(sock);
			}
#endif
		}
	}
}
