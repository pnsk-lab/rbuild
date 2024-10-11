/* $Id$ */

#include "rbs_auth.h"

#ifdef HAS_PAM_AUTH
#include <security/pam_appl.h>
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
