/* $Id$ */

#ifndef __RBS_SERVER_H__
#define __RBS_SERVER_H__

#include <cm_bool.h>

CMBOOL rbs_server_init(void);
CMBOOL rbs_server_loop(void);
int rbs_write(int sock, unsigned char* data, unsigned int size);
int rbs_read(int sock, unsigned char* data, unsigned int size);
char* rbs_readline(int sock);
void rbs_close(int sock);

#endif
