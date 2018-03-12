#include "copy_handler.hpp"
#include "string_helpers.hpp"

copy_handler global_copy_handler;

/*bool point_within_rect(vec2f pos, vec2f tl, vec2f br)
{
    return pos.x() >= tl.x() && pos.y() >= tl.y() && pos.x() < br.x() && pos.y() < br.y();
}*/

///if (RectA.X1 < RectB.X2 && RectA.X2 > RectB.X1 &&
//     RectA.Y1 > RectB.Y2 && RectA.Y2 < RectB.Y1)

std::string check_formatted_text(std::vector<std::vector<formatted_char>>& formatted, vec2f start, vec2f fin)
{
    vec2f tl = {std::min(start.x(), fin.x()), std::min(start.y(), fin.y())};
    vec2f br = {std::max(start.x(), fin.x()), std::max(start.y(), fin.y())};

    //std::cout << "st " << tl << std::endl;
    //std::cout << "fn " << br << std::endl;

    std::vector<formatted_char> found;

    for(auto& i : formatted)
    {
        for(formatted_char& chr : i)
        {
            vec2f pos = chr.render_pos;

            vec2f p1 = pos;
            vec2f p4 = pos + (vec2f){char_inf::cwidth, char_inf::cheight};

            bool within = rect_intersect(p1, p4, tl, br);

            if(within)
            {
                found.push_back(chr);

                chr.background_col = {128, 128, 128, 128};
            }
        }
    }

    std::sort(found.begin(), found.end(),
              [](auto& i1, auto& i2)
              {
                  if(i1.render_pos.y() != i2.render_pos.y())
                    return i1.render_pos.y() < i2.render_pos.y();

                  return i1.render_pos.x() < i2.render_pos.x();
              });

    vec2f lpos;

    if(found.size() > 0)
        lpos = found.front().render_pos;

    std::string ret;

    for(int i=0; i < (int)found.size(); i++)
    {
        vec2f fpos = found[i].render_pos;

        if(round(fpos.y()) != round(lpos.y()))
        {
            ret += "\n";
        }

        lpos = fpos;

        ret += std::string(1, found[i].ioc.c);
    }

    return ret;
}

void copy_handler::on_lclick(vec2f pos)
{
    copy_start = pos;
    finished = false;
}

void copy_handler::on_lclick_release(vec2f pos)
{
    copy_end = pos;

    held = false;

    if(finished)
        return;

    finished = true;

    //copied = check_formatted_text(formatted, copy_start, pos);
}

void copy_handler::on_hold_lclick(sf::RenderWindow& win, vec2f pos)
{
    held = true;
    copy_end = pos;
}

void copy_handler::process_formatted(std::vector<std::vector<formatted_char>>& formatted)
{
    if(!finished)
        return;

    if((copy_end - copy_start).length() >= MIN_SELECT_DISTANCE)
    {
        copied = check_formatted_text(formatted, copy_start, copy_end);
        std::cout << copied << std::endl;
        set_clipboard_contents(copied);
    }

    finished = false;
    held = false;
}

void copy_handler::tick(vec2f pos)
{

}
