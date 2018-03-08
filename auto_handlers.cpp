#include "auto_handlers.hpp"
#include "string_helpers.hpp"

bool interop_starts_with(const std::vector<interop_char>& chs, int idx, const std::string& str)
{
    if(idx < 0 || idx + (int)str.size() > (int)chs.size())
        return false;

    int offset = 0;

    for(int i=idx; i < idx + (int)str.size(); i++)
    {
        if(chs[i].c != str[offset])
            return false;

        offset++;
    }

    return true;
}

void interop_colour(std::vector<interop_char>& chs, int idx, const std::string& str, vec3f col)
{
    if(!interop_starts_with(chs, idx, str))
        return;

    for(int i=idx; i < idx + (int)str.size(); i++)
    {
        chs[i].col = col;
    }
}

std::vector<interop_char> auto_handler::auto_colour(const std::vector<interop_char>& in)
{
    std::vector<interop_char> ret = in;

    std::map<std::string, vec3f> cols
    {
        {"#fs", {60, 255, 60}},
        {"#hs", {255, 255, 40}},
        {"#ms", {255, 140, 40}},
        {"#ls", {255, 20, 20}},
        {"#ns", {255, 20, 255}},
    };

    /*if(interop_starts_with(ret, 0, "#fs."))
    {
        interop_colour(ret, 0, "#fs.", {60, 255, 60});
    }*/

    for(auto& i : cols)
    {
        interop_colour(ret, 0, i.first, i.second);
    }

    return ret;
}
