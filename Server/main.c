/* $Id$ */

#include <stdio.h>
#include <string.h>

#include "rbs_server.h"

#include <cm_bool.h>

extern CMBOOL run_inetd;

int main(int argc, char** argv){
	int i;
	for(i = 1; i < argc; i++){
		if(argv[i][0] == '-'){
			if(strcmp(argv[i], "--inetd") == 0 || strcmp(argv[i], "-i") == 0){
				run_inetd = CMTRUE;
			}else if(strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-C") == 0){
				i++;
				if(argv[i] == NULL){
					fprintf(stderr, "Missing argument\n");
					return 1;
				}
			}else{
				fprintf(stderr, "Unknown option: %s\n", argv[i]);
				return 1;
			}
		}else{
		}
	}
	if(!rbs_server_init()){
		fprintf(stderr, "Failed to initialize\n");
		return 1;
	}
	if(!rbs_server_loop()){
		return 1;
	}
	return 0;
}
