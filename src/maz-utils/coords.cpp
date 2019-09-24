#include "coords.h"
#include <fstream>
#include <sstream>

namespace maz {

    std::string serialize(const bbox_type& b)
    {
        std::ostringstream oss;
        oss << "[" << to_int(b.xlt()) << ", " << to_int(b.ylt()) << ", " << to_int(b.width())
            << ", " << to_int(b.height()) << "]";
        return oss.str();
    }

    void serialize(const char* file, const bbox_type& b)
    {
        std::ofstream off(file);
        off << serialize(b) << std::endl;
        off.close();
    }

} // namespace maz
