/* $Id$ */

#ifndef __CM_STRING_H__
#define __CM_STRING_H__

#include <cm_bool.h>

int cm_hex(const char* str, int len);
CMBOOL cm_nocase_endswith(const char* str, const char* end);
CMBOOL cm_endswith(const char* str, const char* end);
char* cm_html_escape(const char* str);
char* cm_url_escape(const char* str);
char* cm_strcat(const char* a, const char* b);
char* cm_strcat3(const char* a, const char* b, const char* c);
char* cm_strdup(const char* str);
char* cm_trimstart(const char* str);
char* cm_trimend(const char* str);
char* cm_trim(const char* str);
char** cm_split(const char* str, const char* by);
CMBOOL cm_strcaseequ(const char* a, const char* b);

#endif
