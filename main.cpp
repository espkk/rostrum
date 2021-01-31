#include <iostream>
#include <string>

#include "exceptions.h"
#include "manager.h"
#include "sol_check.h"
#include "logging.h"

const char kUsage[] = "Usage: rostrum-host <script.rlua> [args]";

int main(const int argc, const char* const argv[]) {
  try {
    [[maybe_unused]] volatile rostrum::logging::logger_guard logger_guard;
    // system exceptions guard
    [[maybe_unused]] volatile rostrum::except::scoped_exception_guard exception_guard;
    if (argc < 2) {
      std::cerr << kUsage;
      return EXIT_FAILURE;
    }

    // parse args
    const std::string script_name = argv[1];
    std::vector<std::string> script_args;
    for (auto i = 2; i < argc; ++i) {
      script_args.emplace_back(argv[i]);
    }

    // instantiate state manager
    auto& manager = rostrum::manager::get_instance();

    // initialize lua state
    sol::state lua;
    manager.init_state(lua);

    // load rostrum modules
    manager.reload_rostrum_modules();

    // load and run script
    using rostrum::sol_check;
    auto script = sol_check(lua.load_file(script_name));
    sol_check(script(script_args));
    return EXIT_SUCCESS;
  }
  catch (const rostrum::except::system_exception& e) {
    spdlog::get("debug")->critical("Caught system_exception: {}", e.what());
    spdlog::get("default")->critical("Caught system_exception: {}", e.what());
    rostrum::except::debug_break();
  }
  catch (const sol::error& e) {
    spdlog::get("default")->critical(e.what());
  }
  catch (const std::exception& e) {
    spdlog::get("debug")->critical(e.what());
    spdlog::get("default")->critical(e.what());
    rostrum::except::debug_break();
  }
  catch (...) {
    std::cerr << "Caught unknown exception";
    rostrum::except::debug_break();
    // unwind stack
  }

  return EXIT_FAILURE;
}
