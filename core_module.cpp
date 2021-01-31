#include <fstream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <filesystem>

#include <xxhash.h>

#include "manager.h"
#include "core_module.h"
#include "sol_check.h"
#include "logging.h"
#include "sysinfo.h"

namespace rostrum {
  namespace {
    using namespace std::chrono;
    const auto begin_execution_clock = steady_clock::now();

    auto get_elapsed_time() {
      const auto now = steady_clock::now();
      const auto span = duration_cast<duration<double>>(now - begin_execution_clock);
      return span.count();
    }

    auto load_lua_libs(const sol::variadic_args va) {
      auto&& state = va.lua_state();
      for (const auto& v : va) {
        manager::get_instance().imbue_lua_lib(state, v.as<sol::lib>());
      }
    }

    auto load_file_whash(const sol::this_state& state, const char * const filename) {
      sol::state_view lua = state;

      std::ifstream in(filename);
      if (!in) {
        throw std::runtime_error(std::string(filename) + " not found");
      }
      const std::string buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

      const auto script = sol_check(lua.load(buffer, std::string("@") + filename));
      const auto f = script.get<sol::protected_function>();
      const auto hash = XXH32(buffer.c_str(), std::size(buffer), 0);

      return std::make_tuple(f, hash);
    }
  }

  void log_with_info(const sol::this_state& state, void(*logf)(const std::string&), const std::string& msg) {
    static lua_Debug dbg;
    assert(lua_getstack(state, 1, &dbg));
    assert(lua_getinfo(state, "nSl", &dbg));

    // using custom formatter flag is impossible/too heavy with async logger
    logf(fmt::format("[{}:{}:{}] {}", dbg.short_src, dbg.name, dbg.currentline, msg));
  }

  void reroute_log(const std::string& path) {
    const auto& logger = spdlog::get("default");

    // rename temp file to @path
    for (const auto& sink : logger->sinks()) {
      auto sink_inst = std::dynamic_pointer_cast<logging::detail::temp_file_sink_mt>(sink);
      if (sink_inst) {
        sink_inst->rename(path);
        break;
      }
    }
    std::filesystem::path file_path = path;
  }

  void set_log_level(const std::string& level) {
    auto enum_lvl = spdlog::level::off;

    if (level == "trace") {
      enum_lvl = spdlog::level::trace;
    }
    else if (level == "debug") {
      enum_lvl = spdlog::level::debug;
    }
    else if (level == "info") {
      enum_lvl = spdlog::level::info;
    }
    else if (level == "warn") {
      enum_lvl = spdlog::level::warn;
    }
    else if (level == "err") {
      enum_lvl = spdlog::level::err;
    }
    else if (level == "critical") {
      enum_lvl = spdlog::level::critical;
    }
    else {
      throw std::runtime_error(std::string("unsupported log level specified: ") + level);
    }

    spdlog::get("default")->set_level(enum_lvl);
  }

  sol::table imbue_core(sol::state_view& lua) {
    // TODO: constexpr parse for k/v
    auto core_table = lua.create_table();
    core_table.new_enum("lib",
      "base", sol::lib::base,
      "package", sol::lib::package,
      "coroutine", sol::lib::coroutine,
      "string", sol::lib::string,
      "os", sol::lib::os,
      "math", sol::lib::math,
      "table", sol::lib::table,
      "debug", sol::lib::debug,
      "bit32", sol::lib::bit32,
      "io", sol::lib::io,
      "ffi", sol::lib::ffi,
      "jit", sol::lib::jit,
      "utf8", sol::lib::utf8);

    core_table.set_function("get_elapsed_time", get_elapsed_time);
    core_table.set_function("load_lua_libs", load_lua_libs);
    core_table.set_function("load_file_whash", load_file_whash);
    core_table.set_function("set_log_level", set_log_level);
    core_table.set_function("reroute_log", reroute_log);
    core_table.set_function("print_system_info", [] { spdlog::debug(sysinfo::get_sys_info()); });

    spdlog::debug("imbuing lua state with core functions: get_elapsed_time,load_lua_libs,load_file_whash,set_log_level,reroute_log,print_system_info");

    // logging stuff
    static auto logger = spdlog::get("default");
    static auto trace = [](const std::string& msg) { logger->trace(msg); };
    static auto debug = [](const std::string& msg) { logger->debug(msg); };
    static auto info = [](const std::string& msg) { logger->info(msg); };
    static auto warn = [](const std::string& msg) { logger->warn(msg); };
    static auto error = [](const std::string& msg) { logger->error(msg); };

    auto log_trace = [](const sol::this_state& state, const std::string& msg) { log_with_info(state, trace, msg); };
    auto log_debug = [](const sol::this_state& state, const std::string& msg) { log_with_info(state, debug, msg); };
    auto log_info = [](const sol::this_state& state, const std::string& msg) { log_with_info(state, info, msg); };
    auto log_warn = [](const sol::this_state& state, const std::string& msg) { log_with_info(state, warn, msg); };
    auto log_error = [](const sol::this_state& state, const std::string& msg) { log_with_info(state, error, msg); };

    core_table.set_function("log_trace", log_trace);
    core_table.set_function("log_debug", log_debug);
    core_table.set_function("log_info", log_info);
    core_table.set_function("log_warn", log_warn);
    core_table.set_function("log_error", log_error);

    spdlog::debug("imbuing lua state with core functions: log_trace,log_debug,log_info,log_warn,log_error");

    return core_table;
  }
}