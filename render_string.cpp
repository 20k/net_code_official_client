#include "render_string.hpp"
#include "colour_interop.hpp"
#include <iostream>

///this function should handle autocolouring
std::vector<render_string> create_render_strings(std::string_view in, bool include_specials)
{
    std::vector<render_string> ret;

    if(in.size() == 0)
        return ret;

    vec3f default_colour = letter_to_colour('A').value();

    render_string current_chunk;
    current_chunk.colour = default_colour;

    bool currently_colouring = false;
    bool set_colour = false;

    auto bump_colour = [&]()
    {
        ///else, reset current chunk. Resetting current chunk is a no op, because the length is 0 and the start is valid
        ///the colour always gets overwritten
        if(current_chunk.length > 0)
        {
            ret.push_back(current_chunk);
            render_string& last_chunk = ret.back();

            current_chunk = render_string();
            current_chunk.start = last_chunk.start + last_chunk.length;
            current_chunk.length = 0;
        }
    };

    auto add_index = [&](int idx)
    {
        if(idx == current_chunk.start + current_chunk.length)
        {
            current_chunk.length++;
        }
        else
        {
            assert(idx > current_chunk.start + current_chunk.length);

            vec3f last_colour = current_chunk.colour;

            bump_colour();

            current_chunk.colour = last_colour;
            current_chunk.start = idx;
            current_chunk.length = 1;
        }
    };

    int length = in.size();

    for(int i=0; i < length; i++)
    {
        char cur = in[i];
        char next = 0;
        char next_2 = 0;

        if(i < length - 1)
            next = in[i + 1];

        if(i < length - 2)
            next_2 = in[i + 2];

        // \\`
        if(cur == '\\' && next == '\\' && next_2 == '`')
        {
            add_index(i);

            if(include_specials)
            {
                add_index(i+1);
            }

            i++;
            continue;
        }

        // \`
        if(cur == '\\' && next == '`')
        {
            if(include_specials)
            {
                add_index(i);
            }

            add_index(i+1);

            i++;
            continue;
        }

        // ` initial
        if(cur == '`' && !currently_colouring)
        {
            currently_colouring = true;

            vec3f col = letter_to_colour(next).value_or(default_colour);

            bump_colour();
            current_chunk.colour = col;

            if(include_specials)
            {
                add_index(i);
                add_index(i+1);
            }

            if(next != '`')
                i++;

            continue;
        }

        if(cur != '`' && cur != '\n')
        {
            add_index(i);
            continue;
        }

        if((cur == '`' && currently_colouring) || cur == '\n')
        {
            if(include_specials)
            {
                add_index(i);
            }
            else
            {
                if(cur != '`')
                {
                    add_index(i);
                }
            }

            bump_colour();
            currently_colouring = false;
            current_chunk.colour = default_colour;
            continue;
        }
    }

    bump_colour();

    return ret;
}

paragraph_string::paragraph_string(){}

paragraph_string::paragraph_string(std::string_view in, bool include_specials)
{
    basic_render_strings = create_render_strings(in, include_specials);
}

void paragraph_string::build(vec2f clipping_width)
{

}

void test_render_strings()
{
    std::string base = "hello there []\" asdf `Xcatepillar`\n`B`uncoloured\n`Dhithere\n``uncoloured`Dcoloured1`randomtext`Bcoloured2\n";

    std::vector<render_string> strs = create_render_strings(base, false);

    for(render_string& rstr : strs)
    {
        std::string_view view(base.begin() + rstr.start, base.begin() + rstr.start + rstr.length);

        std::cout << view << "|";
    }
}
