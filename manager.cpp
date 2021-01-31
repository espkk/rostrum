#include <filesystem>
#include <vector>
#include <utility>
#include <iostream>

#include <boost/dll.hpp>

#include <spdlog/spdlog.h>

#include "manager.h"
#include "core_module.h"

namespace rostrum {
  class manager::impl {
  private:
    inline static const auto rostrum_module_ext = ".rmod";
    inline static const auto lua_module_ext = ".lmod";

    inline static const auto rostrum_folder = boost::dll::program_location().parent_path().parent_path().string();
    inline static const auto modules_dir = "\\modules";

    using lib_info = std::pair<boost::dll::shared_library, api::module_info>;
    std::vector<lib_info> libs_;

  public:
    void init_state(sol::state_view& lua) {
      // load only base and package libs by default
      lua.open_libraries(sol::lib::base, sol::lib::package);
      spdlog::debug("imbuing lua state with lib::base | lib::package");

      // set CPATH to modules/?.lmod
      const auto cpath = rostrum_folder + modules_dir + +"\\?" + lua_module_ext;
      lua["package"]["cpath"] = cpath;
      spdlog::debug("setting CPATH to '{}'", cpath);

      spdlog::debug("adding rostrum package loader", cpath);
      // specify package loader for rostrum modules
      lua.add_package_loader(+[](lua_State* L) {
        const auto path = sol::stack::get<std::string_view>(L, 1);
        if (std::size(path) > 0 && path[0] == ':') {
          sol::stack::push(L, +[](lua_State* L) {
            const std::string_view path = sol::stack::get<std::string_view>(L, 1).substr(1);
            spdlog::debug("rostrum package loader: requested '{}'", path);

            // load internal core or external rostrum module into lua state
            sol::state_view lua(L);
            auto&& t = (path == "core") ? imbue_core(lua) : get_instance().get(path).imbue(lua);

            sol::stack::push(L, t);
            return 1;
          });
          return 1;
        }
        return 0;
      });
    }

    void reload_rostrum_modules() {
      namespace fs = std::filesystem;
      using api::module_info, api::query_info_ptr;

      // TODO: figure out if sol3/lua terminates it state when unloading library
      libs_.clear();

      for (const auto& module : fs::directory_iterator(rostrum_folder + modules_dir)) {
        try {
          const auto& path = module.path();

          // filter lua modules
          if (path.extension() != rostrum_module_ext) {
            continue;
          }

          // load module and query info
          auto& [name, info] = libs_.emplace_back(std::make_pair(path.string(), module_info{}));
          spdlog::debug("loaded rostrum module '{}'", path.string());
          const auto& query_info = name.get_alias<query_info_ptr>("__rostrum_query_info");
          spdlog::debug("loading __rostrum_query_info...");
          query_info(info);
          spdlog::debug("querying...");
        }
        catch (const std::exception& e) {
          spdlog::error("failed to load {}: {}", module.path().string(), e.what());
        }
        catch (...) {
          spdlog::error("system error while loading {}", module.path().string());
          throw;
        }
      }
    }

    api::module_info get(const std::string_view name) {
      for (const auto& [_, info] : libs_) {
        if (name == static_cast<const char*>(std::data(info.name))) {
          return info;
        }
      }

      const std::string name_string = std::data(name);
      throw std::runtime_error("module " + name_string + " not found");
    }

    static void imbue_lua_lib(sol::state_view& lua, const sol::lib lib) {
      lua.open_libraries(lib);
    }
  };

  manager::manager() : impl_(std::make_unique<impl>()) {
  }

  manager::~manager() = default;

  manager& manager::get_instance() {
    static manager instance;
    return instance;
  }

  void manager::init_state(sol::state_view& lua) const {
    impl_->init_state(lua);
  }

  void manager::reload_rostrum_modules() const {
    impl_->reload_rostrum_modules();
  }

  api::module_info manager::get(const std::string_view name) const {
    return impl_->get(name);
  }

  void manager::imbue_lua_lib(sol::state_view&& lua, const sol::lib lib) const {
    impl_->imbue_lua_lib(lua, lib);
  }
}
