/* 
 * History-Info Header Field Support
 *
 * Copyright (C) 2004 FhG Fokus
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <string.h>
#include "../../core/sr_module.h"
#include "../../core/error.h"
#include "../../core/dprint.h"
#include "../../core/mem/mem.h"
#include "../../core/data_lump.h"
#include "../../core/mod_fix.h"
#include "../../core/parser/parse_uri.h"
#include "../../core/kemi.h"


MODULE_VERSION

#define HISTORY_INFO_HF "History-Info"
#define HISTORY_INFO_HF_LEN (sizeof(HISTORY_INFO_HF) - 1)

#define HISTORY_INFO_PREFIX     HISTORY_INFO_HF ": <"
#define HISTORY_INFO_PREFIX_LEN (sizeof(HISTORY_INFO_PREFIX) - 1)

#define HISTORY_INFO_SUFFIX     ">;index="
#define HISTORY_INFO_SUFFIX_LEN (sizeof(HISTORY_INFO_SUFFIX) - 1)



str suffix = {"", 0};

int w_add_history_info(struct sip_msg* msg, char* r, char* u);
int w_del_history_info(struct sip_msg* msg);

static struct hdr_field *get_last_history_info(struct sip_msg* msg);


/*
 * Exported functions
 */
static cmd_export_t cmds[] = {
	{"del_history_info",    (cmd_function)w_del_history_info,    0, 0,
		0, REQUEST_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE},
	{"add_history_info",    (cmd_function)w_add_history_info,    1, fixup_spve_null,
		0, REQUEST_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE},
	{"add_history_info",    (cmd_function)w_add_history_info,    2, fixup_spve_spve,
		0, REQUEST_ROUTE|FAILURE_ROUTE|BRANCH_ROUTE},
	{0, 0, 0, 0, 0, 0}
};


/*
 * Exported parameters
 */
static param_export_t params[] = {
	{"suffix", PARAM_STR, &suffix},
	{0, 0, 0}
};


/*
 * Module interface
 */
struct module_exports exports = {
	"history_info",		/* module name */
	DEFAULT_DLFLAGS,	/* dlopen flags */
	cmds,				/* exported functions */
	params,				/* exported parameters */
	0,					/* RPC method exports */
	0,					/* exported pseudo-variables */
	0,					/* response handling function */
	0,					/* module initialization function */
	0,					/* per-child init function */
	0					/* module destroy function */
};


static struct hdr_field *get_last_history_info(struct sip_msg* msg) {
	struct hdr_field *hf = 0, *prev_hf = 0;

        // parse HFs
        if (parse_headers(msg, HDR_EOH_F, 0) < 0) {
                LM_ERR("error while parsing message headers\n");
                return NULL;
        }

        // check HFs
	// prev_hf will point to the last History-Info, if any
        for (hf = msg->headers; hf; hf = hf->next) {
		if (hf->name.len != HISTORY_INFO_HF_LEN) {
			continue;
		}

		if (strncmp(hf->name.s, HISTORY_INFO_HF, HISTORY_INFO_HF_LEN) != 0) {
			continue;
		}

		prev_hf = hf;
        }

	return prev_hf;
}

static inline int add_history_info_helper(struct sip_msg* msg, str* s)
{
	struct hdr_field *last_hf = 0;
	struct lump* anchor = 0;
	char *ptr;

	// get last History-Info
	last_hf = get_last_history_info(msg);

	// check last History-Info
	if (last_hf) {
		     // insert just after the last History-Info
		ptr = last_hf->name.s + last_hf->len;
	} else {
		     // insert at the end
		ptr = msg->unparsed;
	}

	// prepare to insert new History-Info
	anchor = anchor_lump(msg, ptr - msg->buf, 0, 0);
	if (!anchor) {
		LM_ERR("can't get anchor\n");
		return -1;
	}
	
	// insert new History-Info
	if (!insert_new_lump_before(anchor, s->s, s->len, 0)) {
		LM_ERR("can't insert lump\n");
		return -2;
	}

	return 1;
}


int add_history_info_uri(sip_msg_t* msg, str* reason, str* uri)
{
	str div_hf;
	char *at;

	if(reason==NULL || reason->s==NULL || uri==NULL || uri->s==NULL) {
		LM_ERR("invalid parameters\n");
		return -1;
	}

	div_hf.len = HISTORY_INFO_PREFIX_LEN + uri->len + HISTORY_INFO_SUFFIX_LEN
					+ reason->len + CRLF_LEN;
	div_hf.s = pkg_malloc(div_hf.len);
	if (!div_hf.s) {
		LM_ERR("no pkg memory left\n");
		return -1;
	}

	at = div_hf.s;
	memcpy(at, HISTORY_INFO_PREFIX, HISTORY_INFO_PREFIX_LEN);
	at += HISTORY_INFO_PREFIX_LEN;

	memcpy(at, uri->s, uri->len);
	at += uri->len;

	memcpy(at, HISTORY_INFO_SUFFIX, HISTORY_INFO_SUFFIX_LEN);
	at += HISTORY_INFO_SUFFIX_LEN;

	memcpy(at, reason->s, reason->len);
	at += reason->len;

	memcpy(at, CRLF, CRLF_LEN);

	if (add_history_info_helper(msg, &div_hf) < 0) {
		pkg_free(div_hf.s);
		return -1;
	}

	return 1;
}

int w_add_history_info(struct sip_msg* msg, char* r, char* u)
{
	str uri;
	str reason;

	if(fixup_get_svalue(msg, (gparam_t*)r, &reason)<0) {
		LM_ERR("cannot get the reason parameter\n");
		return -1;
	}

	if(u==NULL) {
		if(parse_sip_msg_uri(msg)<0) {
			LM_ERR("failed to parse sip msg uri\n");
			return -1;
		}
		uri = msg->first_line.u.request.uri;
	} else {
		if(fixup_get_svalue(msg, (gparam_t*)u, &uri)<0)
		{
			LM_ERR("cannot get the uri parameter\n");
			return -1;
		}
	}
	return add_history_info_uri(msg, &reason, &uri);
}

int w_del_history_info(struct sip_msg* msg) {
	struct hdr_field *last_hf = 0;

	// get last History-Info
	last_hf = get_last_history_info(msg);

        // check last History-Info
	if (!last_hf) {
		return 1;
	}

	// delete last History-Info
	if (!del_lump(msg, last_hf->name.s - msg->buf, last_hf->len, 0)) {
		LM_ERR("can't delete lump\n");
		return -3;
	}

	return 1;
}

/**
 *
 */
static int ki_add_history_info(sip_msg_t *msg, str *reason)
{
	if(parse_sip_msg_uri(msg)<0) {
		LM_ERR("failed to parse sip msg uri\n");
		return -1;
	}

	return add_history_info_uri(msg, reason, &msg->first_line.u.request.uri);
}

/**
 *
 */
/* clang-format off */
static sr_kemi_t sr_kemi_history_info_exports[] = {
	{ str_init("history_info"), str_init("add_history_info"),
		SR_KEMIP_INT, ki_add_history_info,
		{ SR_KEMIP_STR, SR_KEMIP_NONE, SR_KEMIP_NONE,
			SR_KEMIP_NONE, SR_KEMIP_NONE, SR_KEMIP_NONE }
	},
	{ str_init("history_info"), str_init("add_history_info_uri"),
		SR_KEMIP_INT, add_history_info_uri,
		{ SR_KEMIP_STR, SR_KEMIP_STR, SR_KEMIP_NONE,
			SR_KEMIP_NONE, SR_KEMIP_NONE, SR_KEMIP_NONE }
	},

	{ {0, 0}, {0, 0}, 0, NULL, { 0, 0, 0, 0, 0, 0 } }
};
/* clang-format on */

/**
 *
 */
int mod_register(char *path, int *dlflags, void *p1, void *p2)
{
	sr_kemi_modules_add(sr_kemi_history_info_exports);
	return 0;
}
