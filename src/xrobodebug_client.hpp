/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XROB_ROBODEBUG_CLIENT_HPP
#define XROB_ROBODEBUG_CLIENT_HPP

#include "xeus-zmq/xdap_tcp_client.hpp"

namespace xrob
{
    using xeus::xdap_tcp_client;
    using xeus::xdap_tcp_configuration;

    class xrobodebug_client : public xdap_tcp_client
    {
    public:

        using base_type = xdap_tcp_client;
        using event_callback = base_type::event_callback;

        xrobodebug_client(zmq::context_t& context,
                          const xeus::xconfiguration& config,
                          int socket_linger,
                          const xdap_tcp_configuration& dap_config,
                          const event_callback& cb);


        virtual ~xrobodebug_client() = default;

    private:

        void handle_event(nl::json message) override;
        nl::json get_stack_frames(int thread_id, int seq);
    };
}

#endif

