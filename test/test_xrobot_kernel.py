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

    completion_samples = [
        # Context completion
        {'text': '***', 'matches': {'*** Tasks ***', '*** Keywords ***', '*** Settings ***', '*** Variables ***', '*** Test Cases ***'}},
        # Library completion
        {'text': '*** Settings ***\nLibrary S', 'matches': {'Screenshot', 'String'}},
        # Variable completion
        {'text': '*** Variables ***\n${VARNAME}  test\n${V', 'matches': {'${VARNAME}'}},
    ]


if __name__ == '__main__':
    unittest.main()
