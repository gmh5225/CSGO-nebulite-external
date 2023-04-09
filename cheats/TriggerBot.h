#pragma once
#include "memory.h"

class TriggerBot {
	public:
		TriggerBot(Memory mem, auto client, auto localPlayer, auto localPlayerTeam) {
			const auto& localHealth = mem.Read<std::int32_t>(localPlayer + offsets::m_iHealth);
			const auto& crosshairId = mem.Read<std::int32_t>(localPlayer + offsets::m_iCrosshairId);

			if (!crosshairId || crosshairId > 64)
				return;

			const auto& player = mem.Read<std::uintptr_t>(client + offsets::dwEntityList + (crosshairId - 1) * 0x10);

			if (!mem.Read<std::int32_t>(player + offsets::m_iHealth))
				return;

			if (mem.Read<std::int32_t>(player + offsets::m_iTeamNum) == localPlayerTeam)
				return;

			mem.Write<std::uintptr_t>(client + offsets::dwForceAttack, 6);
			std::this_thread::sleep_for(std::chrono::microseconds(500));
			mem.Write<std::uintptr_t>(client + offsets::dwForceAttack, 4);
		}
};