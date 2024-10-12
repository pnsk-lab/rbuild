/* $Id$ */

#include "../config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef __MINGW32__
#define WINSOCK
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include <cm_string.h>

char* rbc_readline(int sock) {
	char cbuf[2];
	char* line;
	cbuf[1] = 0;
	line = cm_strdup("");
	do {
		recv(sock, cbuf, 1, 0);
		if(cbuf[0] != '\n' && cbuf[0] != '\r') {
			char* tmp = line;
			line = cm_strcat(tmp, cbuf);
			free(tmp);
		}
	} while(cbuf[0] != '\n');
	return line;
}

int main(int argc, char** argv) {
	int i;
	char* subcmd = NULL;
	char* ip = "127.0.0.1";
	char* port = "7980";
	char* section = "default";
	char* user = "anon";
	char* pass = "anon";
	int sock;
	char* line;
	char* cmd;
	char* arg;
	int start;
	struct sockaddr_in address;
	int phase = 0;
#ifdef WINSOCK
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 0), &wsa);
#endif
	for(i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--host") == 0) {
				i++;
				if(argv[i] == NULL) {
					fprintf(stderr, "Missing option\n");
					return 1;
				}
				ip = argv[i];
			} else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
				i++;
				if(argv[i] == NULL) {
					fprintf(stderr, "Missing option\n");
					return 1;
				}
				port = argv[i];
			} else if(strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0) {
				i++;
				if(argv[i] == NULL) {
					fprintf(stderr, "Missing option\n");
					return 1;
				}
				user = argv[i];
			} else if(strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--pass") == 0) {
				i++;
				if(argv[i] == NULL) {
					fprintf(stderr, "Missing option\n");
					return 1;
				}
				pass = argv[i];
			} else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--section") == 0) {
				i++;
				if(argv[i] == NULL) {
					fprintf(stderr, "Missing option\n");
					return 1;
				}
				section = argv[i];
			} else {
				fprintf(stderr, "Invalid option: %s\n", argv[i]);
				return 1;
			}
		} else {
			subcmd = argv[i];
			start = i + 1;
			break;
		}
	}
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef WINSOCK
	if(sock == INVALID_SOCKET) return CMFALSE;
#else
	if(sock < 0) return CMFALSE;
#endif
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
	address.sin_port = htons(atoi(port));
	if(connect(sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
		fprintf(stderr, "Failed to connect\n");
		return 1;
	}

	while(1) {
		char* cmd;
		char* arg = NULL;
		line = rbc_readline(sock);
		if(line[0] == '+') {
			printf("%s\n", line + 1);
			free(line);
			continue;
		}
		cmd = line;
		for(i = 0; line[i] != 0; i++) {
			if(line[i] == ' ') {
				line[i] = 0;
				arg = line + i + 1;
				break;
			}
		}
		if(strcmp(cmd, "READY") == 0 && arg != NULL && strcmp(arg, RBUILD_VERSION) != 0) {
			free(line);
			return 1;
		} else if(strcmp(cmd, "READY") == 0 && arg != NULL && strcmp(arg, RBUILD_VERSION) == 0) {
			send(sock, "SECTION ", 8, 0);
			send(sock, section, strlen(section), 0);
			send(sock, "\n", 1, 0);
		} else if(strcmp(cmd, "FAIL") == 0) {
			if(phase == 0) {
				fprintf(stderr, "Section selection failed.\n");
			} else if(phase == 1 || phase == 2) {
				fprintf(stderr, "Login failed.\n");
			}
			return 1;
		} else if(strcmp(cmd, "SUCCESS") == 0) {
			if(phase == 0) {
				send(sock, "USER ", 5, 0);
				send(sock, user, strlen(user), 0);
				send(sock, "\n", 1, 0);
				phase++;
			} else if(phase == 1) {
				send(sock, "PASS ", 5, 0);
				send(sock, pass, strlen(pass), 0);
				send(sock, "\n", 1, 0);
				phase++;
			} else if(phase == 2) {
				int j;
				if(strcmp(subcmd, "cc") == 0) {
					send(sock, "CC", 2, 0);
				}
				for(j = start; j < argc; j++) {
					send(sock, " ", 1, 0);
					send(sock, argv[j], strlen(argv[j]), 0);
				}
				send(sock, "\n", 1, 0);
				phase++;
			}
		} else if(strcmp(cmd, "BYE") == 0) {
			free(line);
			break;
		} else if(strcmp(cmd, "GIMME") == 0 && arg != NULL) {
			struct stat s;
			if(stat(arg, &s) == 0) {
				char size[512];
				unsigned char buffer[512];
				unsigned long bytes = s.st_size;
				FILE* fin = fopen(arg, "rb");
				sprintf(size, "%lu", (unsigned long)s.st_size);
				send(sock, size, strlen(size), 0);
				send(sock, "\n", 1, 0);
				while(bytes != 0) {
					int rd = fread(buffer, 1, bytes < 512 ? bytes : 512, fin);
					send(sock, buffer, rd, 0);
					bytes -= rd;
				}
				fclose(fin);
			} else {
				send(sock, "0\n", 2, 0);
			}
		} else if(strcmp(cmd, "FILE") == 0 && arg != NULL) {
			int j;
			int count = 0;
			char* path;
			char* mode;
			char* size;
			for(j = 0; arg[j] != 0; j++) {
				if(arg[j] == ' ') {
					count++;
					arg[j] = 0;
					if(count == 1) {
						mode = arg;
						size = arg + j + 1;
					}
					if(count == 2) {
						unsigned long bytes;
						unsigned char buffer[512];
						FILE* fout;
						path = arg + j + 1;
						bytes = atol(size);
						fout = fopen(path, "wb");
						while(bytes != 0) {
							int rd = recv(sock, buffer, bytes < 512 ? bytes : 512, 0);
							fwrite(buffer, 1, rd, fout);
							bytes -= rd;
						}
						fclose(fout);
						chmod(path, atoi(mode));
						break;
					}
				}
			}
		}
		free(line);
	}
}
