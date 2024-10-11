/* $Id$ */

#include "../config.h"

#include "rbs_server.h"
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

CMBOOL rbs_server_init(void){
	if(run_inetd){
		return CMTRUE;
	}else{
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
		if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0){
			close(server_socket);
			return CMFALSE;
		}
		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(port);
		if(bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
			close(server_socket);
			fprintf(stderr, "Bind fail\n");
			return CMFALSE;
		}
		if(listen(server_socket, 128) < 0){
			close(server_socket);
			fprintf(stderr, "Listen fail\n");
			return CMFALSE;
		}
		printf("Ready\n");
		return CMTRUE;
	}
}

void rbs_write(int sock, unsigned char* data, unsigned int size){
	if(run_inetd){
		fwrite(data, 1, size, stdout);
		fflush(stdout);
	}else{
		send(sock, data, size, 0);
	}
}

void rbs_close(int sock){
#ifdef WINSOCK
	closesocket(sock);
#else
	close(sock);
#endif
}

void rbs_server_handler(void* sockptr){
	int sock = -1;
	if(sockptr != NULL){
		sock = *(int*)sockptr;
		free(sockptr);
	}
	rbs_write(sock, "READY ", 6);
	rbs_write(sock, ready, strlen(ready));
	rbs_write(sock, "\n", 1);

	while(1){
		fd_set rfds;
		struct timeval tv;
		int ret;

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		ret = select(1, &rfds, NULL, NULL, &tv);

		if(ret < 0){
			break;
		}else if(ret == 0){
			rbs_write(sock, "TIMEOUT\n", 8);
			break;
		}else{
		}
	}

	rbs_close(sock);
#ifdef WINSOCK
	_endthread();
#endif
}

CMBOOL rbs_server_loop(void){
	if(run_inetd){
		rbs_server_handler(NULL);
	}else{
		ready = cm_strdup(RBUILD_VERSION);
#ifndef WINSOCK
		signal(SIGCHLD, SIG_IGN);
#endif
		while(1){
			struct sockaddr_in claddr;
			int clen = sizeof(claddr);
			int sock = accept(server_socket, (struct sockaddr*)&claddr, &clen);
#ifdef WINSOCK
			int* sockptr = malloc(sizeof(*sockptr));
			*sockptr = sock;
			_beginthread(rbs_server_handler, 0, sockptr);
#else
			pid_t p = fork();
			if(p == 0){
				int* sockptr = malloc(sizeof(*sockptr));
				*sockptr = sock;
				rbs_server_handler(sockptr);
				_exit(0);
			}else{
				rbs_close(sock);
			}
#endif
		}
	}
}
