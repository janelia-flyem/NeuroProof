#ifndef SCOPETIME_H
#define SCOPETIME_H

#include <time.h>
#include <iostream>

namespace NeuroProof {

class ScopeTime {
  public:
    ScopeTime(bool debug_ = true) : debug(debug_)
    {
        initial_time = clock();
    }
    ~ScopeTime()
    {
        if (debug) {
            std::cout << "Time Elapsed: " << getElapsed() << " seconds" << std::endl;
        }
    }
    double getElapsed()
    {
        return (clock() - initial_time) / double(CLOCKS_PER_SEC);
    }
  private:
    clock_t initial_time;
    bool debug;
};

}

#endif
