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

#include "../crapmud/script_util_shared.hpp"

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

void test_http_client()
{
    std::string host = "127.0.0.1";
    std::string port = "6750";
    std::string target = "/test.txt";
    std::string command = "user i20k";

    int version = 11;

    boost::asio::io_context ioc;

    tcp::resolver resolver{ioc};
    tcp::socket socket{ioc};

    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(socket, results.begin(), results.end());


    while(1)
    {
        std::string next_command;

        std::getline(std::cin, next_command);

        next_command = handle_up(next_command);

        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        req.set(http::field::content_type, "text/plain");
        req.body() = next_command;

        req.prepare_payload();

        http::write(socket, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(socket, buffer, res);

        // Write the message to standard out
        std::cout << res << std::endl;
    }
}
