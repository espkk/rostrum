#pragma once
#include <string>

#include "exceptions.h"

namespace rostrum {

/* I really want std::source_location here... */
template <typename T>
decltype(auto) sol_check(T&& result) {
    if (!result.valid()) {
      const sol::error err = result;
      throw err;
    } 
    return std::forward<T>(result); 
  }

}