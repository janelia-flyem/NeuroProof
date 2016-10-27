#ifndef ERRMSG
#define ERRMSG

#include <stdexcept>
#include <string>

namespace NeuroProof {

    // Must inherit from std::exception, otherwise boost-python
    // doesn't understand the exceptions, resulting in
    // RuntimeError: unidentifiable C++ exception
    class ErrMsg : public std::runtime_error
    {
    public:
        ErrMsg(std::string const & str_)
            : std::runtime_error(str_)
            , str(str_)
            {}
        std::string str;
    };
}

#endif
