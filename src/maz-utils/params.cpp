#include "maz-utils/params.h"
#include <iostream>
#include <sstream>

namespace maz {
    const int TOTAL_DEBUG = 10;

    int get_debug(env_type& env) { return get_env_val<int>(env, "debug", 0); }

    bool parse_option(
        env_type& args, const std::string& param, std::string expected, bool requires_param)
    {
        std::string expected_key = expected;
        expected = "--" + expected + (requires_param ? "=" : "");
        if (starts_with(param, expected))
        {
            std::string tmp;
            if (get_param(param, tmp))
            {
                args[expected_key] = tmp;
            } else
            {
                slogger log;
                log.error("Invalid param: ", param);
                args["help"] = "";
                throw std::runtime_error("Invalid param");
            }
            return true;
        }
        return false;
    }

} // namespace maz
