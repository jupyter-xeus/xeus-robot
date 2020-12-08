/***************************************************************************
* Copyright (c) 2020, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2020, QuantStack
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XROB_INTERPRETER_HPP
#define XROB_INTERPRETER_HPP

#include <string>

#include "nlohmann/json.hpp"

#include "xeus-python/xinterpreter.hpp"

namespace nl = nlohmann;

namespace xrob
{
    class interpreter : public xpyt::interpreter
    {
    public:

        interpreter();
        virtual ~interpreter();

    protected:

        void configure_impl() override;

        nl::json execute_request_impl(int execution_counter,
                                      const std::string& code,
                                      bool silent,
                                      bool store_history,
                                      nl::json user_expressions,
                                      bool allow_stdin) override;

        nl::json execute_python(const std::string& code, py::object modulename, const std::string& filename, bool silent);

        nl::json complete_request_impl(const std::string& code, int cursor_pos) override;

        nl::json inspect_request_impl(const std::string& code,
                                      int cursor_pos,
                                      int detail_level) override;

        nl::json is_complete_request_impl(const std::string& code) override;

        nl::json kernel_info_request_impl() override;

        void shutdown_request_impl() override;

        nl::json internal_request_impl(const nl::json& content) override;

        py::object m_test_suite;
        py::object m_keywords_listener;
        py::list m_listener;
        py::object m_debug_adapter;
    };
}

#endif
