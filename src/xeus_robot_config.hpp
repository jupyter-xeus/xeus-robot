/***************************************************************************
* Copyright (c) 2020, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2020, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XROB_CONFIG_HPP
#define XROB_CONFIG_HPP

// Project version
#define XROB_VERSION_MAJOR 0
#define XROB_VERSION_MINOR 3
#define XROB_VERSION_PATCH 6

// Composing the version string from major, minor and patch
#define XROB_CONCATENATE(A, B) XROB_CONCATENATE_IMPL(A, B)
#define XROB_CONCATENATE_IMPL(A, B) A##B
#define XROB_STRINGIFY(a) XROB_STRINGIFY_IMPL(a)
#define XROB_STRINGIFY_IMPL(a) #a

#define XROB_VERSION XROB_STRINGIFY(XROB_CONCATENATE(XROB_VERSION_MAJOR,   \
                 XROB_CONCATENATE(.,XROB_CONCATENATE(XROB_VERSION_MINOR,   \
                                  XROB_CONCATENATE(.,XROB_VERSION_PATCH)))))

#endif
