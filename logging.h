#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
# include<spdlog/sinks/null_sink.h>
#ifdef _WIN32
# include <spdlog/sinks/msvc_sink.h>
# include <spdlog/sinks/win_eventlog_sink.h>
#endif
#if __has_include(<syslog.h>)
# include<spdlog/sinks/syslog_sink.h>
#endif

#define __STDC_WANT_LIB_EXT1__ 1
#include <cstdio>
#include <mutex>
#include <string>
#include <iostream>
#include <exception>
#include <queue>
#include <string_view>

namespace rostrum::logging {

  namespace detail {

    /*
     * Sink for writing to temp file until renamed.
     */
    template<typename Mutex>
    class temp_file_sink final : public spdlog::sinks::base_sink<Mutex> {
    public:
      temp_file_sink();
      [[nodiscard]] const spdlog::filename_t& filename() const;
      void rename(const std::string& filename);

    protected:
      void sink_it_(const spdlog::details::log_msg& msg) override;
      void flush_() override;

    private:
      spdlog::details::file_helper file_helper_;
    };

    using temp_file_sink_mt = temp_file_sink<std::mutex>;
    using temp_file_sink_st = temp_file_sink<spdlog::details::null_mutex>;

    template<typename Mutex>
    temp_file_sink<Mutex>::temp_file_sink() {
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
      char filename[MAX_PATH];
      if (tmpnam_s(filename, MAX_PATH) != 0) {
#else
      const char* filename = std::tmpnam(nullptr);
      if (filename == nullptr) {
#endif
        throw std::runtime_error("cannot create temp file for logger sink");
      }
      file_helper_.open(filename);
    }

    template<typename Mutex>
    const spdlog::filename_t& temp_file_sink<Mutex>::filename() const {
      std::scoped_lock(mutex_);
      return file_helper_.filename();
    }

    template <typename Mutex>
    void temp_file_sink<Mutex>::rename(const std::string& filename) {
      std::scoped_lock(mutex_);

      file_helper_.flush();
      file_helper_.close();

      if (std::rename(file_helper_.filename().c_str(), filename.c_str()) != 0) {
        throw std::runtime_error(std::string("cannot rename temp file for logger sink. new filename is '") + filename + '\'');
      }

      file_helper_.open(filename);
    }

    template<typename Mutex>
    void temp_file_sink<Mutex>::sink_it_(const spdlog::details::log_msg& msg) {
      std::scoped_lock(mutex_);

      spdlog::memory_buf_t formatted;
      spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
      file_helper_.write(formatted);
    }

    template<typename Mutex>
    void temp_file_sink<Mutex>::flush_() {
      file_helper_.flush();
    }

  }

  class logger_guard final {
  public:
    logger_guard(const logger_guard&) = delete;
    logger_guard(logger_guard&&) = delete;
    logger_guard& operator= (const logger_guard&) = delete;
    logger_guard& operator= (logger_guard&&) = delete;

  private:
    inline static std::once_flag init_flag_;

    static void invoke_deferred_destruction() {
      struct deferred_destructor final {
        deferred_destructor() = default;
        deferred_destructor(const deferred_destructor&) = delete;
        deferred_destructor(deferred_destructor&&) = delete;
        deferred_destructor& operator= (const deferred_destructor&) = delete;
        deferred_destructor& operator= (deferred_destructor&&) = delete;
        ~deferred_destructor() {
          spdlog::shutdown();
        }
      };
      // initialized on call
      // destructed on std::terminate
      [[maybe_unused]] static deferred_destructor destructor;
    }

    static std::vector<spdlog::sink_ptr> create_system_sinks() {
      std::vector<spdlog::sink_ptr> sinks;
#ifdef _WIN32
      sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
      sinks.push_back(std::make_shared<spdlog::sinks::win_eventlog_sink_mt>("rostrum"));
#endif
#if __has_include(<syslog.h>)
      sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>("rostrum", LOG_PERROR, LOG_USER));
#endif

      if (sinks.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
      }
      return sinks;
    }

    static void initialize_once() {
      try
      {
        spdlog::drop_all();
        spdlog::init_thread_pool(8192, 1);

        // default logger
        {
          //sinks for internal (host) and interface (lua) logging
          const auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
          console_sink->set_color(spdlog::level::trace, console_sink->BOLD);
          console_sink->set_color(spdlog::level::warn,  console_sink->YELLOW);

          const auto file_sink = std::make_shared<detail::temp_file_sink_mt>();
          std::vector<spdlog::sink_ptr> sinks{ console_sink , file_sink };

          const auto logger = std::make_shared<spdlog::async_logger>("default", sinks.begin(), sinks.end(), spdlog::thread_pool());
          logger->set_level(spdlog::level::trace);
          logger->flush_on(spdlog::level::err);
          logger->set_pattern("[%^%l%$] %v");
          register_logger(logger);
          set_default_logger(logger);
          spdlog::info("default logger set up");
        }

        // debug logger
        {
          //sinks for debugging internal(host) errors
          const auto sinks = create_system_sinks();

          const auto logger = std::make_shared<spdlog::logger>("debug", sinks.begin(), sinks.end());
          logger->set_level(spdlog::level::warn);
          register_logger(logger);
        }
      }
      catch (const spdlog::spdlog_ex& e)
      {
        std::cerr << "(CRITICAL) failed to initialize logger: " << e.what() << std::endl;
        std::terminate();
      }
    }

  public:
    logger_guard() {
      std::call_once(init_flag_, initialize_once);
    }

    ~logger_guard() {
      if (std::uncaught_exceptions() == 0) {
        spdlog::shutdown();
      }
      else {
        // still try to gracefully de-init
        invoke_deferred_destruction();
      }
    }
  };
}