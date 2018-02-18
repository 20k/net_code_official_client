#ifndef COLOUR_INTEROP_HPP_INCLUDED
#define COLOUR_INTEROP_HPP_INCLUDED

std::map<char, vec3f> get_cmap()
{
    std::map<char, vec3f> colour_map;

    ///sigh
    colour_map['A'] = {240, 240, 240};
    colour_map['B'] = {234, 241, 241};
    colour_map['C'] = {206, 202, 203};
    colour_map['D'] = {243, 8, 8};
    colour_map['E'] = {242, 164, 160};
    colour_map['F'] = {243, 166, 3};
    colour_map['G'] = {243, 231, 130};
    colour_map['H'] = {243, 241, 7};
    colour_map['I'] = {243, 243, 113};
    colour_map['J'] = {243, 243, 7};
    colour_map['K'] = {242, 242, 193};
    colour_map['L'] = {42, 243, 8};
    colour_map['M'] = {231, 243, 204};
    colour_map['N'] = {5, 243, 243};
    colour_map['O'] = {179, 243, 243};
    colour_map['P'] = {8, 143, 242};
    colour_map['Q'] = {213, 242, 243};
    colour_map['R'] = {4, 8, 243};
    colour_map['S'] = {149, 228, 242};
    colour_map['T'] = {237, 60, 242};
    colour_map['U'] = {241, 236, 241};
    colour_map['V'] = {242, 6, 242};
    colour_map['W'] = {241, 186, 241};
    colour_map['X'] = {241, 6, 142};
    colour_map['Y'] = {240, 123, 195};
    colour_map['Z'] = {11, 16, 40};

    colour_map['a'] = {1,1,1};
    colour_map['b'] = {60,60,60};
    colour_map['c'] = {118,118,118};
    colour_map['d'] = {158,1,1};
    colour_map['e'] = {185,49,49};
    colour_map['f'] = {211,79,2};
    colour_map['g'] = {138,88,50};
    colour_map['h'] = {216,163,2};
    colour_map['i'] = {232,185,71};
    colour_map['j'] = {186,188,1};
    colour_map['k'] = {73,90,34};
    colour_map['l'] = {38,183,1};
    colour_map['m'] = {33,56,26};
    colour_map['n'] = {1,87,100};
    colour_map['o'] = {46,75,79};
    colour_map['p'] = {2,135,213};
    colour_map['q'] = {51,97,127};
    colour_map['r'] = {2,2,122};
    colour_map['s'] = {82,143,207};
    colour_map['t'] = {106,27,158};
    colour_map['u'] = {68,46,81};
    colour_map['v'] = {176,1,123};
    colour_map['w'] = {192,52,160};
    colour_map['x'] = {173,1,37};
    colour_map['y'] = {140,42,72};
    colour_map['z'] = {15,17,20};

    return colour_map;
}

struct interop_char
{
    std::string characters;
    vec3f col = {1,1,1};
};

std::vector<interop_char> build_from_colour_string(std::string in)
{
    std::vector<interop_char> ret;

    interop_char last;
    bool found_colour = false;
    bool set_colour = false;

    bool termd = false;

    for(int i=0; i < in.size(); i++)
    {
        char cur = in[i];

        termd = false;

        if(cur == '`' && !found_colour)
        {
            set_colour = true;
            continue;
        }

        if(set_colour)
        {
            set_colour = false;
            found_colour = true;

            last.colour = get_cmap()[cur];
            continue;
        }

        if(cur != '`')
        {
            last.characters += std::string(1, cur);

            continue;
        }

        if(cur == '`' && found_colour)
        {
            ret.push_back(last);

            termd = true;

            found_colour = false;
            set_colour = false;

            continue;
        }
    }

    if(!termd)
    {
        last.characters = "`" + last.characters;

        ret.push_back(last);
    }

    return ret;
}

#endif // COLOUR_INTEROP_HPP_INCLUDED
