#include "http_beast_client.hpp"

//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP client, asynchronous
//
//------------------------------------------------------------------------------


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <crapmud/script_util_shared.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

std::string handle_up(shared_data* shared, const std::string& unknown_command)
{
    std::string up = "#up ";

    std::vector<std::string> strings = no_ss_split(unknown_command, " ");

    if(unknown_command.substr(0, up.length()) == up && strings.size() == 2)
    {
        std::string name = strings[1];

        std::string hardcoded_user = shared->get_user();

        std::string diskname = "./scripts/" + hardcoded_user + "." + name + ".js";

        return up + name + " " + read_file(diskname);
    }

    return unknown_command;
}

static std::mutex local_mut;
volatile static bool socket_alive = false;

struct shared_context
{
    boost::asio::io_context ioc;

    tcp::resolver resolver;
    tcp::socket socket;

    shared_context() : resolver(ioc), socket(ioc)
    {

    }

    void connect(const std::string& host, const std::string& port)
    {
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(socket, results.begin(), results.end());
    }
};

void handle_async_write(shared_data* shared, tcp::socket* socket)
{
    while(1)
    {
        //std::lock_guard<std::mutex> lk(local_mut);
        Sleep(4);

        try
        {
            if(!socket_alive)
                continue;

            std::string target = "/test.txt";
            int version = 11;
            std::string host = HOST_IP;

            if(shared->has_front_write())
            {
                std::string next_command = shared->get_front_write();

                next_command = handle_up(shared, next_command);

                http::request<http::string_body> req{http::verb::get, target, version};
                req.set(http::field::host, host);
                req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

                req.set(http::field::content_type, "text/plain");
                req.body() = next_command;

                req.prepare_payload();

                http::write(*socket, req);
            }

        }
        catch(...)
        {
            socket_alive = false;
            std::cout << "caught write exception" << std::endl;
            Sleep(1000);
        }
    }
}

void check_auth(shared_data* shared, const std::string& str)
{
    std::string auth_str = "####registered secret ";

    if(str.substr(0, auth_str.length()) == auth_str)
    {
        auto start = str.begin() + auth_str.length();
        std::string key(start, str.end());

        if(!file_exists("key.key"))
        {
            write_all_bin("key.key", key);

            shared->auth = key;
        }
        else
        {
            printf("Key file already exists");
        }
    }
}

void handle_async_read(shared_data* shared, tcp::socket* socket)
{
    while(1)
    {
        //std::lock_guard<std::mutex> lk(local_mut);
        Sleep(4);

        try
        {
            if(!socket_alive)
                continue;

            boost::beast::flat_buffer buffer;

            // Declare a container to hold the response
            http::response<http::string_body> res;

            if(socket->available() > 0)
            {
                // Receive the HTTP response
                http::read(*socket, buffer, res);

                std::string str = res.body();

                check_auth(shared, str);

                shared->add_back_read(str);
            }
        }
        catch(...)
        {
            socket_alive = false;
            std::cout << "caught read exception" << std::endl;
            Sleep(1000);
        }
    }
}

void watchdog(shared_data* shared, shared_context* ctx)
{
    while(1)
    {
        if(socket_alive)
            Sleep(250);

        //std::lock_guard<std::mutex> lk(local_mut);

        while(!socket_alive)
        {
            try
            {
                std::string host = HOST_IP;
                std::string port = HOST_PORT;

                std::cout << "Try Reconnect" << std::endl;

                shared->add_back_read("Connecting...");

                ctx->connect(host, port);

                shared->add_back_read("`LConnected`");
                shared->add_back_write("auth client " + shared->auth);

                socket_alive = true;

                Sleep(1000);
            }
            catch(...)
            {
                shared->add_back_read("`DConnection to the server failed`");

                std::cout << "Server down" << std::endl;
                Sleep(5000);
            }
        }

        Sleep(4);
    }
}

void test_http_client(shared_data& shared)
{
    shared_context* ctx = new shared_context();

    std::thread(handle_async_read, &shared, &ctx->socket).detach();
    std::thread(handle_async_write, &shared, &ctx->socket).detach();
    std::thread(watchdog, &shared, ctx).detach();
}
