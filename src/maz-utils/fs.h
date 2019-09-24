#pragma once
#include "io-document/types.h"
#include "maz-utils/utils.h"

namespace maz {
    namespace fs {

        //===============================
        // path
        //===============================

        struct path
        {
            static const std::string FS;
            static const std::string OTHER_FS;

            static inline std::string join(const std::string& p1, const std::string& p2)
            {
                return p1 + (ends_with(p1, FS) ? "" : FS) + p2;
            }
            static inline std::string
            join(const std::string& p1, const std::string& p2, const std::string& p3)
            {
                return join(join(p1, p2), p3);
            }

            static std::string basename(const std::string& file);
            static std::string dirname(const std::string& file);

            static std::string date_dir();

            static bool exists(const std::string& name);

        }; // path

        //===============================
        // fs
        //===============================

        struct file
        {
            static maz::json read_json(const std::string& file_name);
            static std::string read_text(const std::string& file_name);

            static void write(const std::string& file_name, const maz::json& data, int indent = -1);

            template <typename T> static bool load(const std::string& file_name, T& in)
            {
                slogger logger;

                std::ifstream iff;
                iff.open(file_name.c_str(), std::ios::binary);
                if (!iff.good())
                {
                    logger.error("invalid json file", file_name);
                    return false;
                }

                try
                {
                    iff >> in;
                    iff.close();
                } catch (std::exception& e)
                {
                    logger.error(e.what());
                    return false;
                }

                return true;
            }

            static std::string text2filename(std::string t);

        }; // file

    } // namespace fs
} // namespace maz