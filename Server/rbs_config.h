/* $Id$ */

#ifndef __RBS_CONFIG_H__
#define __RBS_CONFIG_H__

#include <cm_bool.h>

void rbs_config_init(void);
char* rbs_config_get(const char* section, const char* key);
CMBOOL rbs_config_parse(void);

struct rbs_kv {
	char* key;
	char* value;
};

struct rbs_section {
	char* name;
	struct rbs_kv attr[128];
	int used;
};

#endif
