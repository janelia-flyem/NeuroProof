#ifndef ERRMSG
#define ERRMSG

#include <string>

namespace NeuroProof {
struct ErrMsg {
    ErrMsg(std::string str_) : str(str_) {}
    std::string str;
};
}

#endif
