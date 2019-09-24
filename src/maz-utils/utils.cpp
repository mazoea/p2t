#include "utils.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

namespace maz {

    const int OK = 0;
    const int CONTINUE = -1;
    const int VERSION = -42;
    const int INVALID_PARAM = 1;
    const int EXCEPTION = 100;
    const int DONT_KNOW = -1000;

    const l_float32 DEGREE_TO_RADIAN = 3.14159f / 180.f;
    const l_float32 RADIAN_TO_DEGREE = 180.f / 3.14159f;

    //
    //
    //

    void slogger::warn(const std::string& msg) const { log_(std::cerr, "WARN", msg); }

    void slogger::warn(const std::string& msg1, const std::string& msg2) const
    {
        log_(std::cerr, "WARN", msg1 + " " + msg2);
    }

    void slogger::error(const std::string& msg) const { log_(std::cerr, "ERROR", msg); }

    void slogger::error(const std::string& msg1, const std::string& msg2) const
    {
        log_(std::cerr, "ERROR", msg1 + " " + msg2);
    }

    //
    //
    //

    int rotations::next()
    {
        int ret = -1;

        if (0 == step)
        {
            ret = (-1 != guessed_) ? guessed_ : 0;

        } else if (1 == step && -1 != guessed_)
        {
            // the opposite direction
            ret = (guessed_ + 180) % 360;
        } else
        {
            // get first avail
            for (int i = 0; i < 4; ++i)
            {
                if (0 == done_[i])
                {
                    ret = i * 90;
                    break;
                }
            }
        }

        done_[ret / 90] = 1;
        ++step;
        return ret;
    }

    //
    //
    //

    void runs::add(bool on)
    {
        auto diff = on ? ON : OFF;
        if (r_.empty())
        {
            r_.push_back(diff);
            return;
        }

        // change of sides
        if (0 > r_.back() * diff)
        {
            r_.push_back(diff);
            return;
        }

        // same side
        r_.back() += diff;
    }

    bool runs::matches(std::list<int> def) const
    {
        if (def.size() != r_.size()) return false;
        // any one not matching the sign => not matching
        if (std::any_of(r_.begin(), r_.end(), [&def](int r) {
                int b = def.front();
                def.pop_front();
                return 0 > (r * b);
            }))
        {
            return false;
        }
        return true;
    }

    int runs::first(int on_off) const
    {
        for (int v : r_)
        {
            if (0 < on_off * v) return v;
        }
        return 0;
    }

    //
    //
    //

    bool starts_with(std::string haystack, std::string prefix)
    {
        if (haystack.length() < prefix.length()) return false;
        return 0 == haystack.compare(0, prefix.length(), prefix);
    }

    bool ends_with(std::string haystack, std::string suffix)
    {
        if (haystack.length() < suffix.length()) return false;
        return 0 == haystack.compare(haystack.length() - suffix.length(), suffix.length(), suffix);
    }

    bool in_array(char c, const std::string& chars)
    {
        for (size_t i = 0; i < chars.length(); ++i)
            if (c == chars.at(i)) return true;
        return false;
    }

    bool in_array(const std::string& str, const std::string& chars)
    {
        if (str.length() != 1)
        {
            return false;
        }
        return in_array(str.at(0), chars);
    }

    template <> void value<std::string>(const std::string& val, std::string& ret) { ret = val; }

    bool only_numbers_specific(const char* cstr)
    {
        if (!cstr) return false;
        std::string s(cstr);
        std::string::const_iterator it = s.begin();
        static const std::string accepted_chars(",.:-/");
        // do not use isdigit & similar because of portability
        while (it != s.end() && (maz_is_digit(*it) || in_array(*it, accepted_chars)))
        {
            ++it;
        }
        return !s.empty() && it == s.end();
    }

    bool only_numbers(const char* cstr)
    {
        if (!cstr) return false;
        return only_numbers(std::string(cstr));
    }

    bool only_numbers(const std::string& s)
    {
        if (s.empty()) return false;
        return s.find_first_not_of("0123456789") == std::string::npos;
    }

    bool has_number(const std::string& str)
    {
        return std::string::npos != str.find_first_of("0123456789");
    }

    bool has_whitespace(const std::string& str)
    {
        return std::string::npos != str.find_first_of(" \n");
    }

    bool _isspace(int c)
    {
        if (!between(0, c, 256)) return false;
        return 0 != isspace(c);
    }

    char maz_tolower(int c)
    {
        if (!between(0, c, 256)) return '?';
        return static_cast<char>(tolower(c));
    }

    char maz_toupper(int c)
    {
        if (!between(0, c, 256)) return '?';
        return static_cast<char>(toupper(c));
    }

    namespace {
        const std::string strip_chars = ("\r\n ");
    } // namespace

    void lstrip(std::string& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !_isspace(c); }));
    }

    void rstrip(std::string& s)
    {
        s.erase(
            std::find_if(s.rbegin(), s.rend(), [](int c) { return !_isspace(c); }).base(), s.end());
    }

    bool strip_word(std::string& str)
    {
        bool changed = false;

        std::string new_string;
        bool already_added = false;
        for (char i : str)
        {
            if (in_array(i, strip_chars))
            {
                if (already_added && *(new_string.rbegin()) != ' ') new_string += " ";
                continue;
            }
            already_added = true;
            new_string += i;
        }

        if (!new_string.empty() && *(new_string.rbegin()) == ' ')
            new_string = new_string.substr(0, new_string.length() - 1);

        changed = (str != new_string);
        str = new_string;
        return changed;
    }

    void strip(std::string& s)
    {
        lstrip(s);
        rstrip(s);
    }

    std::string remove_whitespace_copy(std::string str)
    {
        remove_whitespace(str);
        return str;
    }

    void remove_whitespace(std::string& str)
    {
        str.erase(std::remove_if(str.begin(), str.end(), _isspace), str.end());
    }

    namespace {
        const char encodeLookup[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const char pad_c = '=';
    } // namespace

    std::string base64_encode(unsigned char* data, size_t len)
    {
        typedef char TCHAR;
        std::basic_string<TCHAR> encodedString;
        encodedString.reserve(((len / 3) + (len % 3 > 0)) * 4);
        int temp = 0;
        unsigned char* cursor = data;
        for (size_t idx = 0; idx < len / 3; ++idx)
        {

            temp = (*cursor++) << 16; // Convert to big endian
            temp += (*cursor++) << 8;
            temp += (*cursor++);
            encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
            encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
            encodedString.append(1, encodeLookup[(temp & 0x00000FC0) >> 6]);
            encodedString.append(1, encodeLookup[(temp & 0x0000003F)]);
        }
        switch (len % 3)
        {
        case 1:
            temp = (*cursor++) << 16; // Convert to big endian
            encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
            encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
            encodedString.append(2, pad_c);
            break;
        case 2:
            temp = (*cursor++) << 16; // Convert to big endian
            temp += (*cursor++) << 8;
            encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
            encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
            encodedString.append(1, encodeLookup[(temp & 0x00000FC0) >> 6]);
            encodedString.append(1, pad_c);
            break;
        }
        return encodedString;
    }

    std::vector<unsigned char> base64_decode(const std::string& input)
    {
        static const char padCharacter = '=';

        if (input.length() % 4) // Sanity check
            throw std::runtime_error("Non-Valid base64!");
        size_t padding = 0;
        if (input.length())
        {
            if (input[input.length() - 1] == padCharacter) padding++;
            if (input[input.length() - 2] == padCharacter) padding++;
        }
        // Setup a vector to hold the result
        std::vector<unsigned char> decodedBytes;
        decodedBytes.reserve(((input.length() / 4) * 3) - padding);
        long temp = 0; // Holds decoded quanta
        auto it = input.begin();
        unsigned char c;
        while (it != input.end())
        {
            for (size_t quantumPosition = 0; quantumPosition < 4; quantumPosition++)
            {
                c = static_cast<unsigned char>(*it);
                temp <<= 6;
                if (c >= 0x41 && c <= 0x5A) // This area will need tweaking if
                    temp |= c - 0x41;       // you are using an alternate alphabet
                else if (c >= 0x61 && c <= 0x7A)
                    temp |= c - 0x47;
                else if (c >= 0x30 && c <= 0x39)
                    temp |= c + 0x04;
                else if (c == 0x2B)
                    temp |= 0x3E; // change to 0x2D for URL alphabet
                else if (c == 0x2F)
                    temp |= 0x3F;           // change to 0x5F for URL alphabet
                else if (c == padCharacter) // pad
                {
                    switch (input.end() - it)
                    {
                    case 1: // One pad character
                        decodedBytes.push_back((temp >> 16) & 0x000000FF);
                        decodedBytes.push_back((temp >> 8) & 0x000000FF);
                        return decodedBytes;
                    case 2: // Two pad characters
                        decodedBytes.push_back((temp >> 10) & 0x000000FF);
                        return decodedBytes;
                    default:
                        throw std::runtime_error("Invalid Padding in Base 64!");
                    }
                } else
                    throw std::runtime_error("Non-Valid Character in Base 64!");
                ++it;
            }
            decodedBytes.push_back((temp >> 16) & 0x000000FF);
            decodedBytes.push_back((temp >> 8) & 0x000000FF);
            decodedBytes.push_back((temp)&0x000000FF);
        }
        return decodedBytes;
    }

    void empty_function() {}

    void empty_function(int) {}

    std::string now()
    {
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        // cppcheck-suppress obsoleteFunctionsasctime
        std::string time_str(asctime(timeinfo));
        return time_str.substr(0, time_str.length() - 1);
    }

    //#include <xmmintrin.h>
    // namespace {
    //    // plus one newton raphson step
    //    void sse_rsqrt_nr(float* pOut, float* pIn)
    //    {
    //        __m128 in = _mm_load_ss(pIn);
    //        _mm_store_ss(pOut, _mm_rsqrt_ss(in));
    //    }
    //}

    // double sse_rsqrt_nr(double x)
    //{
    //    float res = 0.f;
    //    float fx = static_cast<float>(x);
    //    sse_rsqrt_nr(&res, &fx);
    //    return static_cast<double>(res);
    //}

    double clocks_diff_s(const clock_t& start)
    {
        return round<3, double>(static_cast<double>(clock() - start) / CLOCKS_PER_SEC);
    }

    namespace {
        template <std::size_t Prime, std::size_t Salt>
        std::size_t _hash_fnv(const unsigned char* buf, std::size_t len)
        {
            std::size_t hash = Salt;
            for (std::size_t i = 0; i < len; ++i)
            {
                hash *= Prime;
                hash ^= buf[i];
            }

            return hash;
        }

        // For 32 bit machines:
        static constexpr std::size_t fnv_prime32 = 16777619u;
        static constexpr std::size_t fnv_offset_basis32 = 2166136261u;
        // For 64 bit machines:
        // fnv_prime = 1099511628211u;
        // fnv_offset_basis = 14695981039346656037u;
    } // namespace

    std::size_t hash_fnv(const unsigned char* buf, std::size_t len)
    {
        return _hash_fnv<fnv_prime32, fnv_offset_basis32>(buf, len);
    }

} // namespace maz
