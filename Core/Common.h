#ifndef COMMON_H_INC
#define COMMON_H_INC

#include <exception>

__forceinline __declspec(naked) static UINT64 rdtsc()
{ __asm
  { 
    rdtsc
    ret
  }
}

class already_exists : std::exception { };

#endif // COMMON_H_INC
