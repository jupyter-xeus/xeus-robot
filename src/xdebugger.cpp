/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>

// This must be included BEFORE pybind
// otherwise it fails to build on Windows
// because of the redefinition of snprintf
#include "nlohmann/json.hpp"

#include "pybind11_json/pybind11_json.hpp"

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include "xeus/xinterpreter.hpp"
#include "xeus/xmiddleware.hpp"
#include "xeus/xsystem.hpp"

#include "xdebugger.hpp"
#include "xrobodebug_client.hpp"
#include "xinternal_utils.hpp"

namespace nl = nlohmann;
namespace py = pybind11;

using namespace pybind11::literals;
using namespace std::placeholders;

namespace xrob
{
    debugger::debugger(zmq::context_t& context,
                       const xeus::xconfiguration& config,
                       const std::string& user_name,
                       const std::string& session_id,
                       const nl::json& debugger_config)
        : xdebugger_base(context)
        , p_robodebug_client(new xrobodebug_client(context,
                                                   config,
                                                   xeus::get_socket_linger(),
                                                   xdap_tcp_configuration(xeus::dap_tcp_type::server,
                                                                          xeus::dap_init_type::sequential,
                                                                          user_name,
                                                                          session_id),
                                                   get_event_callback()))

        , m_robodebug_host("127.0.0.1")
        , m_robodebug_port("")
        , m_debugger_config(debugger_config)
    {
        register_request_handler("inspectVariables", std::bind(&debugger::inspect_variables_request, this, _1), false);
        m_robodebug_port = xeus::find_free_port(100, 5678, 5900);
    }

    debugger::~debugger()
    {
        delete p_robodebug_client;
        p_robodebug_client = nullptr;
    }

    nl::json debugger::inspect_variables_request(const nl::json& message)
    {
        py::gil_scoped_acquire acquire;
        py::object variables = py::globals();

        nl::json json_vars = nl::json::array();
        for (const py::handle& key : variables)
        {
            nl::json json_var = nl::json::object();
            json_var["name"] = py::str(key);
            json_var["variablesReference"] = 0;
            try
            {
                json_var["value"] = variables[key];
            }
            catch(std::exception&)
            {
                json_var["value"] = py::repr(variables[key]);
            }
            json_vars.push_back(json_var);
        }

        nl::json reply = {
            {"type", "response"},
            {"request_seq", message["seq"]},
            {"success", true},
            {"command", message["command"]},
            {"body", {
                {"variables", json_vars}
            }}
        };

        return reply;
    }

    bool debugger::start(zmq::socket_t& header_socket, zmq::socket_t& request_socket)
    {
        std::string temp_dir = xeus::get_temp_directory_path();
        std::string log_dir = temp_dir + "/" + "xpython_debug_logs_" + std::to_string(xeus::get_current_pid());

        std::string controller_end_point = xeus::get_controller_end_point("debugger");
        std::string controller_header_end_point = xeus::get_controller_end_point("debugger_header");
        std::string publisher_end_point = xeus::get_publisher_end_point();

        request_socket.bind(controller_end_point);
        header_socket.bind(controller_header_end_point);

        std::string robodebug_end_point = "tcp://" + m_robodebug_host + ':' + m_robodebug_port;
        std::thread client(&xrobodebug_client::start_debugger,
                           p_robodebug_client,
                           robodebug_end_point,
                           publisher_end_point,
                           controller_end_point,
                           controller_header_end_point);
        client.detach();


        std::string tmp_folder =  get_tmp_prefix();
        xeus::create_directory(tmp_folder);

        xeus::create_directory(log_dir);

        std::string var_py = "robot_port = " + m_robodebug_port
                           + "\nlog_file=\'" + log_dir + "/xrobot.log\'\n";
        std::string init_logger_py = R"(
import robotframework_ls
robotframework_ls.import_robocorp_ls_core()
from robocorp_ls_core.robotframework_log import configure_logger, _log_config
configure_logger("robot", 3, log_file)
        )";
        std::string init_debugger_py = R"(
from robotframework_debug_adapter.run_robot__main__ import connect, _RobotTargetComm
s = connect(int(robot_port))
processor = _RobotTargetComm(s, debug=True)
processor.start_communication_threads()
        )";
        std::string init_listener_py = R"(
from robotframework_debug_adapter.listeners import DebugListener, DebugListenerV2
debug_listener = DebugListener()
debug_listenerv2 = DebugListenerV2()
        )";

        std::string code = var_py + init_logger_py + init_debugger_py + init_listener_py;

        nl::json json_code;
        json_code["port"] = m_robodebug_port;
        json_code["code"] = code;
        nl::json rep = xdebugger::get_control_messenger().send_to_shell(json_code);
        std::string status = rep["status"].get<std::string>();
        if(status != "ok")
        {
            std::string ename = rep["ename"].get<std::string>();
            std::string evalue = rep["evalue"].get<std::string>();
            std::vector<std::string> traceback = rep["traceback"].get<std::vector<std::string>>();
            std::clog << "Exception raised when trying to import ptvsd" << std::endl;
            for(std::size_t i = 0; i < traceback.size(); ++i)
            {
                std::clog << traceback[i] << std::endl;
            }
            std::clog << ename << " - " << evalue << std::endl;
        }

        request_socket.send(zmq::message_t("REQ", 3), zmq::send_flags::none);
        zmq::message_t ack;
        (void)request_socket.recv(ack);

        return true;
    }

    void debugger::stop(zmq::socket_t& header_socket, zmq::socket_t& request_socket)
    {
        std::string controller_end_point = xeus::get_controller_end_point("debugger");
        std::string controller_header_end_point = xeus::get_controller_end_point("debugger_header");
        request_socket.unbind(controller_end_point);
        header_socket.unbind(controller_header_end_point);
    }

    xeus::xdebugger_info debugger::get_debugger_info() const
    {
        return xeus::xdebugger_info(xeus::get_tmp_hash_seed(),
                                    get_tmp_prefix(),
                                    get_tmp_suffix());
    }

    std::string debugger::get_cell_temporary_file(const std::string& code) const
    {
        return get_cell_tmp_file(code);
    }

    std::unique_ptr<xeus::xdebugger> make_robot_debugger(xeus::xcontext& context,
                                                         const xeus::xconfiguration& config,
                                                         const std::string& user_name,
                                                         const std::string& session_id,
                                                         const nl::json& debugger_config)
    {
        return std::unique_ptr<xeus::xdebugger>(new debugger(context.get_wrapped_context<zmq::context_t>(),
                                                             config, user_name, session_id, debugger_config));
    }
}
