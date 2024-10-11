/* $Id$ */

#include "../config.h"

#include <stdio.h>
#include <string.h>

#include "rbs_server.h"
#include "rbs_config.h"

#include <cm_bool.h>

extern CMBOOL run_inetd;

char* rbs_config = NULL;
extern const char* rbs_none_auth;
extern const char* rbs_crypt_auth;
extern const char* rbs_plain_auth;
extern const char* rbs_pam_auth;

int main(int argc, char** argv){
	int i;
	CMBOOL dryrun = CMFALSE;
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
				rbs_config = argv[i];
			}else if(strcmp(argv[i], "--dry-run") == 0 || strcmp(argv[i], "-d") == 0){
				dryrun = CMTRUE;
			}else if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-V") == 0){
				printf("rbuild-server version %s\n", RBUILD_VERSION);
				printf("Authentication methods:\n");
				printf("\tNone  : %s\n", rbs_none_auth);
				printf("\tCrypt : %s\n", rbs_crypt_auth);
				printf("\tPlain : %s\n", rbs_plain_auth);
				printf("\tPAM   : %s\n", rbs_pam_auth);
				return 0;
			}else{
				fprintf(stderr, "Unknown option: %s\n", argv[i]);
				return 1;
			}
		}else{
		}
	}
	if(rbs_config == NULL){
		fprintf(stderr, "Config is required\n");
		return 1;
	}
	rbs_config_init();
	if(!rbs_config_parse()){
		fprintf(stderr, "Failed to parse config\n");
		return 1;
	}
	if(dryrun) return 0;
	if(!rbs_server_init()){
		fprintf(stderr, "Failed to initialize\n");
		return 1;
	}
	if(!rbs_server_loop()){
		return 1;
	}
	return 0;
}
