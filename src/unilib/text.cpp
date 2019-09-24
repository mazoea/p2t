/*
 *  Mazoea s.r.o.
 *  @author jm
 */

#include <algorithm>
#include <map>

#include "unilib/text.h"
#include "unilib/uninorms.h"

namespace unilib {

    class utf8;

    bool is_hyphen(char32_t c) { return 0 != (unicode::category(c) & unicode::Pd); }

    bool is_hyphen(const utf8_string& s) { return is_hyphen(utf8::first(s)); }

    bool is_space(char32_t c) { return 0 != (unicode::category(c) & unicode::Zs); }

    bool is_space(const utf8_string& s) { return is_space(utf8::first(s)); }

    bool compare(const utf8_string& s1, char c1) { return s1[0] == c1; }

    utf8_string from_utf8(utf8_char c)
    {
        std::string ret;
        do
        {
            utf8_string::value_type tc = static_cast<char>(c & 0xff);
            ret = tc + ret;
        } while ((c = c >> 8));

        return ret;
    }

    bool contains_ascii(const char* needles, const std::string& haystack)
    {
        const char* needle = needles;
        while (*needle != '\0')
        {
            if (is_in(*needle, haystack))
            {
                return true;
            }
            ++needle;
        }
        return false;
    }

    utf8_string to_lower_first(const utf8_string& s)
    {
        char32_t result = unicode::lowercase(utf8::first(s));
        utf8_string ret;
        utf8::append(ret, result);
        return ret;
    }

    utf8_string to_lower(const utf8_string& str)
    {
        utf8_string res;
        utf8::map(unicode::lowercase, str, res);
        return res;
    }

    utf8_string to_upper(const utf8_string& str)
    {
        utf8_string res;
        utf8::map(unicode::uppercase, str, res);
        return res;
    }

    bool is_lower(const utf8_string& s, int len)
    {
        for (auto ch : utf8::decoder(s))
        {
            if (!(unicode::category(ch) & unicode::Ll)) return false;
            if (0 == --len) break;
        }
        return true;
    }

    bool is_lower(const utf8_string& s) { return is_lower(s, -1); }

    //

    bool is_upper_first(const utf8_string& s)
    {
        char32_t u = utf8::first(s);
        return u == unicode::uppercase(u);
    }

    bool is_upper(const utf8_string& s, int len)
    {
        for (auto ch : utf8::decoder(s))
        {
            if (!(unicode::category(ch) & unicode::Lu)) return false;
            if (0 == --len) break;
        }
        return true;
    }

    bool is_upper(const utf8_string& s) { return is_upper(s, -1); }

    bool is_first_capital(const utf8_string& s)
    {
        const char* sc = s.c_str();
        int pos = 0;
        for (char32_t u; (u = utf8::decode(sc)); ++pos)
        {
            if (0 == pos)
            {
                if (u != unicode::uppercase(u))
                {
                    return false;
                }
            } else
            {
                if (u == unicode::uppercase(u))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool icompare(const utf8_string& s1, const utf8_string& s2)
    {
        if (s1.size() != s2.size())
        {
            return false;
        }

        const char* sc1 = s1.c_str();
        const char* sc2 = s2.c_str();
        for (char32_t u1, u2; (u1 = utf8::decode(sc1)) && (u2 = utf8::decode(sc2));)
        {
            if (unicode::lowercase(u1) != unicode::lowercase(u2))
            {
                return false;
            }
        }
        return true;
    }

    //
    //

    bool is_on_baseline_utf8(utf8_char c)
    {
        return is_in(c, cfp::baselines_utf8) ||
               (is_ascii_char(c) && is_in(static_cast<char>(c), cfp::baselines));
    }

    bool is_on_baseline_utf8(const utf8_string& s) { return is_on_baseline_utf8(utf8::first(s)); }

    bool is_normal_width(utf8_char c)
    {
        return is_ascii_char(c) && is_in(static_cast<char>(c), cfp::letter_width_chars_normal);
    }

    bool is_normal_width_first(const utf8_string& s) { return is_normal_width(s[0]); }

    bool is_numeric(const std::string& s)
    {
        static const std::string digits("0123456789");
        static const std::string delims("/.,;:-()");
        size_t cnt = 0;
        for (auto c : s)
        {
            if (is_in(c, digits))
            {
                ++cnt;
                continue;
            }
            if (is_in(c, delims))
            {
                continue;
            }
            return false;
        }
        return 0 < cnt;
    }

    //
    //

    // get utf8 (diacritic) from ascii
    utf8_string accented_utf8(char ascii)
    {
        std::map<char, utf8_char>::const_iterator it = cfp::map_to_accent.find(ascii);
        if (cfp::map_to_accent.end() == it) return {};
        return from_utf8(it->second);
    }

    utf8_string ascii_from_utf8(const utf8_string& s)
    {
        utf8_string ret;
        // to unicode -> strip -> to result
        std::u32string u32s;
        utf8::decode(s.c_str(), u32s);
        uninorms::nfd(u32s);

        static const std::map<char32_t, char> ascii_map({
            {U'\u0141', 'L'}, // Ł
            {U'\u00d0', 'D'}, // Ð
            {U'\u018f', 'O'}, // Ə
            {U'\u0126', 'H'}, // Ħ
        });

        // manual
        std::for_each(u32s.begin(), u32s.end(), [](char32_t& c32) {
            if (c32 <= 200) return;
            if (ascii_map.end() == ascii_map.find(c32)) return;
            c32 = ascii_map.at(c32);
        });

        //
        u32s.erase(
            std::remove_if(u32s.begin(), u32s.end(), [](char32_t c) { return 255 < c; }),
            u32s.end());
        utf8::encode(u32s, ret);

        return ret;
    }

    //
    //

    const std::string cfp::char_qm_etc("MmTt4Hhiw9");
    const std::string cfp::char_minus_dot("-");

    const std::string cfp::on_baseline_small_chars("acemnorsuvzxw");
    const std::string cfp::under_baseline_chars("gqyp");
    const std::string cfp::little_under_baseline_chars("_;,()[]{}@/|\\$");

    const std::map<char, utf8_char> cfp::map_to_accent = {
        {'C', 0xc48c}, // Č
        {'c', 0xc48d}, // č
        {'S', 0xc5a0}, // Š
        {'s', 0xc5a1}, // š
        {'Z', 0xc5bd}, // Ž
        {'z', 0xc5be}, // ž
        {'A', 0xc381}, // Á
        {'a', 0xc3a1}, // á
        {'U', 0xc39a}, // Ú
        {'u', 0xc3ba}  // ú
    };

    const std::array<utf8_char, 32> cfp::baselines_utf8 = {
        0xc38d, // Í
        0xc393, // Ó
        0xc394, // Ô
        0xc39a, // Ú
        0xc39c, // Ü
        0xc39d, // Ý
        0xc381, // Á
        0xc389, // É
        0xc3a1, // á
        0xc3a9, // é
        0xc3ad, // í
        0xc3b3, // ó
        0xc3b4, // ô
        0xc3ba, // ú
        0xc3bc, // ü
        0xc48c, // Č
        0xc48d, // č
        0xc48e, // Ď
        0xc48f, // ď
        0xc494, // Ĕ
        0xc495, // ĕ
        0xc4bd, // Ľ
        0xc4be, // ľ
        0xc599, // ř
        0xc5a0, // Š
        0xc5a1, // š
        0xc5a4, // Ť
        0xc5a5, // ť
        0xc5ae, // Ů
        0xc5af, // ů
        0xc5bd, // Ž
        0xc5be  // ž
    };

    const utf8_string
        cfp::baselines("AaBbCcDdEeFfGHhIiJKkLlMmNnOoPRrSsTtUuVvWwXxZYz0123456789!%?:");

    const utf8_string cfp::num_like_chars("IlOo.,:/-+;");
    const utf8_string cfp::num_separators("/.,-");
    const utf8_string cfp::num_exchange_chars("Ss]");
    const utf8_string cfp::digit_like_chars(".-/;:,");
    const utf8_string cfp::date_like_first_chars("IloO");
    const utf8_string cfp::date_like_one_chars("Il");
    const utf8_string cfp::date_like_zero_chars("Oo");
    const utf8_string cfp::degree_symbol_like_chars("0Oo");
    const utf8_string cfp::next_char_after_degree_symbol("CFK");
    const utf8_string cfp::residual_letter("[lI");

    const utf8_string cfp::speckle_like_chars(".,:-)");

    const utf8_string cfp::end_word_punctuation(".?:!,)");

    const utf8_string cfp::letter_width_chars_normal_num("023456789");
    const utf8_string cfp::letter_width_chars_narrow_num("1");
    const utf8_string cfp::letter_width_chars_normal_num_n("2357");

    const utf8_string cfp::small_interpunction("+*");
    const utf8_string cfp::sure_lower_chars("aemnr:+");
    const utf8_string cfp::sure_upper_chars_yu("ABDEFGHIJKLMNPRTbdfhiklt!%?");
    const utf8_string cfp::sure_upper_chars_yl("pyqg");

    const utf8_string cfp::normal_width("AaBbCcDdEeFfGgHhKkLMmNnOoPpQqRrSsTtUuVvXxYyZz023456789");

    // does not have K because that is problematic
    const utf8_string cfp::lower_upper_chars_similar_all("cCkKoOsSuUvVxXzZwW");
    const utf8_string cfp::lower_upper_chars_similar("cCoOsSuUvVxXzZwW");

    const utf8_string cfp::colon_chars("4Zz");

    const utf8_string cfp::same_height_upper_lower("pyPY");

    const utf8_string cfp::char_normal("abcdefghijklmnoprstuvxyzABCDEFGHIJKLMNOPRSTUVXYZ");

    const utf8_string cfp::accented_letters("ZSCUAzscua");

    const utf8_string cfp::char_punct("[],;:_\\/.()-+$");

    const utf8_string cfp::end_delims(":;,.");

    const utf8_string
        cfp::letter_width_chars_normal("aBbCcdEeFGgHhKkLNnoPpQqRSsTUuVvXxZzYy"); // too large "DAO"

    const utf8_string cfp::ascii_special_chars("\".,;-=_~`*");

    const utf8_string cfp::too_narrow_chars(".:;,");

    const utf8_string cfp::too_low_chars(".:=_-;*");

    //
    //

    const utf8_string cfp::accented_d(from_utf8(0xc48f));
    const utf8_string cfp::accented_t(from_utf8(0xc5a5));
    const utf8_string cfp::accented_l(from_utf8(0xc4be));

    const utf8_string cfp::degree_symbol(from_utf8(0xc2b0));

} // namespace unilib
