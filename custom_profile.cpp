#include "custom_profile.hpp"
#include <stdio.h>
#include <toolkit/stacktrace.hpp>
#include <iostream>
#include <unordered_map>
#include <toolkit/clock.hpp>
#include <array>
#include <assert.h>
#include <map>
#include <atomic>
#include <vector>

thread_local int stack_count = 0;
thread_local int stack_stop = 0;

struct stack_stopper
{
    stack_stopper() __attribute__((no_instrument_function))
    {
        stack_stop++;
    }

    ~stack_stopper() __attribute__((no_instrument_function))
    {
        stack_stop--;
    }
};

__attribute__((no_instrument_function))
std::atomic_int* get_profile_ptr_impl(const std::thread::id& id)
{
    static std::map<std::thread::id, std::atomic_int> ptr_map;
    static std::mutex mut;

    std::lock_guard guard(mut);

    auto it = ptr_map.find(id);

    if(it == ptr_map.end())
    {
        ptr_map[id] = 1;
        return &ptr_map[id];
    }
    else
    {
        return &it->second;
    }
}

__attribute__((no_instrument_function))
std::atomic_int* get_profile_ptr(const std::thread::id& id)
{
    stack_stopper stop;

    return get_profile_ptr_impl(id);
}

__attribute__((no_instrument_function))
std::atomic_int* get_profile_ptr_self()
{
    return get_profile_ptr_impl(std::this_thread::get_id());
}

thread_local std::atomic_int* profile = nullptr;

void profiling::disable_thread()
{
    disable_thread(std::this_thread::get_id());
}

void profiling::disable_thread(std::thread::id id)
{
    *get_profile_ptr(id) = 1;
}

struct profile_data
{
    std::string name;

    static constexpr int max_values = 5;

    std::array<double, max_values + 1> longest_times;
    double avg = 0;

    void add_value(const double& in)
    {
        longest_times[max_values] = in;
        std::sort(longest_times.begin(), longest_times.end(), std::greater<double>());

        avg = (avg + in)/2.;
    }
};

constexpr int max_stack_depth = 4096;

thread_local std::map<std::pair<std::thread::id, void*>, profile_data> data;
thread_local std::array<steady_timer, max_stack_depth> timers;

extern "C"
{
    void __cyg_profile_func_enter(void *, void *) __attribute__((no_instrument_function));
    void __cyg_profile_func_exit(void *, void *) __attribute__((no_instrument_function));

    void __cyg_profile_func_enter (void *this_fn, void *call_site)
    {
        int old_stack_count = stack_count;

        stack_count++;

        if(stack_stop > 0)
            return;

        stack_stopper stop;

        if(profile && *profile == 0)
            return;

        if(profile == nullptr)
        {
            profile = get_profile_ptr_self();
        }

        if(old_stack_count < max_stack_depth)
            timers[old_stack_count].restart();

        //stack_frame frame = frame_from_ptr(this_fn);

        //std::cout << "HERE " << frame.name << " " << frame.file << ":" << frame.line << std::endl;

        //std::cout << get_stacktrace() << std::endl;
    }

    void __cyg_profile_func_exit (void *this_fn, void *call_site)
    {
        stack_count--;

        if(stack_stop > 0)
            return;

        if(stack_count >= max_stack_depth)
            return;

        stack_stopper stop;

        if(profile && *profile == 0)
            return;

        assert(stack_count >= 0 && stack_count < max_stack_depth);

        std::pair<std::thread::id, void*> my_key = {std::this_thread::get_id(), this_fn};

        auto it = data.find(my_key);

        if(it == data.end())
        {
            stack_frame frame = frame_from_ptr(this_fn);

            profile_data& dat = data[my_key];

            dat.name = frame.name + " " + frame.file + ":" + std::to_string(frame.line);

            it = data.find(my_key);
        }

        steady_timer& elapsed = timers[stack_count];

        profile_data& dat = it->second;

        dat.add_value(elapsed.get_elapsed_time_s());
        //dat.elapsed.push_back();

        //std::cout << "ELAPSED " << frame.name  << " at " << frame.file << ":" << frame.line << " TIME " << elapsed.get_elapsed_time_s() << std::endl;
    }
}
