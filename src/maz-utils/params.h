//
// author: jm (Mazoea s.r.o.)
// date: 2014
//
#pragma once

#include "utils.h"
#include <map>
#include <sstream>
#include <string>

namespace maz {

    //===============================
    // types
    //===============================

    extern const int TOTAL_DEBUG;

    typedef std::map<std::string, std::string> env_type;

    //===============================
    // helpers
    //===============================

    template <typename T> bool get_param(const std::string& input, T& output)
    {
        size_t pos = input.find("=");
        if (pos != std::string::npos)
        {
            value(input.substr(pos + 1), output);
            return true;
        }
        return false;
    }

    template <typename T> bool get_args(const std::string& input, T& arg1, T& arg2)
    {
        size_t pos = input.find(",");
        if (pos != std::string::npos)
        {
            arg1 = get_value(input.substr(0, pos), arg1);
            arg2 = get_value(input.substr(pos + 1), arg2);
            return true;
        }
        return false;
    }

    template <typename T> T get_env_val(env_type& env, const std::string& key, T default_value)
    {
        if (env.end() == env.find(key)) return default_value;
        return get_value<T>(env[key]);
    }

    inline bool has_env(env_type& env, const std::string& key, const char* value)
    {
        if (env.end() == env.find(key)) return false;
        return env[key] == value;
    }

    template <typename T> bool has_env(env_type& env, const std::string& key, T value)
    {
        if (env.end() == env.find(key)) return false;
        return get_value<T>(env[key]) == value;
    }

    // is debugging turned on?
    int get_debug(env_type& env);

    //===============================
    // param parsing
    //===============================

    bool parse_option(
        env_type& args,
        const std::string& param,
        std::string expected,
        bool requires_param = false);

} // namespace maz
