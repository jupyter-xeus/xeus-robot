/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XROB_TRACEBACK_HPP
#define XROB_TRACEBACK_HPP

#include <vector>
#include <string>

#include "pybind11/pybind11.h"

#include "xeus-python/xtraceback.hpp"

namespace py = pybind11;

namespace xrob
{
    std::string red_text(const std::string& text);
    std::string green_text(const std::string& text);
    std::string blue_text(const std::string& text);

    std::string first_error_delimiter(const std::string& error_name);
    std::string last_error_delimiter();

    xpyt::xerror extract_robot_error(py::error_already_set& error);
}

#endif
