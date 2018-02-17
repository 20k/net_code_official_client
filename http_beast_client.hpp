#ifndef HTTP_BEAST_CLIENT_HPP_INCLUDED
#define HTTP_BEAST_CLIENT_HPP_INCLUDED

#include <fstream>
#include <string>
#include <vector>

/*struct request
{
    //int64_t id;
    std::string data;
};

struct request_holder
{
    std::mutex mut;

    std::deque<request> to_process;
    //std::map<int64_t, std::string> send_back;

    void add_request(request& r)
    {
        std::lock_guard<std::mutex> lock(mut);

        to_process.push_back(r);
    }

    request fetch_remove_next()
    {
        std::lock_guard<std::mutex> lock(mut);

        request req = to_process[0];

        to_process.pop_front();

        return req;
    }

    void add_to_send_back_queue(request& r)
    {
        std::lock_guard<std::mutex> lock(mut);

        send_back[r.id] = r.data;
    }

    bool any_requests()
    {
        std::lock_guard<std::mutex> lock(mut);

        return to_process.size() > 0;
    }
};*/

inline
std::string read_file(const std::string& file)
{
    std::ifstream t(file);
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    return str;
}

void test_http_client();

#endif // HTTP_BEAST_CLIENT_HPP_INCLUDED
