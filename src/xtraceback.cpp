/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <map>
#include <vector>
#include <sstream>
#include <string>

#include "xeus-python/xutils.hpp"
#include "xeus-python/xtraceback.hpp"

#include "xtraceback.hpp"

#include "pybind11/pybind11.h"

namespace py = pybind11;

namespace xrob
{
    std::string red_text(const std::string& text)
    {
        return "\033[0;31m" + text + "\033[0m";
    }

    std::string green_text(const std::string& text)
    {
        return "\033[0;32m" + text + "\033[0m";
    }

    std::string blue_text(const std::string& text)
    {
        return "\033[0;34m" + text + "\033[0m";
    }

    std::string first_error_delimiter(const std::string& error_name)
    {
        std::size_t size(75);
        std::stringstream first_frame;

        first_frame << red_text(std::string(size, '-')) << "\n"
                    << red_text(error_name) << ":\n";

        return first_frame.str();
    }

    std::string last_error_delimiter()
    {
        std::size_t size(75);
        std::stringstream first_frame;

        first_frame << red_text(std::string(size, '-')) << "\n";

        return first_frame.str();
    }

    xpyt::xerror extract_robot_error(py::error_already_set& error)
    {
        xpyt::xerror out;

        // Fetch the error message, it must be released by the C++ exception first
        error.restore();

        py::object py_type;
        py::object py_value;
        py::object py_tb;
        PyErr_Fetch(&py_type.ptr(), &py_value.ptr(), &py_tb.ptr());

        // This should NOT happen
        if (py_type.is_none())
        {
            out.m_ename = "Error";
            out.m_evalue = error.what();
            out.m_traceback.push_back(error.what());
        }
        else
        {
            out.m_ename = py::str(py_type.attr("__name__"));
            out.m_evalue = py::str(py_value);

            out.m_traceback.push_back(first_error_delimiter(out.m_ename));
            out.m_traceback.push_back(out.m_evalue);
            out.m_traceback.push_back(last_error_delimiter());
        }

        return out;
    }
}
