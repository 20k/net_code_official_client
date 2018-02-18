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

std::string handle_up(const std::string& unknown_command)
{
    std::string up = "#up ";

    std::vector<std::string> strings = no_ss_split(unknown_command, " ");

    if(unknown_command.substr(0, up.length()) == up && strings.size() == 2)
    {
        std::string name = strings[1];

        std::string hardcoded_user = "i20k";

        std::string diskname = "./scripts/" + hardcoded_user + "." + name + ".js";

        return up + name + " " + read_file(diskname);
    }

    return unknown_command;
}

void handle_async_write(shared_data* shared, tcp::socket* socket)
{
    while(1)
    {
        std::string target = "/test.txt";
        int version = 11;
        std::string host = "127.0.0.1";

        if(shared->has_front_write())
        {
            std::string next_command = shared->get_front_write();

            next_command = handle_up(next_command);

            http::request<http::string_body> req{http::verb::get, target, version};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            req.set(http::field::content_type, "text/plain");
            req.body() = next_command;

            req.prepare_payload();

            http::write(*socket, req);
        }

        Sleep(1);
    }
}

void handle_async_read(shared_data* shared, tcp::socket* socket)
{
    while(1)
    {
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::string_body> res;

        if(socket->available() > 0)
        {
            // Receive the HTTP response
            http::read(*socket, buffer, res);

            std::string str = res.body();

            // Write the message to standard out
            std::cout << str << std::endl;

            shared->add_back_read(str);
        }

        Sleep(1);
    }
}

void test_http_client(shared_data& shared)
{
    std::string host = "127.0.0.1";
    std::string port = "6750";
    //std::string target = "/test.txt";
    //std::string command = "user i20k";

    //int version = 11;

    boost::asio::io_context* ioc = new boost::asio::io_context;

    tcp::resolver* resolver = new tcp::resolver(*ioc);
    tcp::socket* socket = new tcp::socket(*ioc);

    auto const results = resolver->resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(*socket, results.begin(), results.end());


    std::thread(handle_async_read, &shared, socket).detach();
    std::thread(handle_async_write, &shared, socket).detach();
}
