/*
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
 */


#include "../../sr_module.h"
#include "../../lib/kmi/mi.h"

#include "geolocation_mod.h"


/* module functions */
static int mod_init(void);
static int child_init(int rank);
static void mod_destroy(void);

/* mi functions */

/* module exports */
static cmd_export_t cmds[] = {
	{0, 0, 0, 0, 0, 0}
};

static pv_export_t mod_pvs[] = {
	{{0, 0}, 0, 0, 0, 0, 0, 0, 0}
};

static param_export_t params[] = {
	{0, 0, 0}
};

static mi_export_t mi_cmds[] = {
	{ 0, 0, 0, 0, 0}
};


struct module_exports exports = {
	"geolocation",
	DEFAULT_DLFLAGS,/* dlopen flags */
	cmds,
	params,
	0,           /* exported statistics */
	mi_cmds,     /* exported MI functions */
	mod_pvs,     /* exported pseudo-variables */
	0,           /* extra processes */
	mod_init,
	0,           /* reply processing */
	mod_destroy, /* destroy function */
	child_init
};


static int mod_init(void)
{
	if (register_mi_mod(exports.name, mi_cmds) != 0) {
		LM_ERR("failed to register MI commands\n");
		return -1;
	}

	return 0;
}


static int child_init(int rank)
{
	return 0;
}


static void mod_destroy(void)
{
	return ;
}
