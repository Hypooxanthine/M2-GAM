#pragma once

#include <iostream>

#define ASSERT(x) if((x) == false)\
    {\
        std::cerr << "Assertion " << #x << "\n\tfailed in file " << __FILE__ << ":" << __LINE__ << ".\n";\
        std::exit(1);\
    }