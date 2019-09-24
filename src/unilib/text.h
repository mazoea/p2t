/*
 *  Mazoea s.r.o.
 */

#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <string>

#include "unilib/unicode.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#include "unilib/utf8.h"

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace unilib {

    //
    //

    typedef char32_t utf8_char;
    typedef std::string utf8_string;

    static const char space_c = ' ';
    static const char newline_c = '\n';
    static const std::string space(" ");
    static const std::string newline("\n");

    //
    //

    // common font properties
    struct cfp
    {
        // TODO(jms)
        static const utf8_string char_qm_etc;
        static const utf8_string char_minus_dot;
        // small lowercase on baseline
        static const utf8_string on_baseline_small_chars;
        static const utf8_string under_baseline_chars;
        static const utf8_string little_under_baseline_chars;

        // widths
        static const utf8_string letter_width_chars_normal;

        static const utf8_string letter_width_chars_normal_num;
        static const utf8_string letter_width_chars_narrow_num;
        static const utf8_string letter_width_chars_normal_num_n;

        // map some characters to their accents
        static const std::map<char, utf8_char> map_to_accent;

        static const std::array<utf8_char, 32> baselines_utf8;
        static const utf8_string baselines;

        // ocr related
        static const utf8_string num_like_chars;
        // join chars even if the separators are standalone
        static const utf8_string num_separators;
        static const utf8_string num_exchange_chars;

        //
        static const char simple_dash = '-';

        static const utf8_string small_interpunction;
        static const utf8_string sure_lower_chars;
        // TODO(jms)
        static const utf8_string sure_upper_chars_yu;
        static const utf8_string sure_upper_chars_yl;

        // !! TODO(jms) look at is_normal_width
        static const utf8_string normal_width;

        // more generic form
        static const utf8_string lower_upper_chars_similar_all;
        // does not contain K cause it is problematic in correctors
        static const utf8_string lower_upper_chars_similar;

        static const utf8_string colon_chars;

        static const utf8_string same_height_upper_lower;

        static const utf8_string char_normal;
        // TODO(jms)
        static const utf8_string digit_like_chars;

        static const utf8_string date_like_first_chars;
        static const utf8_string date_like_one_chars;
        static const utf8_string date_like_zero_chars;
        static const utf8_string degree_symbol_like_chars;
        static const utf8_string next_char_after_degree_symbol;
        static const utf8_string residual_letter;

        static const utf8_string speckle_like_chars;

        static const utf8_string end_word_punctuation;

        // should be in sync with map_to_accent
        static const utf8_string accented_letters;

        static const utf8_string char_punct;

        static const utf8_string end_delims;

        static const utf8_string too_narrow_chars;
        static const utf8_string too_low_chars;
        static const utf8_string ascii_special_chars;

        static const utf8_string accented_d;
        static const utf8_string accented_t;
        static const utf8_string accented_l;

        static const utf8_string degree_symbol;

    }; // struct

    //
    //
    template <typename T, typename Y> bool is_in(T needle, const Y& haystack)
    {
        for (const auto& c : haystack)
        {
            if (needle == c) return true;
        }
        return false;
    }

    template <typename T> bool is_in(T needle, const std::string& haystack)
    {
        return std::string::npos != haystack.find(needle);
    }

    // is in word digit-like string
    template <typename T> bool is_in(T first, T last, const std::string& haystack)
    {
        for (; first != last; ++first)
        {
            if (!is_in(first->text, haystack))
            {
                return false;
            }
        }
        return true;
    }

    bool contains_ascii(const char* needles, const std::string& haystack);

    //
    //

    utf8_string from_utf8(utf8_char c);

    inline bool is_ascii_char(const utf8_string& s) { return 1 == s.size(); }
    inline bool is_ascii_char(int c) { return 0 == (c >> 8); }
    inline bool is_ascii_char(uint32_t c) { return 0 == (c >> 8); }

    //
    //

    bool is_hyphen(char32_t c);
    // utf8
    bool is_hyphen(const utf8_string& s);

    // unicode!
    bool is_space(char32_t c);
    // utf8
    bool is_space(const utf8_string& s);

    bool compare(const utf8_string& s1, char c1);

    utf8_string to_lower(const utf8_string& s);
    utf8_string to_lower_first(const utf8_string& s);
    bool is_lower(const utf8_string& s, int len);
    bool is_lower(const utf8_string& s);

    utf8_string to_upper(const utf8_string& s);
    bool is_upper(const utf8_string& s);
    bool is_upper(const utf8_string& s, int len);
    bool is_upper_first(const utf8_string& s);
    bool is_first_capital(const utf8_string& s);

    template <typename T> bool is_first_capital(T start, T end);
    bool is_first_capital(const utf8_string& s);

    bool icompare(const utf8_string& s1, const utf8_string& s2);

    //
    //

    // is char baseline char
    bool is_on_baseline_utf8(utf8_char c);
    bool is_on_baseline_utf8(const utf8_string& s);

    bool is_normal_width(utf8_char c);

    template <typename T> bool is_digit_like(T start, T end);

    template <typename T> bool is_digit_like_suspicious(T start, T end);

    /**
     * Return if the text is numeric like e.g., dates, amounts, numbers.
     */
    bool is_numeric(const std::string& s);

    //
    //
    utf8_string accented_utf8(char ascii);

    //
    //
    //
    template <typename T> bool is_first_capital(T start, T end)
    {
        if (!is_upper(start->text)) return false;
        return std::all_of(++start, end, [](decltype(*start)& l) { return is_lower(l.text); });
    }

    template <typename T> bool is_digit_like(T start, T end)
    {
        int digits_cnt = 0;
        for (; start != end; ++start)
        {
            if (!is_ascii_char(start->text)) return false;

            char letter = start->text[0];
            // do not use isdigit & similar because of portability
            if ('0' <= letter && letter <= '9')
            {
                ++digits_cnt;

            } else if (is_in(letter, cfp::num_like_chars))
            {
                continue;

            } else
            {
                return false;
            }
        }

        return 0 < digits_cnt;
    }

    template <typename T> bool is_digit_like_suspicious(T start, T end)
    {
        int digits_cnt = 0;
        int non_digits_cnt = 0;
        for (; start != end; ++start)
        {
            if (!is_ascii_char(start->text))
            {
                return false;
            }
            char letter = start->text[0];

            // do not use isdigit & similar because of portability
            if ('0' <= letter && letter <= '9')
            {
                ++digits_cnt;

            } else if (is_in(letter, cfp::num_like_chars))
            {
                continue;
            } else if (is_in(letter, cfp::num_exchange_chars))
            {
                ++non_digits_cnt;
            } else
            {
                return false;
            }
        }

        return 0 < non_digits_cnt && 0 < digits_cnt;
    }

    /** Return ascii representation of the string. */
    utf8_string ascii_from_utf8(const utf8_string& s);

} // namespace unilib
