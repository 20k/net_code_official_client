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

thread_local int stack_count = 0;
thread_local int stack_stop = -1;

struct stack_stopper
{
    stack_stopper() __attribute__((no_instrument_function))
    {
        stack_stop = 1;
    }

    ~stack_stopper() __attribute__((no_instrument_function))
    {
        stack_stop = -1;
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
    stack_stopper stop;

    return get_profile_ptr_impl(std::this_thread::get_id());
}

thread_local std::atomic_int* profile = nullptr;

void profiling::disable_thread()
{
    disable_thread(std::this_thread::get_id());
}

void profiling::disable_thread(std::thread::id id)
{
    *get_profile_ptr_impl(id) = 1;
}

struct cached_data
{
    std::string name;
};

constexpr int max_stack_depth = 4096;

thread_local std::unordered_map<void*, cached_data> data;
thread_local std::array<steady_timer, max_stack_depth> timers;

extern "C"
{
    void __cyg_profile_func_enter(void *, void *) __attribute__((no_instrument_function));
    void __cyg_profile_func_exit(void *, void *) __attribute__((no_instrument_function));

    void __cyg_profile_func_enter (void *this_fn, void *call_site)
    {
        if(profile && *profile == 0)
            return;

        int old_stack_count = stack_count;

        stack_count++;

        if(stack_stop != -1)
            return;

        if(profile == nullptr)
        {
            profile = get_profile_ptr_self();
        }

        stack_stopper stop;

        if(stack_count < max_stack_depth)
            timers[old_stack_count].restart();

        //stack_frame frame = frame_from_ptr(this_fn);

        //std::cout << "HERE " << frame.name << " " << frame.file << ":" << frame.line << std::endl;

        //std::cout << get_stacktrace() << std::endl;
    }

    void __cyg_profile_func_exit (void *this_fn, void *call_site)
    {
        if(profile && *profile == 0)
            return;

        stack_count--;

        if(stack_stop != -1)
            return;

        if(stack_count >= max_stack_depth)
            return;

        stack_stopper stop;

        assert(stack_count >= 0 && stack_count < max_stack_depth);

        auto it = data.find(this_fn);

        if(it == data.end())
        {
            stack_frame frame = frame_from_ptr(this_fn);

            data[this_fn];
        }

        steady_timer& elapsed = timers[stack_count];

        //std::cout << "ELAPSED " << frame.name  << " at " << frame.file << ":" << frame.line << " TIME " << elapsed.get_elapsed_time_s() << std::endl;
    }
}
