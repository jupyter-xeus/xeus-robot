/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XROB_DEBUGGER_HPP
#define XROB_DEBUGGER_HPP

#include <map>
#include <mutex>
#include <set>

#include "zmq.hpp"

#include "nlohmann/json.hpp"

#include "xeus_robot_config.hpp"
#include "xeus/xeus_context.hpp"

#include "xeus-zmq/xdebugger_base.hpp"

namespace xrob
{

    class xrobodebug_client;

    class debugger : public xeus::xdebugger_base
    {
    public:

        debugger(zmq::context_t& context,
                 const xeus::xconfiguration& config,
                 const std::string& user_name,
                 const std::string& session_id,
                 const nl::json& debugger_config);

        virtual ~debugger();

    private:

        nl::json inspect_variables_request(const nl::json& message);

        bool start(zmq::socket_t& header_socket,
                   zmq::socket_t& request_socket) override;
        void stop(zmq::socket_t& header_socket,
                  zmq::socket_t& request_socket) override;
        xeus::xdebugger_info get_debugger_info() const override;
        std::string get_cell_temporary_file(const std::string& code) const override;

        xrobodebug_client* p_robodebug_client;
        std::string m_robodebug_host;
        std::string m_robodebug_port;
        nl::json m_debugger_config;
    };

    std::unique_ptr<xeus::xdebugger> make_robot_debugger(xeus::xcontext& context,
                                                         const xeus::xconfiguration& config,
                                                         const std::string& user_name,
                                                         const std::string& session_id,
                                                         const nl::json& debugger_config);
}

#endif
