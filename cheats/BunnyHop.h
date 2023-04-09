#pragma once
#include <Windows.h>
#include "memory.h"
#include "offsets.h"

class BunnyHop
{
	public:
		BunnyHop(Memory mem, auto client, auto localPlayer) {
			const auto localPlayerFlags = mem.Read<std::uintptr_t>(localPlayer + offsets::m_fFlags);

			if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0) {
				if (localPlayerFlags & (1 << 0)) {
					mem.Write<std::uintptr_t>(client + offsets::dwForceJump, 6);
				}
				else {
					mem.Write<std::uintptr_t>(client + offsets::dwForceJump, 4);
				}
			}
		}
};

