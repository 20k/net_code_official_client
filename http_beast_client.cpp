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

#include <crapmud/socket_shared.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <SFML/System.hpp>

#include <crapmud/script_util_shared.hpp>
#include <crapmud/shared_data.hpp>


//#define HOST_IP "192.168.0.55"
#ifdef EXTERN_IP
#define HOST_IP "77.96.132.101"
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_IP "127.0.0.1"
#endif // LOCAL_IP

#ifdef EXTERN_IP
#define HOST_PORT "6760"
#endif // EXTERN_IP

#ifdef LOCAL_IP
#define HOST_PORT "6761"
#endif // LOCAL_IP

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

std::string handle_up(shared_data* shared, const std::string& unknown_command)
{
    std::string up = "client_command #up ";
    std::string dry = "client_command #dry ";

    std::vector<std::string> strings = no_ss_split(unknown_command, " ");

    if((starts_with(unknown_command, up) || starts_with(unknown_command, dry)) && strings.size() == 3)
    {
        std::string name = strings[2];

        std::string hardcoded_user = shared->get_user();

        std::string diskname = "./scripts/" + hardcoded_user + "." + name + ".js";

        std::string comm = up;

        if(starts_with(unknown_command, dry))
            comm = dry;

        std::string final_command = comm + name + " " + read_file(diskname);

        return final_command;
    }

    return unknown_command;
}

static std::mutex local_mut;
volatile static bool socket_alive = false;

struct shared_context
{
    boost::asio::io_context ioc;

    //tcp::resolver resolver;
    //tcp::socket socket;

    websock_socket* sock = nullptr;

    shared_context()
    {

    }

    void connect(const std::string& host, const std::string& port)
    {
        if(sock)
            delete sock;

        websock_socket_client* tsock = new websock_socket_client(ioc);

        auto const results = tsock->resolver.resolve(host, port);

        boost::asio::connect(tsock->ws.next_layer(), results.begin(), results.end());
        tsock->ws.handshake(host, "/");
        tsock->ws.text(false);

        sock = tsock;
    }
};

void handle_async_write(shared_data* shared, shared_context& ctx)
{
    while(1)
    {
        //std::lock_guard<std::mutex> lk(local_mut);
        sf::sleep(sf::milliseconds(8));

        if(shared->should_terminate)
            break;

        try
        {
            if(!socket_alive)
                continue;

            if(shared->has_front_write())
            {
                std::string next_command = shared->get_front_write();

                next_command = handle_up(shared, next_command);

                if(ctx.sock->write(next_command))
                    break;
            }
        }
        catch(...)
        {
            socket_alive = false;
            std::cout << "caught write exception" << std::endl;
            sf::sleep(sf::milliseconds(1000));
        }
    }

    shared->termination_count++;

    printf("write\n");
}

void check_auth(shared_data* shared, const std::string& str)
{
    std::string auth_str = "command ####registered secret ";

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

void handle_async_read(shared_data* shared, shared_context& ctx)
{
    boost::system::error_code ec;

    while(1)
    {
        sf::sleep(sf::milliseconds(8));

        if(shared->should_terminate)
            break;

        try
        {
            if(!socket_alive)
                continue;

            if(ctx.sock->read(ec))
                break;

            std::string next_command = ctx.sock->get_read();

            check_auth(shared, next_command);
            shared->add_back_read(next_command);
        }
        catch(...)
        {
            socket_alive = false;
            std::cout << "caught read exception" << std::endl;
            sf::sleep(sf::milliseconds(1000));
        }
    }

    shared->termination_count++;

    printf("read\n");
}

void watchdog(shared_data* shared, shared_context& ctx)
{
    while(1)
    {
        if(socket_alive)
            sf::sleep(sf::milliseconds(50));

        if(shared->should_terminate)
            break;

        while(!socket_alive)
        {
            try
            {
                std::string host = HOST_IP;
                std::string port = HOST_PORT;

                std::cout << "Try Reconnect" << std::endl;

                shared->add_back_read("Connecting...");

                ctx.connect(host, port);

                shared->add_back_read("`LConnected`");
                shared->add_back_write("client_command auth client " + shared->auth);

                socket_alive = true;

                sf::sleep(sf::milliseconds(50));
            }
            catch(...)
            {
                shared->add_back_read("`DConnection to the server failed`");

                std::cout << "Server down" << std::endl;
                sf::sleep(sf::milliseconds(5000));
            }
        }

        sf::sleep(sf::milliseconds(8));
    }

    shared->termination_count++;

    printf("watchdog\n");

    //ctx.sock->shutdown();
}

void test_http_client(shared_data& shared)
{
    shared_context* ctx = new shared_context();

    std::thread(handle_async_read, &shared, std::ref(*ctx)).detach();
    std::thread(handle_async_write, &shared, std::ref(*ctx)).detach();
    std::thread(watchdog, &shared, std::ref(*ctx)).detach();
}
