#pragma once
#include <infoware/infoware.hpp>

#include <string>
#include <sstream>

namespace rostrum::sysinfo {

	inline std::string get_sys_info() {
		std::stringstream info;

		const char* cache_type_name(const iware::cpu::cache_type_t cache_type) noexcept;
		const char* architecture_name(const iware::cpu::architecture_t architecture) noexcept;
		const char* endianness_name(const iware::cpu::endianness_t endianness) noexcept;

		const auto os_info = iware::system::OS_info();
		const auto memory = iware::system::memory();
		const auto quantities = iware::cpu::quantities();

		info << "System information:\n"
			"  OS:\n"
			<< "    Name     : " << os_info.name << '\n'
			<< "    Full name: " << os_info.full_name << '\n'
			<< "    Version  : " << os_info.major << '.' << os_info.minor << '.' << os_info.patch << " build " << os_info.build_number << '\n'
			<< "  Memory:\n"
			"    Physical:\n"
			<< "      Available: " << memory.physical_available << "B\n"
			<< "      Total    : " << memory.physical_total << "B\n"
			<< "    Virtual:\n"
			<< "      Available: " << memory.virtual_available << "B\n"
			<< "      Total    : " << memory.virtual_total << "B\n" <<
			"  CPU:\n"
			<< "    Architecture: " << architecture_name(iware::cpu::architecture()) << '\n'
			<< "    Frequency: " << iware::cpu::frequency() << " Hz\n"
			<< "    Endianness: " << endianness_name(iware::cpu::endianness()) << '\n'
			<< "    Model name: " << iware::cpu::model_name() << '\n'
			<< "    Vendor ID: " << iware::cpu::vendor_id() << '\n'
			<< "  Quantities:\n"
			<< "    Logical CPUs : " << quantities.logical << '\n'
			<< "    Physical CPUs: " << quantities.physical << '\n'
			<< "    CPU packages : " << quantities.packages << '\n'
			<< "  Caches:\n";


		for (auto i = 1u; i <= 3; ++i) {
			const auto cache = iware::cpu::cache(i);
			info << "    L" << i << ":\n"
				<< "      Size         : " << cache.size << "B\n"
				<< "      Line size    : " << cache.line_size << "B\n"
				<< "      Associativity: " << static_cast<unsigned int>(cache.associativity) << '\n'
				<< "      Type         : " << cache_type_name(cache.type) << '\n';
		}

		info << std::boolalpha
			<< "  Instruction set support:\n";
		for (const auto& [instruction_set, is_supported] : { std::make_pair("3D-now!", iware::cpu::instruction_set_t::s3d_now),
												std::make_pair("MMX    ", iware::cpu::instruction_set_t::mmx),
												std::make_pair("SSE    ", iware::cpu::instruction_set_t::sse),
												std::make_pair("SSE2   ", iware::cpu::instruction_set_t::sse2),
												std::make_pair("SSE3   ", iware::cpu::instruction_set_t::sse3),
												std::make_pair("AVX    ", iware::cpu::instruction_set_t::avx) }) {
			info << "    " << instruction_set << ": " << instruction_set_supported(is_supported) << '\n';
		}

		return info.str();
	}

}