/*
 * Copyright (c) 2016, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "infra/log.h"

#include "cfw/cfw.h"
#include "cfw/cproxy.h"
#include "cfw_internal.h"

#include "services/service_queue.h"

#include "machine.h"

#ifdef CONFIG_CFW_MASTER
void cfw_init(T_QUEUE queue)
{
	/* CFW init on Quark: initialize the master service manager */
	cfw_service_mgr_init(queue);

	/* CFW init on ARC: initialize the slave service manager */
	shared_data->arc_cfw_ready = 0;
	shared_data->services = services;
	shared_data->service_mgr_port_id = cfw_get_service_mgr_port_id();
	/* Unblock the ARC CFW init, and wait that it's done */
	shared_data->quark_cfw_ready = 1;
	while (shared_data->arc_cfw_ready == 0) ;

	/* Cproxy is another (deprecated) way to communicate with the component
	 * framework, used by some test commands and some services. Initialize
	 * it here. */
	cproxy_init(queue);

	/* We use the main task queue to support most services */
	set_service_queue(queue);

	cfw_init_registered_services(queue);
}
#endif

#ifdef CONFIG_CFW_PROXY
void cfw_init(T_QUEUE queue)
{
	/* Init CFW slave, once Quark CFW master is initialized */
	while (shared_data->quark_cfw_ready != 1) ;
	pr_debug(LOG_MODULE_CFW, "Ports: %p services: %p %d",
		 shared_data->ports,
		 shared_data->services,
		 shared_data->service_mgr_port_id);

	cfw_service_mgr_init_proxy(queue, shared_data->service_mgr_port_id);
	shared_data->arc_cfw_ready = 1;

	cfw_init_registered_services(queue);
}
#endif
