/* $Id$ */

#include "rbs_config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <cm_string.h>
#include <cm_bool.h>

extern CMBOOL run_inetd;

struct rbs_section sections[1024];
int used_sections = -1;

extern char* rbs_config;

void rbs_config_init(void) {
	int i;
	for(i = 0; i <= used_sections; i++) {
		int j;
		for(j = 0; j < sections[i].used; j++) {
			free(sections[i].attr[j].key);
			free(sections[i].attr[j].value);
		}
		free(sections[i].name);
	}
	used_sections = -1;
}

char* rbs_config_get(const char* section, const char* key) {
	int i;
	for(i = 0; i <= used_sections; i++) {
		if(strcmp(sections[i].name, section) == 0) {
			int j;
			for(j = 0; j < sections[i].used; j++) {
				if(strcmp(sections[i].attr[j].key, key) == 0) return sections[i].attr[j].value;
			}
		}
	}
	return "";
}

CMBOOL rbs_config_parse(void) {
	FILE* f = fopen(rbs_config, "r");
	if(f != NULL) {
		struct stat s;
		if(stat(rbs_config, &s) == 0) {
			char* buf = malloc(s.st_size + 1);
			int i;
			int incr = 0;
			buf[s.st_size] = 0;
			fread(buf, 1, s.st_size, f);
			for(i = 0;; i++) {
				if(buf[i] == 0 || buf[i] == '\n') {
					char oldc = buf[i];
					char* line;
					buf[i] = 0;

					line = buf + incr;
					if(strlen(line) > 0 && line[0] != '#') {
						if(line[0] == '[' && line[strlen(line) - 1] == ']') {
							line[strlen(line) - 1] = 0;
							used_sections++;
							sections[used_sections].name = cm_strdup(line + 1);
							sections[used_sections].used = 0;
							if(!run_inetd) printf("Adding section `%s'\n", line + 1);
						} else if(used_sections == -1) {
						} else {
							int j;
							for(j = 0; line[j] != 0; j++) {
								if(line[j] == '=') {
									line[j] = 0;
									sections[used_sections].attr[sections[used_sections].used].key = cm_strdup(line);
									sections[used_sections].attr[sections[used_sections].used].value = cm_strdup(line + j + 1);
									if(!run_inetd) printf("\t%s: `%s'\n", line, line + j + 1);
									sections[used_sections].used++;
									break;
								}
							}
						}
					}

					incr = i + 1;
					if(oldc == 0) break;
				} else if(buf[i] == '\r') {
					buf[i] = 0;
				}
			}
			free(buf);
		}
		fclose(f);
		if(used_sections == -1) return CMFALSE;
		return CMTRUE;
	} else {
		fprintf(stderr, "Could not open the config\n");
		return CMFALSE;
	}
}
