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
#include <iostream>
#include <string>
#include <utility>

#ifdef __GNUC__
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus/xserver.hpp"

#include "pybind11/embed.h"
#include "pybind11/pybind11.h"

#include "xeus-python/xpaths.hpp"
#include "xeus_robot_config.hpp"

#include "xinterpreter.hpp"
#include "xdebugger.hpp"

#ifdef __GNUC__
void handler(int sig)
{
    void* array[10];

    // get void*'s for all entries on the stack
    size_t size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
#endif

namespace py = pybind11;

bool should_print_version(int argc, char* argv[])
{
    for (int i = 0; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--version")
        {
            return true;
        }
    }
    return false;
}

std::string extract_filename(int argc, char* argv[])
{
    std::string res = "";
    for (int i = 0; i < argc; ++i)
    {
        if ((std::string(argv[i]) == "-f") && (i + 1 < argc))
        {
            res = argv[i + 1];
            for (int j = i; j < argc - 2; ++j)
            {
                argv[j] = argv[j + 2];
            }
            argc -= 2;
            break;
        }
    }
    return res;
}

void print_pythonhome()
{
    std::setlocale(LC_ALL, "en_US.utf8");
    wchar_t* ph = Py_GetPythonHome();

    char mbstr[1024];
    std::wcstombs(mbstr, ph, 1024);

    std::clog << "PYTHONHOME set to " << mbstr << std::endl;
}

int main(int argc, char* argv[])
{
    if (should_print_version(argc, argv))
    {
        std::clog << "xrobot " << XROB_VERSION << std::endl;
        return 0;
    }

    // If we are called from the Jupyter launcher, silence all logging. This
    // is important for a JupyterHub configured with cleanup_servers = False:
    // Upon restart, spawned single-user servers keep running but without the
    // std* streams. When a user then tries to start a new kernel, xpython
    // will get a SIGPIPE and exit.
    if (std::getenv("JPY_PARENT_PID") != NULL)
    {
        std::clog.setstate(std::ios_base::failbit);
    }

    // Registering SIGSEGV handler
#ifdef __GNUC__
    std::clog << "registering handler for SIGSEGV" << std::endl;
    signal(SIGSEGV, handler);
#endif

    // Setting Program Name
    static const std::string executable(xpyt::get_python_path());
    static const std::wstring wexecutable(executable.cbegin(), executable.cend());

    // On windows, sys.executable is not properly set with Py_SetProgramName
    // Cf. https://bugs.python.org/issue34725
    // A private undocumented API was added as a workaround in Python 3.7.2.
    // _Py_SetProgramFullPath(const_cast<wchar_t*>(wexecutable.c_str()));
    Py_SetProgramName(const_cast<wchar_t*>(wexecutable.c_str()));

    // Setting PYTHONHOME
    xpyt::set_pythonhome();
    print_pythonhome();

    // Instanciating the Python interpreter
    py::scoped_interpreter guard;

    // Setting argv
    wchar_t** argw = new wchar_t*[size_t(argc)];
    for(auto i = 0; i < argc; ++i)
    {
        argw[i] = Py_DecodeLocale(argv[i], nullptr);
    }
    PySys_SetArgvEx(argc, argw, 0);
    for(auto i = 0; i < argc; ++i)
    {
        PyMem_RawFree(argw[i]);
    }
    delete[] argw;

    // Instantiating the xeus xinterpreter
    using interpreter_ptr = std::unique_ptr<xrob::interpreter>;
    interpreter_ptr interpreter = interpreter_ptr(new xrob::interpreter());

    using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
    history_manager_ptr hist = xeus::make_in_memory_history_manager();

    nl::json debugger_config = nl::json::object();
    std::string connection_filename = extract_filename(argc, argv);

    if (!connection_filename.empty())
    {
        xeus::xconfiguration config = xeus::load_configuration(connection_filename);

        xeus::xkernel kernel(config,
                             xeus::get_user_name(),
                             std::move(interpreter),
                             std::move(hist),
                             xeus::make_console_logger(xeus::xlogger::msg_type,
                                                       xeus::make_file_logger(xeus::xlogger::content, "xeus.log")),
                             xeus::make_xserver_shell_main,
                             xrob::make_robot_debugger,
                             debugger_config);

        std::clog <<
            "Starting xeus-robot kernel...\n\n"
            "If you want to connect to this kernel from an other client, you can use"
            " the " + connection_filename + " file."
            << std::endl;

        kernel.start();
    }
    else
    {
        xeus::xkernel kernel(xeus::get_user_name(),
                             std::move(interpreter),
                             std::move(hist),
                             nullptr,
                             xeus::make_xserver_shell_main,
                             xrob::make_robot_debugger,
                             debugger_config);

        const auto& config = kernel.get_config();
        std::clog <<
            "Starting xeus-robot kernel...\n\n"
            "If you want to connect to this kernel from an other client, just copy"
            " and paste the following content inside of a `kernel.json` file. And then run for example:\n\n"
            "# jupyter console --existing kernel.json\n\n"
            "kernel.json\n```\n{\n"
            "    \"transport\": \"" + config.m_transport + "\",\n"
            "    \"ip\": \"" + config.m_ip + "\",\n"
            "    \"control_port\": " + config.m_control_port + ",\n"
            "    \"shell_port\": " + config.m_shell_port + ",\n"
            "    \"stdin_port\": " + config.m_stdin_port + ",\n"
            "    \"iopub_port\": " + config.m_iopub_port + ",\n"
            "    \"hb_port\": " + config.m_hb_port + ",\n"
            "    \"signature_scheme\": \"" + config.m_signature_scheme + "\",\n"
            "    \"key\": \"" + config.m_key + "\"\n"
            "}\n```"
            << std::endl;

        kernel.start();
    }

    return 0;
}
