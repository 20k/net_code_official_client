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
    std::string display_name;

    static constexpr int max_values = 5;

    std::array<double, max_values + 1> longest_times;
    double avg = 0;
    double total = 0;

    void add_value(const double& in)
    {
        longest_times[max_values] = in;
        std::sort(longest_times.begin(), longest_times.end(), std::greater<double>());

        if(avg == 0)
            avg = in;
        else
            avg = (avg + in)/2.;

        total += in;
    }
};

constexpr int max_stack_depth = 4096;

thread_local std::map<void*, profile_data> data;
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

        void* my_key = this_fn;

        auto it = data.find(my_key);

        if(it == data.end())
        {
            stack_frame frame = frame_from_ptr(this_fn);

            profile_data& dat = data[my_key];

            dat.name = frame.name;
            dat.display_name = frame.name + " " + frame.file + ":" + std::to_string(frame.line);

            it = data.find(my_key);
        }

        steady_timer& elapsed = timers[stack_count];

        profile_data& dat = it->second;

        dat.add_value(elapsed.get_elapsed_time_s());
        //dat.elapsed.push_back();

        //std::cout << "ELAPSED " << frame.name  << " at " << frame.file << ":" << frame.line << " TIME " << elapsed.get_elapsed_time_s() << std::endl;
    }
}

std::string format_single(const profile_data& dat)
{
    std::string val;

    val += "\n------\n";

    val += dat.display_name + "\n";

    val += "TOP 5:\n";

    for(int i=0; i < dat.max_values; i++)
    {
        if(dat.longest_times[i] == 0)
            continue;

        val += std::to_string(dat.longest_times[i]) + "\n";
    }

    val += "AVG: " + std::to_string(dat.avg) + "\n";
    val += "TOTAL: " + std::to_string(dat.total) + "\n------\n";

    return val;
}

std::string profiling::format_profiling_data(const std::string& function_name)
{
    stack_stopper stop;

    for(auto& [func, pdata] : data)
    {
        profile_data& dat = pdata;

        if(dat.name != function_name)
            continue;

        return format_single(dat);
    }

    return "";
}

std::string profiling::format_profiling_data()
{
    stack_stopper stop;

    std::string val;

    std::vector<profile_data> sorted_profile_data;

    for(auto& [func, pdata] : data)
    {
        sorted_profile_data.push_back(pdata);
    }

    std::sort(sorted_profile_data.begin(), sorted_profile_data.end(), [](profile_data& d1, profile_data& d2)
    {
        return d1.total > d2.total;
    });

    for(profile_data& dat : sorted_profile_data)
    {
        val += format_single(dat);
    }

    return val;
}
