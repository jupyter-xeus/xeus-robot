#############################################################################
# Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and      #
# Wolf Vollprecht                                                           #
# Copyright (c) 2018, QuantStack                                            #
#                                                                           #
# Distributed under the terms of the BSD 3-Clause License.                  #
#                                                                           #
# The full license is in the file LICENSE, distributed with this software.  #
#############################################################################

import tempfile
import unittest
import jupyter_kernel_test


class XeusRobotTests(jupyter_kernel_test.KernelTests):

    kernel_name = "xrobot"
    language_name = "robotframework"


if __name__ == '__main__':
    unittest.main()
