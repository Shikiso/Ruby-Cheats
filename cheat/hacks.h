#pragma once
#include "memory.h"

namespace hacks {
	void VisualsThread(const Memory& mem) noexcept;
	void AimbotThread(const Memory& mem) noexcept;
	void PlayerThread(const Memory& mem) noexcept;
	void TriggerbotThread(const Memory& mem) noexcept;
}