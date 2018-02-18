#include <iostream>

#include "http_beast_client.hpp"

using namespace std;

int main()
{
    shared_data shared;

    test_http_client(shared);

    while(1)
    {
        std::string next_command;

        std::getline(std::cin, next_command);

        shared.add_back_write(next_command);
    }

    return 0;
}
