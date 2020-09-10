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

        // Import needed modules and initialize the test suite
        py::module os = py::module::import("os");
        py::module model = py::module::import("robot.running.model");
        py::module testsettings = py::module::import("robot.running.builder.testsettings");

        m_test_suite = model.attr("TestSuite")("name"_a="xeus-robot", "source"_a=os.attr("getcwd")());
        m_test_defaults = testsettings.attr("TestDefaults")(py::none());

        // TODO call super configure impl
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

        // Import needed modules for compiling the test cases
        py::module os = py::module::import("os");
        py::module io = py::module::import("io");
        py::module robot_api = py::module::import("robot.api");
        py::module robot_parsers = py::module::import("robot.running.builder.parsers");
        py::module robot_transformers = py::module::import("robot.running.builder.transformers");

        // Compile current cell and populate the test suite
        py::object model = robot_api.attr("get_model")(
            io.attr("StringIO")(code),
            "data_only"_a=py::bool_(false),
            "curdir"_a=os.attr("getcwd")()
        );
        robot_parsers.attr("ErrorReporter")(code).attr("visit")(model);
        robot_transformers.attr("SettingsBuilder")(m_test_suite, m_test_defaults).attr("visit")(model);
        robot_transformers.attr("SuiteBuilder")(m_test_suite, m_test_defaults).attr("visit")(model);

        // TODO Strip keyword and variables duplicates

        py::object stdout = io.attr("StringIO")();

        // py::exec because the "with" keyword cannot be emulated with pybind11
        py::dict scope = py::dict("test_suite"_a=m_test_suite, "stdout"_a=stdout);
        py::exec(py::str(R"(
from tempfile import TemporaryDirectory

with TemporaryDirectory() as path:
    result = test_suite.run(outputdir=path, stdout=stdout)
)"), scope);

        // Remove executed tests
        m_test_suite.attr("tests").attr("_items") = py::list();

        // Get execution result
        py::object stats = scope["result"].attr("statistics").attr("total").attr("critical");
        std::string text = std::string("Failed tests: ") + py::str(stats.attr("failed")).cast<std::string>() +
            std::string("; Passed tests: ") + py::str(stats.attr("passed")).cast<std::string>() + std::string(";");

        nl::json pub_data;
        nl::json pub_metadata;
        pub_data["text/plain"] = text;
        xpyt::interpreter::publish_execution_result(execution_count, pub_data, pub_metadata);

        kernel_res["status"] = "ok";
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
    }

}
