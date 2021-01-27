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

#include "pybind11_json/pybind11_json.hpp"

#include "pybind11/functional.h"
#include "pybind11/eval.h"

#include "xeus/xguid.hpp"

#include "xeus-python/xinterpreter.hpp"
#include "xeus-python/xtraceback.hpp"
#include "xeus-python/xutils.hpp"

#include "xeus_robot_config.hpp"
#include "xinternal_utils.hpp"
#include "xinterpreter.hpp"

namespace nl = nlohmann;

namespace py = pybind11;
using namespace pybind11::literals;

#define PYTHON_MODULE_REGEX "^%%python module ([a-zA-Z_]+)"


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
        py::module robot_interpreter = py::module::import("robotframework_interpreter");

        m_test_suite = robot_interpreter.attr("init_suite")("name"_a="xeus-robot");

        // Initialize listeners
        m_listeners = py::list();
        m_drivers = py::list();
        m_debug_listener = py::none();

        m_keywords_listener = robot_interpreter.attr("RobotKeywordsIndexerListener")();
        m_listeners.append(m_keywords_listener);

        m_return_value_listener = robot_interpreter.attr("ReturnValueListener")();
        m_listeners.append(m_return_value_listener);

        m_status_listener = robot_interpreter.attr("StatusEventListener")();
        m_listeners.append(m_status_listener);

        m_listeners.append(robot_interpreter.attr("RpaBrowserConnectionsListener")(m_drivers));
        m_listeners.append(robot_interpreter.attr("SeleniumConnectionsListener")(m_drivers));
        m_listeners.append(robot_interpreter.attr("JupyterConnectionsListener")(m_drivers));
        m_listeners.append(robot_interpreter.attr("AppiumConnectionsListener")(m_drivers));
        m_listeners.append(robot_interpreter.attr("WhiteLibraryListener")(m_drivers));

        m_debug_adapter = py::none();
    }

    nl::json interpreter::execute_request_impl(
        int execution_count,
        const std::string& code,
        bool silent,
        bool /*store_history*/,
        nl::json /*user_expressions*/,
        bool /*allow_stdin*/)
    {
        // Acquire GIL before executing code
        py::gil_scoped_acquire acquire;

        std::string filename = get_cell_tmp_file(code);

        // If it's Python code
        py::module re = py::module::import("re");
        py::object match = re.attr("match")(PYTHON_MODULE_REGEX, code);
        if (!match.is_none())
        {
            py::object modulename = py::list(match.attr("groups")())[0];

            // Extract Python code from the cell
            std::string python_code = code;
            python_code.erase(0, py::list(match.attr("span")())[1].cast<int>());

            return execute_python(python_code, modulename, filename, silent);
        }

        // Maps source file for debugger/traceback
        xpyt::register_filename_mapping(filename, execution_count);
        m_test_suite.attr("source") = py::str(filename);

        nl::json kernel_res;

        py::object outputdir = py::module::import("tempfile").attr("TemporaryDirectory")();
        py::module robot_interpreter = py::module::import("robotframework_interpreter");
        py::object partial = py::module::import("functools").attr("partial");

        // Create progress updater and pass it to the state listener
        py::str display_id = py::str(xeus::new_xguid());

        py::object progress_updater = robot_interpreter.attr("ProgressUpdater")(
            partial(py::globals()["display"], "raw"_a=true, "display_id"_a=display_id),
            partial(py::globals()["update_display"], "raw"_a=true, "display_id"_a=display_id)
        );
        m_status_listener.attr("callback") = progress_updater.attr("update");

        // Get execution result
        py::list result;
        try
        {
            result = robot_interpreter.attr("execute")(
                code, m_test_suite, "listeners"_a=m_listeners, "drivers"_a=m_drivers,
                "outputdir"_a=outputdir.attr("name")
            );
        }
        catch (py::error_already_set& e)
        {
            // Cleanup
            outputdir.attr("cleanup")();
            progress_updater.attr("clear")();

            xpyt::xerror error = xpyt::extract_error(e);

            std::vector<std::string> traceback({error.m_ename + ": " + error.m_evalue});
            if (!silent)
            {
                publish_execution_error(error.m_ename, error.m_evalue, traceback);
            }

            kernel_res["status"] = "error";
            kernel_res["ename"] = error.m_ename;
            kernel_res["evalue"] = error.m_evalue;
            kernel_res["traceback"] = traceback;

            return kernel_res;
        }

        // If the result is None, it means the suite has not been executed, instead
        // widgets have been created
        if (result[0].is_none())
        {
            for (const py::handle& widget: result[1])
            {
                py::globals()["display"](widget);
            }
        }
        // Otherwise publish tests report if there is one
        else if (!result[1].is_none())
        {
            publish_execution_result(execution_count, result[1], nl::json::object());
        }

        // Publish the latest test evaluation
        py::object last_test_evaluation = m_return_value_listener.attr("get_last_value")();
        if (!last_test_evaluation.is_none())
        {
            py::globals()["display"](last_test_evaluation, "raw"_a=true);
        }

        // Cleanup
        outputdir.attr("cleanup")();
        progress_updater.attr("clear")();

        kernel_res["status"] = "ok";
        kernel_res["user_expressions"] = nl::json::object();
        kernel_res["payload"] = nl::json::array();

        return kernel_res;
    }

    nl::json interpreter::execute_python(
        const std::string& code,
        py::object modulename,
        const std::string& filename,
        bool silent)
    {
        nl::json kernel_res;

        py::module sys = py::module::import("sys");
        py::module types = py::module::import("types");
        py::module linecache = py::module::import("linecache");
        py::module builtins = py::module::import("builtins");

        // Create Python module
        py::object module = types.attr("ModuleType")(modulename);

        sys.attr("modules")[modulename] = module;

        // Caching the input code
        linecache.attr("xupdatecache")(code, filename);

        // Execute it
        try
        {
            py::object compiled_code = builtins.attr("compile")(code, filename, "exec");

            // Inject display in the scope
            module.attr("__dict__")["display"] = py::globals()["display"];

            xpyt::exec(compiled_code, module.attr("__dict__"));

            kernel_res["status"] = "ok";
            kernel_res["user_expressions"] = nl::json::object();
            kernel_res["payload"] = nl::json::array();

            // Keep the Python module name around for library completion
            m_python_modules.attr("append")(modulename);
        }
        catch (py::error_already_set& e)
        {
            xpyt::xerror error = xpyt::extract_error(e);

            if (!silent)
            {
                publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);
            }

            kernel_res["status"] = "error";
            kernel_res["ename"] = error.m_ename;
            kernel_res["evalue"] = error.m_evalue;
            kernel_res["traceback"] = error.m_traceback;
        }

        return kernel_res;
    }

    nl::json interpreter::complete_request_impl(
        const std::string& code,
        int cursor_pos)
    {
        // Acquire GIL before executing code
        py::gil_scoped_acquire acquire;

        // If it's Python code
        py::module re = py::module::import("re");
        py::object match = re.attr("match")(PYTHON_MODULE_REGEX, code);
        if (!match.is_none())
        {
            py::object module_name = py::list(match.attr("groups")())[0];

            // Extract Python code from the cell
            std::string python_code = code;
            int header_len = py::list(match.attr("span")())[1].cast<int>();
            python_code.erase(0, header_len);

            nl::json xpython_res = xpyt::interpreter::complete_request_impl(python_code, cursor_pos - header_len);

            // Fix cursor pos in xpython's answer
            xpython_res["cursor_start"] = xpython_res["cursor_start"].get<int>() + header_len;
            xpython_res["cursor_end"] = xpython_res["cursor_end"].get<int>() + header_len;

            return xpython_res;
        }

        py::module robot_interpreter = py::module::import("robotframework_interpreter");

        nl::json xrobot_res = robot_interpreter.attr("complete")(code, cursor_pos, m_test_suite, m_keywords_listener, m_python_modules, m_drivers);
        xrobot_res["status"] = "ok";
        return xrobot_res;
    }

    nl::json interpreter::inspect_request_impl(const std::string& code,
                                               int cursor_pos,
                                               int detail_level)
    {
        // Acquire GIL before executing code
        py::gil_scoped_acquire acquire;

        py::module robot_interpreter = py::module::import("robotframework_interpreter");

        nl::json xrobot_res = robot_interpreter.attr("inspect")(code, cursor_pos, m_test_suite, m_keywords_listener, detail_level);
        xrobot_res["status"] = "ok";
        return xrobot_res;
    }

    nl::json interpreter::is_complete_request_impl(const std::string& /*code*/)
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

        result["help_links"] = nl::json::array();
        result["help_links"][0] = nl::json::object({
            {"text", "RobotFramework Reference"},
            {"url", "https://robotframework.org"}
        });

        result["status"] = "ok";
        return result;
    }

    void interpreter::shutdown_request_impl()
    {
        // Acquire GIL before executing code
        py::gil_scoped_acquire acquire;

        py::module robot_interpreter = py::module::import("robotframework_interpreter");

        // Shutdown drivers
        robot_interpreter.attr("shutdown_drivers")(m_drivers);
    }

    nl::json interpreter::internal_request_impl(const nl::json& content)
    {
        py::gil_scoped_acquire acquire;
        std::string port = content.value("port", "");
        std::string code = content.value("code", "");
        nl::json reply;
        try
        {
            try
            {
                m_listeners.attr("remove")(m_debug_listener);
            }
            catch(py::error_already_set& e)
            {
                // Ignoring the exception if the element is not in the list
            }

            py::dict scope;

            xpyt::exec(py::str(code), scope);

            m_debug_listener = scope["debug_listener"];
            m_debug_adapter = scope["processor"];

            m_listeners.append(m_debug_listener);

            reply["status"] = "ok";
        }
        catch (py::error_already_set& e)
        {
            xpyt::xerror error = xpyt::extract_error(e);

            publish_execution_error(error.m_ename, error.m_evalue, error.m_traceback);

            reply["status"] = "error";
            reply["ename"] = error.m_ename;
            reply["evalue"] = error.m_evalue;
            reply["traceback"] = error.m_traceback;
        }
        return reply;
    }

}
