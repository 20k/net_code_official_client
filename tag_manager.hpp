#ifndef TAG_MANAGER_HPP_INCLUDED
#define TAG_MANAGER_HPP_INCLUDED

#include <string>
#include <vector>

struct server_tagged_message
{
    std::string tag;
    std::string message;
};

struct tag_manager
{
    std::vector<server_tagged_message> msgs;
    inline static int tag_ids = 0;

    int next_get_tag()
    {
        return tag_ids++;
    }

    void add_tagged(const std::string& tag, const std::string& msg)
    {
        msgs.push_back({tag, msg});
    }

    bool received_tag(const std::string& tag)
    {
        for(auto& i : msgs)
        {
            if(i.tag == tag)
                return true;
        }

        return false;
    }

    void remove_tag(const std::string& tag)
    {
        for(int i=0; i < (int)msgs.size(); i++)
        {
            if(msgs[i].tag == tag)
            {
                msgs.erase(msgs.begin() + i);
                i--;
                continue;
            }
        }
    }
};

inline
tag_manager& get_global_tag_manager()
{
    static tag_manager tags;

    return tags;
}

#endif // TAG_MANAGER_HPP_INCLUDED
