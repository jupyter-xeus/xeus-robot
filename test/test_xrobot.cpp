/***************************************************************************
* Copyright (c) 2020, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <thread>

#include "xeus/xsystem.hpp"

#include "gtest/gtest.h"
#include "xeus_client.hpp"

using namespace std::chrono_literals;

nl::json make_shutdown_request()
{
    nl::json req = {
        {"restart", false}
    };
    return req;
}

nl::json make_kernel_info_request()
{
    nl::json req = nl::json::object();
    return req;
}

namespace
{
    const std::string KERNEL_JSON = "kernel-debug.json";
}

void dump_connection_file()
{
    static bool dumped = false;
    static std::string connection_file = R"(
{
  "shell_port": 60779,
  "iopub_port": 55691,
  "stdin_port": 56973,
  "control_port": 56505,
  "hb_port": 45551,
  "ip": "127.0.0.1",
  "key": "6ef0855c-5cba319b6d05552c44a8ac90",
  "transport": "tcp",
  "signature_scheme": "hmac-sha256",
  "kernel_name": "xcpp"
}
        )";
    if(!dumped)
    {
        std::ofstream out(KERNEL_JSON);
        out << connection_file;
        dumped = true;
    }
}

void start_kernel()
{
    dump_connection_file();
    std::thread kernel([]()
    {
        std::string cmd = "xrobot -f " + KERNEL_JSON + "&";
        int ret2 = std::system(cmd.c_str());
    });
    std::this_thread::sleep_for(2s);
    kernel.detach();
}

std::condition_variable cv;
std::mutex mcv;
bool done = false;

void notify_done()
{
    {
        std::lock_guard<std::mutex> lk(mcv);
        done = true;
    }
    cv.notify_one();
}

void run_timer()
{
    std::unique_lock<std::mutex> lk(mcv);
    if (!cv.wait_for(lk, std::chrono::seconds(20), []() { return done; }))
    {
        std::clog << "Unit test time out !!" << std::endl;
        std::terminate();
    }
}

void start_timer()
{
    done = false;
    std::thread t(run_timer);
    t.detach();
}

TEST(xrobot, kernel_info)
{
    start_kernel();
    start_timer();
    zmq::context_t context;
    {
        std::cout << "Instantiating client" << std::endl;
        xeus_logger_client client(context, "xrobot_client", xeus::load_configuration(KERNEL_JSON), "kernel_info.log");

        std::cout << "Sending kernel_info_request" << std::endl;
        client.send_on_control("kernel_info_request", make_kernel_info_request());
        nl::json res = client.receive_on_control();
        std::cout << "received: " << std::endl << res.dump(4) << std::endl;
        
        client.send_on_control("shutdown_request", make_shutdown_request());
        client.receive_on_control();

        std::this_thread::sleep_for(2s);
        notify_done();
    }
}

