/* $Id$ */

#include "rbs_config.h"

#include <stdio.h>

#include <cm_bool.h>

extern char* rbs_config;

void rbs_config_init(void){
}

CMBOOL rbs_config_parse(void){
	FILE* f = fopen(rbs_config, "r");
	if(f != NULL){
		fclose(f);
		return CMTRUE;
	}else{
		fprintf(stderr, "Could not open the config\n");
		return CMFALSE;
	}
}
