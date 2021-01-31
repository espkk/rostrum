#include "sysinfo.h"

const char* cache_type_name(const iware::cpu::cache_type_t cache_type) noexcept {
	switch (cache_type) {
	case iware::cpu::cache_type_t::unified:
		return "Unified";
	case iware::cpu::cache_type_t::instruction:
		return "Instruction";
	case iware::cpu::cache_type_t::data:
		return "Data";
	case iware::cpu::cache_type_t::trace:
		return "Trace";
	default:
		return "Unknown";
	}
}

const char* architecture_name(const iware::cpu::architecture_t architecture) noexcept {
	switch (architecture) {
	case iware::cpu::architecture_t::x64:
		return "x64";
	case iware::cpu::architecture_t::arm:
		return "ARM";
	case iware::cpu::architecture_t::itanium:
		return "Itanium";
	case iware::cpu::architecture_t::x86:
		return "x86";
	default:
		return "Unknown";
	}
}

const char* endianness_name(const iware::cpu::endianness_t endianness) noexcept {
	switch (endianness) {
	case iware::cpu::endianness_t::little:
		return "Little-Endian";
	case iware::cpu::endianness_t::big:
		return "Big-Endian";
	default:
		return "Unknown";
	}
}