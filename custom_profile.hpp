#ifndef CUSTOM_PROFILE_HPP_INCLUDED
#define CUSTOM_PROFILE_HPP_INCLUDED

#include <thread>

namespace profiling
{
    void disable_thread() __attribute__((no_instrument_function));
    void disable_thread(std::thread::id id) __attribute__((no_instrument_function));

    std::string format_profiling_data() __attribute__((no_instrument_function));
    std::string format_profiling_data(const std::string& function_name) __attribute__((no_instrument_function));
}

#endif // CUSTOM_PROFILE_HPP_INCLUDED
