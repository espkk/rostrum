#pragma once
#include <exception>
#include <stdexcept>
#ifdef _WIN32
# include <tuple>
# include <array>
# include <string_view>
# include <fmt/format.h>
# include <Windows.h>
# ifndef _WIN64
#   include <errhandlingapi.h>
# endif
# include <eh.h>
#else
# include <csignal>
#endif

namespace rostrum::except {
#pragma optimize("", off)
  inline void debug_break() {
#if defined _WIN32 && not defined _WIN64
    SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
#ifdef _WIN32
    __debugbreak();
    /* to get __debugbreak prompt since windows 10 make sure that
     * REG:\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AeDebug\Auto:REG_SZ == "1"
     */
#else
    raise(SIGABRT);
#endif
  }
#pragma optimize("", on)

#ifdef _WIN32
  namespace detail {
    // exception defines parsing
    template <std::size_t N>
    constexpr auto split_args(std::string_view s) {
      std::array<std::string_view, N> arr{};

      std::size_t begin{0}, end{0};
      for (std::size_t i = 0; i < N && end != std::string_view::npos; ++i) {
        end = s.find_first_of(',', begin);
        arr[i] = s.substr(begin, end - begin);
        // on windows 'min' is a macro
        arr[i].remove_prefix((std::min)(arr[i].find_first_not_of(' '), arr[i].size()));
        begin = end + 1;
      }

      return arr;
    }

    template <std::size_t N, int ...Values, std::size_t ...I>
    constexpr auto get_array(std::array<std::string_view, N> strings, std::index_sequence<I...>) {
      return std::array<std::pair<std::string_view, int>, N>{std::make_pair(strings[I], Values)...};
    }

#define ROSTRUM_EXPAND(x) x
#define ROSTRUM_VA_ARGS_SIZE(...) std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value
#define ROSTRUM_GET_EXCEPT_ARR(...) detail::get_array<ROSTRUM_VA_ARGS_SIZE(__VA_ARGS__), __VA_ARGS__>( detail::split_args<ROSTRUM_VA_ARGS_SIZE(__VA_ARGS__)>( ROSTRUM_EXPAND( #__VA_ARGS__ ) ), std::make_index_sequence<ROSTRUM_VA_ARGS_SIZE(__VA_ARGS__)>{} )
  }

  class system_exception final : public std::exception {
  private:
    const unsigned int err_code_;
  public:
    system_exception() noexcept : system_exception{0} {
    }

    system_exception(unsigned int err_code) noexcept : err_code_{err_code} {
    }

    [[nodiscard]]
     const char * what() const noexcept override {
      const auto exceptions{
        ROSTRUM_GET_EXCEPT_ARR(
          EXCEPTION_DATATYPE_MISALIGNMENT, EXCEPTION_BREAKPOINT, EXCEPTION_SINGLE_STEP,
          EXCEPTION_ARRAY_BOUNDS_EXCEEDED, EXCEPTION_FLT_DENORMAL_OPERAND, EXCEPTION_FLT_DIVIDE_BY_ZERO,
          EXCEPTION_FLT_INEXACT_RESULT,
          EXCEPTION_FLT_INVALID_OPERATION, EXCEPTION_FLT_OVERFLOW, EXCEPTION_FLT_STACK_CHECK, EXCEPTION_FLT_UNDERFLOW,
          EXCEPTION_INT_DIVIDE_BY_ZERO,
          EXCEPTION_INT_OVERFLOW, EXCEPTION_PRIV_INSTRUCTION, EXCEPTION_IN_PAGE_ERROR, EXCEPTION_ILLEGAL_INSTRUCTION,
          EXCEPTION_NONCONTINUABLE_EXCEPTION,
          EXCEPTION_STACK_OVERFLOW, EXCEPTION_INVALID_DISPOSITION, EXCEPTION_GUARD_PAGE, EXCEPTION_INVALID_HANDLE,
          /*EXCEPTION_POSSIBLE_DEADLOCK,*/CONTROL_C_EXIT
        )
      };

      // get exception message
      std::string_view msg = "Unknown SEH exception";
      for (auto& [name, code] : exceptions) {
        if (code == err_code_) {
          msg = name;
          break;
        }
      }

      // format into char array without allocations
      constexpr size_t msg_max_size = 128;
      constexpr size_t code_max_size = 1 + 2 + 8 + 1; // ( + 0x + 00000000 + )
      static char ret[msg_max_size + code_max_size + 1];
      size_t i{ 0 };
      for (; i != std::size(msg) && i != msg_max_size; ++i) {
        ret[i] = msg[i];
      }
      fmt::format_to(&ret[i], "({:#x})", err_code_);

      return ret;
    }
  };

  class scoped_exception_guard {
  private:
    const _se_translator_function original_translator_;
  public:
    scoped_exception_guard() noexcept
      : original_translator_{
        _set_se_translator([](const unsigned int u, PEXCEPTION_POINTERS) { throw system_exception(u); })
      } {
    }

    ~scoped_exception_guard() noexcept {
      _set_se_translator(original_translator_);
    }
  };

#undef ROSTRUM_GET_EXCEPT_ARR
#undef ROSTRUM_VA_ARGS_SIZE
#undef ROSTRUM_EXPAND
#else
  struct system_exception : public std::exception {
    const char * what() const noexcept override {
      return "system_exception not implemented";
    }
    int get_code() { 
      return -1; 
    }
  };
  struct scoped_exception_guard { };
#endif
}
