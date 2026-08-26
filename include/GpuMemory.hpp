#pragma once

#include <cstddef>

// Self-tracked estimate of GPU memory this program has allocated.
namespace GpuMemory {
	// Add (positive) or remove (negative) bytes from the running total.
	void adjust(long long deltaBytes);

	// Current estimated total, in bytes.
	size_t total();
}
