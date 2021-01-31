#pragma once
#include <string_view>
#include <memory>
#include "include/api.hpp"

namespace rostrum {
  class manager final {
  private:
    manager();
    ~manager();

  public:
    static manager& get_instance();

    void init_state(sol::state_view& lua) const;

    void reload_rostrum_modules() const;
    [[nodiscard]] api::module_info get(std::string_view name) const;

    void imbue_lua_lib(sol::state_view&& lua, sol::lib lib) const;

  private:
    class impl;
    std::unique_ptr<impl> impl_;
  };
}
