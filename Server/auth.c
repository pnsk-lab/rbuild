/* $Id$ */

#include "rbs_auth.h"

#include "rbs_config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cm_bool.h>
#include <cm_string.h>

#ifdef HAS_PAM_AUTH
#include <security/pam_appl.h>
#endif

#ifdef HAS_CRYPT_AUTH
#ifdef __linux__
#include <crypt.h>
#endif
#endif

const char* rbs_pam_auth =
#ifdef HAS_PAM_AUTH
    "available"
#else
    "not available"
#endif
    ;

const char* rbs_plain_auth =
#ifdef HAS_PLAIN_AUTH
    "available"
#else
    "not available"
#endif
    ;

const char* rbs_crypt_auth =
#ifdef HAS_CRYPT_AUTH
    "available"
#else
    "not available"
#endif
    ;

const char* rbs_none_auth =
#ifdef HAS_NONE_AUTH
    "available"
#else
    "not available"
#endif
    ;

#ifdef HAS_PLAIN_AUTH
CMBOOL rbs_auth_plain(const char* fn, const char* username, const char* password) {
	FILE* f;
	struct stat s;
	char* buffer;
	int i;
	int incr = 0;
	int state = CMFALSE;
	if(fn == NULL) return CMFALSE;
	f = fopen(fn, "r");
	if(f == NULL) return CMFALSE;
	if(stat(fn, &s) != 0) {
		fclose(f);
		return CMFALSE;
	}

	buffer = malloc(s.st_size + 1);
	buffer[s.st_size] = 0;

	fread(buffer, 1, s.st_size, f);
	for(i = 0;; i++) {
		if(buffer[i] == 0 || buffer[i] == '\n') {
			char oldc = buffer[i];
			char* line = buffer + incr;
			buffer[i] = 0;

			if(strlen(line) > 0 && line[0] != '#') {
				int j;
				for(j = 0; line[j] != 0; j++) {
					if(line[j] == ':') {
						line[j] = 0;
						if(strcmp(username, line) == 0 && strcmp(password, line + j + 1) == 0) {
							state = CMTRUE;
						}
						break;
					}
				}
			}

			incr = i + 1;
			if(oldc == 0 || state) break;
		} else if(buffer[i] == '\r') {
			buffer[i] = 0;
		}
	}

	free(buffer);
	fclose(f);
	return state;
}
#endif

#ifdef HAS_CRYPT_AUTH
CMBOOL rbs_auth_crypt(const char* fn, const char* username, const char* password) {
	FILE* f;
	struct stat s;
	char* buffer;
	int i;
	int incr = 0;
	int state = CMFALSE;
	if(fn == NULL) return CMFALSE;
	f = fopen(fn, "r");
	if(f == NULL) return CMFALSE;
	if(stat(fn, &s) != 0) {
		fclose(f);
		return CMFALSE;
	}

	buffer = malloc(s.st_size + 1);
	buffer[s.st_size] = 0;

	fread(buffer, 1, s.st_size, f);
	for(i = 0;; i++) {
		if(buffer[i] == 0 || buffer[i] == '\n') {
			char oldc = buffer[i];
			char* line = buffer + incr;
			buffer[i] = 0;

			if(strlen(line) > 0 && line[0] != '#') {
				int j;
				for(j = 0; line[j] != 0; j++) {
					if(line[j] == ':') {
						line[j] = 0;
						if(strcmp(username, line) == 0 && strcmp(crypt(password, line + j + 1), line + j + 1) == 0) {
							state = CMTRUE;
						}
						break;
					}
				}
			}

			incr = i + 1;
			if(oldc == 0 || state) break;
		} else if(buffer[i] == '\r') {
			buffer[i] = 0;
		}
	}

	free(buffer);
	fclose(f);
	return state;
}
#endif

#ifdef HAS_PAM_AUTH
int rbs_pam_conv(int nmsg, const struct pam_message** msg, struct pam_response** resp, void* appdata) {
	int i;
	*resp = malloc(sizeof(**resp) * nmsg);
	for(i = 0; i < nmsg; i++) {
		if((*msg)[i].msg_style == PAM_PROMPT_ECHO_OFF || (*msg)[i].msg_style == PAM_PROMPT_ECHO_ON) {
			(*resp)[i].resp = cm_strdup((const char*)appdata);
			(*resp)[i].resp_retcode = 0;
		}
	}
	return PAM_SUCCESS;
}

CMBOOL rbs_auth_pam(const char* arg, const char* username, const char* password) {
	CMBOOL state = CMFALSE;
	struct pam_conv conv;
	pam_handle_t* pamh = NULL;
	int retval;
	conv.conv = rbs_pam_conv;
	conv.appdata_ptr = (void*)password;
	retval = pam_start(arg == NULL ? "rbuild" : arg, username, &conv, &pamh);
	if(retval == PAM_SUCCESS) {
		retval = pam_authenticate(pamh, 0);
	}
	if(retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt(pamh, 0);
	}
	if(retval == PAM_SUCCESS) {
		state = CMTRUE;
	}
	pam_end(pamh, retval);
	return state;
}
#endif

CMBOOL rbs_auth(const char* section, const char* username, const char* password) {
	char* auth = cm_strdup(rbs_config_get(section, "auth"));
	int i;
	char* arg = NULL;
	CMBOOL state = CMFALSE;
	for(i = 0; auth[i] != 0; i++) {
		if(auth[i] == ':') {
			arg = auth + i + 1;
			auth[i] = 0;
			break;
		}
	}
#ifdef HAS_NONE_AUTH
	if(strcmp(auth, "none") == 0) {
		state = CMTRUE;
	} else
#endif
#ifdef HAS_PAM_AUTH
	    if(strcmp(auth, "pam") == 0) {
		state = rbs_auth_pam(arg, username, password);
	} else
#endif
#ifdef HAS_PLAIN_AUTH
	    if(strcmp(auth, "plain") == 0) {
		state = rbs_auth_plain(arg, username, password);
	} else
#endif
#ifdef HAS_CRYPT_AUTH
	    if(strcmp(auth, "crypt") == 0) {
		state = rbs_auth_crypt(arg, username, password);
	} else
#endif
	    if(1) {
		free(auth);
		return CMFALSE;
	}
	free(auth);
	return state;
}
