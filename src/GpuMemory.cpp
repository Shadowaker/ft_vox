#include "../include/GpuMemory.hpp"

namespace {
	size_t g_bytes = 0;
}

void GpuMemory::adjust(long long deltaBytes) {
	long long newTotal = static_cast<long long>(g_bytes) + deltaBytes;
	g_bytes = newTotal > 0 ? static_cast<size_t>(newTotal) : 0;
}

size_t GpuMemory::total() {
	return g_bytes;
}
