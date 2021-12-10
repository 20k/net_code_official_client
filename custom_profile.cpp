#include "custom_profile.hpp"
#include <stdio.h>

extern "C"
{
    void __cyg_profile_func_enter(void *, void *) __attribute__((no_instrument_function));
    void __cyg_profile_func_enter(void *, void *) __attribute__((no_instrument_function));

    void __cyg_profile_func_enter (void *this_fn, void *call_site) {
        printf( "entering %pn", this_fn );
    }

    void __cyg_profile_func_exit (void *this_fn, void *call_site) {
        printf( "leaving %pn", this_fn );
    }
}
