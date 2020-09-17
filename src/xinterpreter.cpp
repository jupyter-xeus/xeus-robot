/***************************************************************************
* Copyright (c) 2020, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2020, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <iostream>
#include <string>

#include "nlohmann/json.hpp"

#include "pybind11/functional.h"
#include "pybind11/eval.h"

#include "xeus-python/xinterpreter.hpp"

#include "xeus_robot_config.hpp"
#include "xinterpreter.hpp"
#include "xutils.hpp"
#include "xtraceback.hpp"

namespace nl = nlohmann;

namespace py = pybind11;
using namespace pybind11::literals;

namespace xrob
{

    interpreter::interpreter()
        : xpyt::interpreter()
    {
    }

    interpreter::~interpreter()
    {
    }

    void interpreter::configure_impl()
    {
        xpyt::interpreter::configure_impl();

        py::gil_scoped_acquire acquire;

        // Initialize the test suite
        py::module robot = py::module::import("robotframework_interpreter");

        m_test_suite = robot.attr("init_suite")("name"_a="xeus-robot");
        m_listener = py::list();
        m_debug_adapter = py::none();    
}

    nl::json interpreter::execute_request_impl(int execution_count,
                                               const std::string& code,
                                               bool silent,
                                               bool /*store_history*/,
                                               nl::json /*user_expressions*/,
                                               bool allow_stdin)
    {
        nl::json kernel_res;

        // Acquire GIL before executing code
        py::gil_scoped_acquire acquire;

        py::module robot = py::module::import("robotframework_interpreter");

        // Maps source file for debugger
        {
            std::string filename = get_cell_tmp_file(code);
            py::dict tmp_scope = py::dict("test_suite"_a=m_test_suite, "filename"_a=filename);
            py::exec(py::str(R"(
test_suite.source=filename
            )"), tmp_scope);
        }

        // Get execution result
        py::object result = robot.attr("execute")(code, m_test_suite);

        py::object stats = result.attr("statistics").attr("total").attr("critical");
        std::string text = std::string("Failed tests: ") + py::str(stats.attr("failed")).cast<std::string>() +
            std::string("; Passed tests: ") + py::str(stats.attr("passed")).cast<std::string>() + std::string(";");

        nl::json pub_data;
        pub_data["text/plain"] = text;
        xpyt::interpreter::publish_execution_result(execution_count, pub_data, nl::json::object());

        kernel_res["status"] = "ok";
        kernel_res["user_expressions"] = nl::json::object();
        kernel_res["payload"] = nl::json::array();
        return kernel_res;
    }

    nl::json interpreter::complete_request_impl(
        const std::string& code,
        int cursor_pos)
    {
        nl::json kernel_res;
        return kernel_res;
    }

    nl::json interpreter::inspect_request_impl(const std::string& code,
                                               int cursor_pos,
                                               int /*detail_level*/)
    {
        nl::json kernel_res;
        return kernel_res;
    }

    nl::json interpreter::is_complete_request_impl(const std::string& code)
    {
        nl::json kernel_res;
        return kernel_res;
    }

    nl::json interpreter::kernel_info_request_impl()
    {
        nl::json result;
        result["implementation"] = "xeus-robot";
        result["implementation_version"] = XROB_VERSION;

        /* The jupyter-console banner for xeus-robot is the following:
          __  _____ _   _ ___
          \ \/ / _ \ | | / __|
           >  <  __/ |_| \__ \
          /_/\_\___|\__,_|___/

          xeus-robot: a Jupyter lernel for robotframework
        */

        std::string banner = ""
              "  __  _____ _   _ ___\n"
              "  \\ \\/ / _ \\ | | / __|\n"
              "   >  <  __/ |_| \\__ \\\n"
              "  /_/\\_\\___|\\__,_|___/\n"
              "\n"
              "  xeus-robot: a Jupyter kernel for robotframework";
        result["banner"] = banner;
        // result["debugger"] = true;

        result["language_info"]["name"] = "robotframework";
        result["language_info"]["version"] = "3.2";
        result["language_info"]["mimetype"] = "text/x-robotframework";
        result["language_info"]["file_extension"] = ".robot";
        result["language_info"]["codemirror_mode"] = "robotframework";
        result["language_info"]["pygments_lexer"] = "robotframework";

        result["status"] = "ok";
        return result;
    }

    void interpreter::shutdown_request_impl()
    {
    }

    nl::json interpreter::internal_request_impl(const nl::json& content)
    {
        py::gil_scoped_acquire acquire;
        std::string port = content.value("port", "");
        std::string code = content.value("code", "");
        nl::json reply;
        try
        {
            std::cout << code << std::endl;
            py::dict scope = py::dict();
            py::exec(py::str(code), py::globals()/*, scope*/);

            py::exec(py::str(R"(
listener=[]
listener.append(get_listener())
            )")
            , py::globals()/*, scope*/);

            m_listener = py::globals()["listener"];
            m_debug_adapter = py::globals()["processor"];

            reply["status"] = "ok";
        }
        catch (py::error_already_set& e)
        {
            std::cout << "exception" << std::endl;
            xerror error = extract_error(e);

            std::cout << error.m_ename << std::endl;
            std::cout << error.m_evalue << std::endl;

            publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);
            error.m_traceback.resize(1);
            error.m_traceback[0] = code;

            reply["status"] = "error";
            reply["ename"] = error.m_ename;
            reply["evalue"] = error.m_evalue;
            reply["traceback"] = error.m_traceback;
            std::exit(-1);
        }
        return reply;
    }

}
