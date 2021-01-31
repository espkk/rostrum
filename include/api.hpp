#pragma once
#include <array>
#include <cstdint>

#include <boost/dll.hpp>

#pragma comment(lib, "lua51")
extern "C" {
	#include <luajit/luajit.h>
	#include <luajit/lualib.h>
	#include <luajit/lauxlib.h>
}
/* we always want to be safe */
#define SOL_ALL_SAFETIES_ON 1
#define SOL_NO_LUA_HPP 1
#define SOL_USING_CXX_LUAJIT 1
#include <sol/sol.hpp>

namespace rostrum::api
{
	namespace details {
		struct version {
			uint32_t major;
			uint32_t minor;
		};
	}

	using api_version = details::version;
	using module_version = details::version;
	using module_name = std::array<char, 12>;
	using module_description = std::array<char, 52>;
	
	constexpr api_version kRostrumApiVersion = { 0,1 };

	class module_info {
	public:
		using imbue_lua_ptr = sol::table(sol::state_view & lua);
		
		module_info() = default;
		module_info(const module_name& name, const module_description& description, const module_version& version, imbue_lua_ptr imbue) :
			apiVersion(kRostrumApiVersion), name(name), description(description), version(version), imbue(imbue) { }

		api_version apiVersion;
		module_name name;
		module_description description;
		module_version version;
		imbue_lua_ptr * imbue;
	};

	using query_info_ptr = void(rostrum::api::module_info &) noexcept;

	#define DECLARE_MODULE_INTERFACE(name, description, version_major, version_minor, imbue_ptr) \
		void query_info(rostrum::api::module_info& module_info_ptr) noexcept { \
			module_info_ptr = rostrum::api::module_info{ {name}, {description}, rostrum::api::module_version{version_major, version_minor}, imbue_ptr }; \
		} \
		BOOST_DLL_ALIAS(query_info, __rostrum_query_info)
}