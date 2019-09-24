#include <algorithm>
#include <fstream>
#include <iostream>

#include "maz-utils/fs.h"
#include "maz-utils/utils.h"

namespace maz {
    namespace fs {

        std::string path::basename(const std::string& file)
        {
            int slash_pos_max = -1;
            for (auto slash : {path::FS, path::OTHER_FS})
            {
                size_t slash_pos = file.find_last_of(slash);
                if (std::string::npos != slash_pos)
                {
                    slash_pos_max = maz_max(slash_pos_max, static_cast<int>(slash_pos));
                }
            }

            if (slash_pos_max != -1)
            {
                return file.substr(slash_pos_max + 1);
            }

            return file;
        }

        std::string path::dirname(const std::string& file)
        {
            int slash_pos_max = -1;
            for (auto slash : {path::FS, path::OTHER_FS})
            {
                size_t slash_pos = file.find_last_of(slash);
                if (std::string::npos != slash_pos)
                {
                    slash_pos_max = maz_max(slash_pos_max, static_cast<int>(slash_pos));
                }
            }

            if (slash_pos_max != -1)
            {
                return file.substr(0, slash_pos_max);
            }

            return file;
        }

        std::string path::date_dir()
        {
            std::ostringstream oss;
            char date[24] = {0};

            time_t rawtime;
            time(&rawtime);
            struct tm* timeinfo = localtime(&rawtime);
            strftime(date, sizeof(date), "%Y_%m_%d__%H.%M.%S", timeinfo);

            oss << date;
            return oss.str();
        }

        bool path::exists(const std::string& name)
        {
            std::ifstream f(name.c_str());
            return f.good();
        }

        const std::string path::FS("/");
        const std::string path::OTHER_FS("\\");

        maz::json file::read_json(const std::string& file_name)
        {
            std::ifstream iff;
            iff.open(file_name.c_str(), std::ios::binary);
            if (!iff.good()) throw std::runtime_error("Cannot load json");

            maz::json js;
            try
            {
                iff >> js;
            } catch (std::exception&)
            {
                throw;
            }
            return js;
        }

        std::string file::read_text(const std::string& file_name)
        {
            std::ifstream iff;
            iff.open(file_name.c_str(), std::ios::binary);
            if (!iff.good())
            {
                throw std::runtime_error("Cannot load text");
            }
            return std::string(
                (std::istreambuf_iterator<char>(iff)), std::istreambuf_iterator<char>());
        }

        void file::write(const std::string& file_name, const maz::json& data, int indent)
        {
            std::ofstream off;
            off.open(file_name);
            if (-1 == indent)
            {
                off << data;
            } else
            {
                off << data.dump(indent);
            }
            off.close();
        }

        std::string file::text2filename(std::string t)
        {
            std::replace(t.begin(), t.end(), '/', '_');
            std::replace(t.begin(), t.end(), ':', '_');
            return t;
        }

    } // namespace fs
} // namespace maz
