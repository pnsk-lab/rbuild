/* $Id$ */

#include <string.h>
#include <stdlib.h>
#include <cm_bool.h>
#include <stdio.h>
#include <ctype.h>

char* cm_strcat(const char* a, const char* b) {
	char* str;
	if(a == NULL) a = "";
	if(b == NULL) b = "";
	str = malloc(strlen(a) + strlen(b) + 1);
	memcpy(str, a, strlen(a));
	memcpy(str + strlen(a), b, strlen(b));
	str[strlen(a) + strlen(b)] = 0;
	return str;
}

char* cm_strcat3(const char* a, const char* b, const char* c) {
	char* tmp = cm_strcat(a, b);
	char* str = cm_strcat(tmp, c);
	free(tmp);
	return str;
}

char* cm_strdup(const char* str) { return cm_strcat(str, ""); }

CMBOOL cm_endswith(const char* str, const char* end) {
	int i;
	if(strlen(str) < strlen(end)) return CMFALSE;
	for(i = strlen(str) - strlen(end); i < strlen(str); i++) {
		if(str[i] != end[i - strlen(str) + strlen(end)]) return CMFALSE;
	}
	return CMTRUE;
}

CMBOOL cm_nocase_endswith(const char* str, const char* end) {
	int i;
	if(strlen(str) < strlen(end)) return CMFALSE;
	for(i = strlen(str) - strlen(end); i < strlen(str); i++) {
		if(tolower(str[i]) != tolower(end[i - strlen(str) + strlen(end)])) return CMFALSE;
	}
	return CMTRUE;
}

char* cm_trimstart(const char* str) {
	int i;
	for(i = 0; str[i] != 0; i++) {
		if(str[i] != ' ' && str[i] != '\t') {
			return cm_strdup(str + i);
		}
	}
	return cm_strdup("");
}

char* cm_trimend(const char* str) {
	char* s = cm_strdup(str);
	int i;
	for(i = strlen(s) - 1; i >= 0; i--) {
		if(s[i] != '\t' && s[i] != ' ') {
			s[i + 1] = 0;
			break;
		}
	}
	return s;
}

char* cm_trim(const char* str) {
	char* tmp = cm_trimstart(str);
	char* s = cm_trimend(tmp);
	free(tmp);
	return s;
}

char** cm_split(const char* str, const char* by) {
	int i;
	char** r = malloc(sizeof(*r));
	char* b = malloc(1);
	char cbuf[2];
	CMBOOL dq = CMFALSE;
	CMBOOL sq = CMFALSE;
	r[0] = NULL;
	b[0] = 0;
	cbuf[1] = 0;
	for(i = 0;; i++) {
		int j;
		CMBOOL has = CMFALSE;
		for(j = 0; by[j] != 0; j++) {
			if(by[j] == str[i]) {
				has = CMTRUE;
				break;
			}
		}
		if(!(dq || sq) && (has || str[i] == 0)) {
			if(strlen(b) > 0) {
				char** old = r;
				int j;
				for(j = 0; old[j] != NULL; j++)
					;
				r = malloc(sizeof(*r) * (j + 2));
				for(j = 0; old[j] != NULL; j++) r[j] = old[j];
				r[j] = b;
				r[j + 1] = NULL;
				free(old);
			}
			b = malloc(1);
			b[0] = 0;
			if(str[i] == 0) break;
		} else {
			if(str[i] == '"' && !sq) {
				dq = !dq;
			} else if(str[i] == '\'' && !dq) {
				sq = !sq;
			} else {
				char* tmp = b;
				cbuf[0] = str[i];
				b = cm_strcat(tmp, cbuf);
				free(tmp);
			}
		}
	}
	free(b);
	return r;
}

CMBOOL cm_strcaseequ(const char* a, const char* b) {
	int i;
	if(a == NULL) return CMFALSE;
	if(b == NULL) return CMFALSE;
	if(strlen(a) != strlen(b)) return CMFALSE;
	for(i = 0; a[i] != 0; i++) {
		if(tolower(a[i]) != tolower(b[i])) return CMFALSE;
	}
	return CMTRUE;
}

int cm_hex(const char* str, int len) {
	int n = 0;
	int i;
	for(i = 0; i < len; i++) {
		char c = str[i];
		n *= 16;
		if('0' <= c && c <= '9') {
			n += c - '0';
		} else if('a' <= c && c <= 'f') {
			n += c - 'a' + 10;
		} else if('A' <= c && c <= 'F') {
			n += c - 'A' + 10;
		}
	}
	return n;
}

char* cm_html_escape(const char* str) {
	int i;
	char* result = malloc(1);
	char cbuf[2];
	result[0] = 0;
	cbuf[1] = 0;
	for(i = 0; str[i] != 0; i++) {
		cbuf[0] = str[i];
		if(str[i] == '&') {
			char* tmp = result;
			result = cm_strcat(tmp, "&amp;");
			free(tmp);
		} else if(str[i] == '<') {
			char* tmp = result;
			result = cm_strcat(tmp, "&lt;");
			free(tmp);
		} else if(str[i] == '>') {
			char* tmp = result;
			result = cm_strcat(tmp, "&gt;");
			free(tmp);
		} else {
			char* tmp = result;
			result = cm_strcat(tmp, cbuf);
			free(tmp);
		}
	}
	return result;
}

char* cm_url_escape(const char* str) {
	int i;
	char* result = malloc(1);
	char cbuf[2];
	result[0] = 0;
	cbuf[1] = 0;
	for(i = 0; str[i] != 0; i++) {
		cbuf[0] = str[i];
		if('!' <= str[i] && str[i] <= '@' && str[i] != '.' && str[i] != '-' && str[i] != '/' && !('0' <= str[i] && str[i] <= '9')) {
			char code[4];
			char* tmp = result;
			sprintf(code, "%%%02X", str[i]);
			result = cm_strcat(tmp, code);
			free(tmp);
		} else {
			char* tmp = result;
			result = cm_strcat(tmp, cbuf);
			free(tmp);
		}
	}
	return result;
}
