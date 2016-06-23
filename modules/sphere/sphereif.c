/*
 * $Id$
 *
 * CASSANDRA module interface
 *
 * Copyright (C) 2012 1&1 Internet AG
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * --------
 * 2012-01  first version (Anca Vamanu)
 */

#include <stdio.h>
#include <stdlib.h>

#include "../../sr_module.h"
#include "../../parser/msg_parser.h"

static int sphere_mod_init(void);
static void mod_destroy(void);

MODULE_VERSION


extern int cpp_sphere_alloc_route(struct hdr_field * callid, char * ip, char * number);
extern int cpp_sphere_release_route(struct hdr_field * callid);

str sphere_oracle_db={0, 0};
/*
 *  database module interface
 */
 static int sphere_alloc_route(struct sip_msg* msg, char* ip, char* number)
 {
 		return cpp_sphere_alloc_route(msg->callid, ip, number);
 }

 static int sphere_release_route(struct sip_msg * msg) {
	 return cpp_sphere_release_route(msg->callid);
 }



static cmd_export_t cmds[] = {
	{"sphere_alloc_route", (cmd_function)sphere_alloc_route, 2, 0, 0, ANY_ROUTE},
	{"sphere_release_route", (cmd_function)sphere_release_route, 0, 0, 0, ANY_ROUTE},
	{0, 0, 0, 0,0, 0}
};


static param_export_t params[] = {
	{"oracle_db",      PARAM_STR,  &sphere_oracle_db.s},
	{0, 0, 0}
};

#define GET_INT_PARAM(X) \
extern unsigned int cpp_sphere_get_##X(struct hdr_field * callid); \
static int pv_get_##X(struct sip_msg *msg, pv_param_t *param,	pv_value_t *res) \
{ \
	if(msg==NULL || param==NULL) \
		return -1; \
  unsigned int val = cpp_sphere_get_##X(msg->callid); \
  return pv_get_uintval(msg, param, res, val); \
}

#define GET_STRING_PARAM(X) \
extern char * cpp_sphere_get_##X(struct hdr_field * callid); \
static int pv_get_##X(struct sip_msg *msg, pv_param_t *param,	pv_value_t *res) \
{ \
	if(msg==NULL || param==NULL) \
		return -1; \
  char * strval = cpp_sphere_get_##X(msg->callid); \
  str val;\
  val.s = strval;\
  val.len = strlen(strval);\
  return pv_get_strval(msg, param, res, &val); \
}

GET_INT_PARAM(egress_port);
GET_STRING_PARAM(egress_ip);
GET_INT_PARAM(termination_port);
GET_STRING_PARAM(termination_ip);
GET_STRING_PARAM(suffix);
GET_STRING_PARAM(prefix);
GET_STRING_PARAM(route);

static pv_export_t mod_items[] = {
	{ {"sphere_egress_port", (sizeof("sphere_egress_port")-1)}, PVT_OTHER, pv_get_egress_port,
		0, 0, 0, 0, 0},
 { {"sphere_egress_ip", (sizeof("sphere_egress_ip")-1)}, PVT_OTHER, pv_get_egress_ip,
  		0, 0, 0, 0, 0},
  { {"sphere_termination_ip", (sizeof("sphere_termination_ip")-1)}, PVT_OTHER, pv_get_termination_ip,
        0, 0, 0, 0, 0},
  { {"sphere_termination_port", (sizeof("sphere_termination_port")-1)}, PVT_OTHER, pv_get_termination_port,
              0, 0, 0, 0, 0},
  { {"sphere_suffix", (sizeof("sphere_suffix")-1)}, PVT_OTHER, pv_get_suffix,
                          0, 0, 0, 0, 0},
  { {"sphere_prefix", (sizeof("sphere_prefix")-1)}, PVT_OTHER, pv_get_prefix,
                                      0, 0, 0, 0, 0},
  { {"sphere_route", (sizeof("sphere_route")-1)}, PVT_OTHER, pv_get_route,
                                                  0, 0, 0, 0, 0},
	{ {0, 0}, 0, 0, 0, 0, 0, 0, 0 }
};

struct module_exports exports = {
	"sphere",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	params,          /* module parameters */
	0,               /* exported statistics */
	0,               /* exported MI functions */
	mod_items,               /* exported pseudo-variables */
	0,               /* extra processes */
	sphere_mod_init,  /* module initialization function */
	0,               /* response function*/
	mod_destroy,     /* destroy function */
	0                /* per-child init function */
};

static int sphere_mod_init(void)
{
	if(!sphere_oracle_db.s) {
		LM_ERR("Set the oracle_db parameter to"
				" where the oracle DB resides\n");
		return 0;
	}
	sphere_oracle_db.len = strlen(sphere_oracle_db.s);

	return 1;
}

static void mod_destroy(void)
{
  return 1;
}
