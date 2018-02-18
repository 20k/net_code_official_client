#ifndef HTTP_BEAST_CLIENT_HPP_INCLUDED
#define HTTP_BEAST_CLIENT_HPP_INCLUDED

#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <mutex>

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

struct shared_data
{
    std::deque<std::string> read_queue;
    std::deque<std::string> write_queue;

    std::mutex ilock;

    void make_lock()
    {
        ilock.lock();
    }

    void make_unlock()
    {
        ilock.unlock();
    }

    bool has_front_read()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return read_queue.size() > 0;
    }

    bool has_front_write()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return write_queue.size() > 0;
    }

    std::string get_front_read()
    {
        std::lock_guard<std::mutex> lk(ilock);

        std::string ret = read_queue.front();

        read_queue.pop_front();

        return ret;
    }

    std::string get_front_write()
    {
        std::lock_guard<std::mutex> lk(ilock);

        std::string ret = write_queue.front();

        write_queue.pop_front();

        return ret;
    }

    void add_front_write(const std::string& str)
    {
        std::lock_guard<std::mutex> lk(ilock);

        write_queue.push_back(str);
    }

    void add_front_read(const std::string& str)
    {
        std::lock_guard<std::mutex> lk(ilock);

        read_queue.push_back(str);
    }
};

void test_http_client();

#endif // HTTP_BEAST_CLIENT_HPP_INCLUDED
