#pragma once
#include <imgui/imgui.h>
#include "offsets.h"

struct Vector {
	Vector() noexcept :
		x(x), y(y), z(z) {};

	Vector(float x, float y, float z) noexcept :
		x(x), y(y), z(z) {};

	Vector& operator+(const Vector& v) noexcept {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vector& operator-(const Vector& v) noexcept {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	float x, y, z;
};

struct ViewMatrix {
	ViewMatrix() noexcept :
		data() {};

	float* operator[](int index) noexcept {
		return data[index];
	}

	const float* operator[](int index) const noexcept {
		return data[index];
	}

	float data[4][4];
};

static bool worldToScreen(const Vector& world, Vector& screen, const ViewMatrix& vm) noexcept {
	float w = vm[3][0] * world.x + vm[3][1] * world.y + vm[3][2] * world.z + vm[3][3];

	if (w < 0.001f) {
		return false;
	}

	const float x = world.x * vm[0][0] + world.y * vm[0][1] + world.z * vm[0][2] + vm[0][3];
	const float y = world.x * vm[1][0] + world.y * vm[1][1] + world.z * vm[1][2] + vm[1][3];

	w = 1.f / w;
	float nx = x * w;
	float ny = y * w;

	const ImVec2 size = ImGui::GetIO().DisplaySize;

	screen.x = (size.x * 0.5f * nx) + (nx + size.x * 0.5f);
	screen.y = -(size.y * 0.5f * ny) + (ny + size.y * 0.5f);

	return true;
}

class ESP
{
	public:
		ESP(auto mem, auto client, auto entity) {
			const auto bones = mem.Read<std::uintptr_t>(entity + offsets::m_dwBoneMatrix);
			const auto viewMatrix = mem.Read<ViewMatrix>(client + offsets::dwViewMatrix);

			if (!bones)
				return;

			Vector headPos{
				mem.Read<float>(bones + 0x30 * 8 + 0x0C),
				mem.Read<float>(bones + 0x30 * 8 + 0x1C),
				mem.Read<float>(bones + 0x30 * 8 + 0x2C)
			};

			auto feetPos = mem.Read<Vector>(entity + offsets::m_vecOrigin);

			Vector top, bottom;
			if (
				worldToScreen(headPos + Vector{ 0, 0, 11.f }, top, viewMatrix)
				&&
				worldToScreen(feetPos, bottom - Vector{ 0, 0, 9.f }, viewMatrix))
			{
				const float h = bottom.y - top.y;
				const float w = h * 0.35f;

				const auto left = top.x - w;
				const auto right = top.x + w;
				ImGui::GetBackgroundDrawList()->AddRect({ left, top.y }, { right, bottom.y }, ImColor(1.f, 1.f, 1.f));
			}
		}
};

