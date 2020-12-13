/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include "xeus/xsystem.hpp"
#include "xinternal_utils.hpp"

namespace xrob
{
    std::string get_tmp_prefix()
    {
        return xeus::get_tmp_prefix("xrobot");
    }

    std::string get_tmp_suffix()
    {
        return ".robot";
    }

    std::string get_cell_tmp_file(const std::string& content)
    {
        return xeus::get_cell_tmp_file(get_tmp_prefix(),
                                       content,
                                       get_tmp_suffix());
    }
}

