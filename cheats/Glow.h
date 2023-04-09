#pragma once
#include "memory.h"

__declspec(align(16)) struct Color
{
	constexpr Color(const float r, const float g, const float b, const float a = 1.f) noexcept :
		r(r), g(g), b(b), a(a) { }

	float r, g, b, a;
};

class Glow
{
	public:
		Glow(Memory mem, auto client, auto entity, Color glowColor) {
			const auto glowObjectManager = mem.Read<std::uintptr_t>(client + offsets::dwGlowObjectManager);
			const auto glowIndex = mem.Read<std::int32_t>(entity + offsets::m_iGlowIndex);

			mem.Write<Color>(glowObjectManager + (glowIndex * 0x38) + 0x8, glowColor);

			mem.Write<bool>(glowObjectManager + (glowIndex * 0x38) + 0x28, true);
			mem.Write<bool>(glowObjectManager + (glowIndex * 0x38) + 0x29, true);
		}
};

