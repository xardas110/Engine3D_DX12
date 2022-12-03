#pragma once

#define DEBUG 1

#if DEBUG
#include <iostream>
#include <ostream>
#define LOG(x) std::cout << x << std::endl;
#else 
#define LOG(col, x) // LOG(x) is replaced with nothing in non-debug
#endif
