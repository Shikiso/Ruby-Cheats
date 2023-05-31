#include "hacks.h"
#include "globals.h"
#include "vector.h"
#include "gui.h"

#include <thread>
#include <array>
#include <iostream>

struct GlowColor {
	constexpr GlowColor(float r, float g, float b, float a = 1.f) noexcept :
		r(r), g(g), b(b), a(a) { }

	float r, g, b, a;
};

struct CharmColor {
	std::uint8_t r{ }, g{ }, b{ };
};

void writeGlow(auto glowManager, const Memory& mem, auto player, GlowColor colour) {
	const auto glowIndex = mem.Read<std::int32_t>(player + offsets::m_iGlowIndex);

	mem.Write(glowManager + (glowIndex * 0x38) + 0x8, colour);

	mem.Write(glowManager + (glowIndex * 0x38) + 0x27, true);
	mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);
}

void hacks::VisualsThread(const Memory& mem) noexcept {
	auto enemyColor = GlowColor{ globals::glowColor[0], globals::glowColor[1] , globals::glowColor[2] };
	auto teamColor = GlowColor{ globals::teamGlowColor[0], globals::teamGlowColor[1] , globals::teamGlowColor[2] };

	constexpr auto teamCharmColour = CharmColor{ 0, 255, 0 };
	constexpr auto enemyCharmColour = CharmColor{ 255, 0, 0 };

	int flashDur = 0;

	while (gui::isRunning) {
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

		enemyColor = GlowColor{ globals::glowColor[0], globals::glowColor[1] , globals::glowColor[2] };
		teamColor = GlowColor{ globals::teamGlowColor[0], globals::teamGlowColor[1] , globals::teamGlowColor[2] };

		if (!localPlayer)
		{
			continue;
		}

		const auto glowManager = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwGlowObjectManager);

		if (!glowManager)
		{
			continue;
		}

		flashDur = mem.Read<int>(localPlayer + offsets::m_flFlashDuration);
		if (flashDur > 0 && globals::noFlash) {
			mem.Write(localPlayer + offsets::m_flFlashDuration, 0);
		}

		const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

		for (auto i = 1; i <= 32; ++i) {
			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

			if (!player)
			{
				continue;
			}

			const auto team = mem.Read<std::int32_t>(player + offsets::m_iTeamNum);

			const auto lifeState = mem.Read<std::int32_t>(player + offsets::m_lifeState);

			if (lifeState != 0)
			{
				continue;
			}

			if (team == localTeam && globals::teamGlow) {
				writeGlow(glowManager, mem, player, teamColor);
			}

			if (globals::glow && team != localTeam)
			{
				writeGlow(glowManager, mem, player, enemyColor);
			}

			if (globals::charms && team == localTeam) {
				mem.Write<CharmColor>(player + offsets::m_clrRender, teamCharmColour);
			}
			else if (globals::charms) {
				mem.Write(player + offsets::m_clrRender, enemyCharmColour);
			}

			if (globals::radar) {
				mem.Write(player + offsets::m_bSpotted, true);
			}
		}
	}
}

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}


void hacks::AimbotThread(const Memory& mem) noexcept {
	while (gui::isRunning) {
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		if (GetAsyncKeyState(VK_F1) & 1) {
			globals::aimbot = !globals::aimbot;
		}

		if (!GetAsyncKeyState(VK_MENU)) {
			continue;
		}

		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

		const auto localEyePosition = mem.Read<Vector3>(localPlayer + offsets::m_vecOrigin) +
			mem.Read<Vector3>(localPlayer + offsets::m_vecViewOffset);

		const auto clientState = mem.Read<std::uintptr_t>(globals::engineAddress + offsets::dwClientState);

		const auto viewAngles = mem.Read<Vector3>(clientState + offsets::dwClientState_ViewAngles);
		const auto aimPunch = mem.Read<Vector3>(localPlayer + offsets::m_aimPunchAngle) * 2;

		auto bestFov = 5.f;
		auto bestAngle = Vector3{ };

		for (auto i = 1; i <= 32; ++i) {
			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);
			const auto team = mem.Read<std::int32_t>(player + offsets::m_iTeamNum);
			const auto lifeState = mem.Read<std::int32_t>(player + offsets::m_lifeState);

			if (!player || team == localTeam || lifeState != 0 || mem.Read<bool>(player + offsets::m_bDormant)) {
				continue;
			}

			const auto boneMatrix = mem.Read<std::uintptr_t>(player + offsets::m_dwBoneMatrix);

			if (globals::inView && !mem.Read<bool>(player + offsets::m_bSpottedByMask)) {
				continue;
			}

			const auto playerHeadPosition = Vector3{
				mem.Read<float>(boneMatrix + 0x30 * 8 + 0x0C),
				mem.Read<float>(boneMatrix + 0x30 * 8 + 0x1C),
				mem.Read<float>(boneMatrix + 0x30 * 8 + 0x2C)
			};

			const auto angle = CalculateAngle(
				localEyePosition,
				playerHeadPosition,
				viewAngles + aimPunch
			);

			const auto fov = std::hypot(angle.x, angle.y);

			if (fov < bestFov) {
				bestFov = fov;
				bestAngle = angle;
			}
		}

		if (!bestAngle.IsZero() && globals::aimbot) {
			if (globals::aimbot_smoothness_on) {
				mem.Write<Vector3>(clientState + offsets::dwClientState_ViewAngles, viewAngles + bestAngle / globals::aimbot_smoothness);
			}
			else {
				mem.Write<Vector3>(clientState + offsets::dwClientState_ViewAngles, viewAngles + bestAngle);
			}
		}
	}
}

void hacks::PlayerThread(const Memory& mem) noexcept {
	while (gui::isRunning) {
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

		if (!localPlayer) {
			continue;
		}

		if (globals::bhop) {
			const auto onGround = mem.Read<bool>(localPlayer + offsets::m_fFlags);

			if (GetAsyncKeyState(VK_SPACE) && onGround & (1 << 0)) {
				mem.Write(globals::clientAddress + offsets::dwForceJump, 6);
			}
		}
	}
}

void hacks::TriggerbotThread(const Memory& mem) noexcept {
	while (gui::isRunning) {
		std::this_thread::sleep_for(std::chrono::microseconds(1));

		if (GetAsyncKeyState(VK_F2) & 1) {
			globals::triggerbot = !globals::triggerbot;
		}

		if (!globals::triggerbot) {
			continue;
		}
		if (!GetAsyncKeyState(VK_SHIFT) && !globals::constantTriggerBot) {
			continue;
		}

		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		const auto localHealth = mem.Read<std::uintptr_t>(localPlayer + offsets::m_iHealth);

		if (!localHealth) {
			continue;
		}

		const auto crosshairId = mem.Read<std::int32_t>(localPlayer + offsets::m_iCrosshairId);

		if (!crosshairId || crosshairId > 64) {
			continue;
		}

		const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + (crosshairId - 1) * 0x10);

		if (!mem.Read<std::int32_t>(player + offsets::m_iHealth)) {
			continue;
		}

		if (mem.Read<std::int32_t>(player + offsets::m_iTeamNum) ==
			mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum)) {
			continue;
		}

		mem.Write(globals::clientAddress + offsets::dwForceAttack, 6);
		std::this_thread::sleep_for(std::chrono::microseconds(10));
		mem.Write(globals::clientAddress + offsets::dwForceAttack, 4);
	}
}