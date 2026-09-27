#include "SOCOM.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <unordered_set>

namespace Engine
{
	namespace zdb
	{
		namespace Tools
		{

			/* */
			bool Camera::GetCamera(Classes::CZCamera& camera)
			{

				__int64 eemem = g_Memory.GetEEMemory();
				if (!eemem)
					return false;

				auto pCamera = g_Memory.Read<__int32>(eemem + Offsets::gCamera);
				if (!pCamera || pCamera == Offsets::gCamera)
					return false;

				camera = g_Memory.Read<Classes::CZCamera>(eemem + pCamera);

				return true;
			}

			/* */
			bool Camera::GetModelMtx(Matrix4x4& ModelView)
			{
				Classes::CZCamera camera{};
				if (!GetCamera(camera))
					return false;

				ModelView = camera.m_mtxModel;

				return true;
			}

			/* */
			bool Camera::GetMtxSet(Structs::tag_ZCAM_MTX_SET& mtxSet)
			{
				Classes::CZCamera camera{};
				if (!GetCamera(camera))
					return false;

				mtxSet = camera.m_mtxSet;

				return true;
			}

			/* */
			bool Camera::GetViewport(Structs::tag_RECT& viewport)
			{

				Classes::CZCamera camera{};
				if (!GetCamera(camera))
					return false;

				viewport = camera.m_ScreenRect;

				return true;
			}

			/* */
			bool Entity::GetLocalSeal(Classes::CZSealBody& seal, i64_t* pSealAddr)
			{
				//	get eemem
				__int64 eemem = g_Memory.GetEEMemory();
				if (!eemem)
					return false;

				//	pointer to local seal
				auto pSeal = g_Memory.Read<__int32>(eemem + Offsets::gLocalSeal);
				if (!pSeal)
					return false;

				//	result
				seal = g_Memory.Read<Classes::CZSealBody>(eemem + pSeal);
				*pSealAddr = eemem + pSeal;

				return true;
			}

			/* */
			bool Entity::GetPlayers(std::vector<Classes::CZSealBody>*players)
			{

				std::vector<Classes::CZSealBody> seals;

				__int64 eemem = g_Memory.GetEEMemory();
				if (!eemem)
					return false;

				auto sealArray = g_Memory.Read<Structs::ZArray>(eemem + Offsets::gEntityArray);
				if (sealArray.count <= 1 || sealArray.begin <= 0 || sealArray.end <= 0)
					return false;


				auto it = g_Memory.Read<Structs::ZIterator>(eemem + sealArray.begin);
				auto end = g_Memory.Read<Structs::ZIterator>(eemem + it.prev);
				do
				{
					auto data = it.data;
					if (data > 0)
					{
						auto seal = g_Memory.Read<Classes::CZSealBody>(eemem + data);
						if (seal.p_name)
							seals.push_back(seal);
					}

					it = g_Memory.Read<Structs::ZIterator>(eemem + it.next);

				} while (it.data != end.data);

				*players = seals;

				return players->size() > 0;
			}

			/* */
			bool Weapon::GetWeapon(const int& weaponIndex, Classes::CZWeapon& weapon, i64_t* pWeaponAddr)
			{
				__int64 eemem = g_Memory.GetEEMemory();
				if (!eemem)
					return false;

				i64_t sealAddr = 0;
				Classes::CZSealBody czSeal;
				if (!Tools::Entity::GetLocalSeal(czSeal, &sealAddr) || !sealAddr)
					return false;

				const auto szWeaponArray = czSeal.m_kit.m_MaxWeaponIndex;
				if (weaponIndex >= szWeaponArray)
					return false;

				const auto& pBaseWeapon = sealAddr + (offsetof(Classes::CZSealBody, m_kit) + offsetof(Classes::CZKit, p_weapons[0]));
				if (pBaseWeapon <= sealAddr)
					return false;

				const auto& pWeapon = pBaseWeapon + (weaponIndex * 0x4);
				if (!pWeapon)
					return false;

				weapon = g_Memory.Read<Classes::CZWeapon>(eemem + pWeapon);
				*pWeaponAddr = eemem + pWeapon;

				return true;
			}

			/* */
			std::string Weapon::GetWeaponName(u32_t weapon)
			{
				std::string result = "";
				__int64 eemem = g_Memory.GetEEMemory();
				if (!eemem)
					return result;

				const auto& addr = eemem + weapon;
				if (addr <= weapon)
					return result;

				const auto& czWeapon = g_Memory.Read<Classes::CZWeapon>(addr);

				g_Memory.ReadString(eemem + czWeapon.p_name, result);

				return result;
			}

			/* */
			std::string Weapon::GetAmmoName(u32_t ammo)
			{
				std::string result = "";
				__int64 eemem = g_Memory.GetEEMemory();
				if (!eemem)
					return result;

				const auto& addr = eemem + (u32_t)ammo;
				if (addr <= (u32_t)ammo)
					return result;

				const auto& czAmmo = g_Memory.Read<Classes::CZAmmo>(addr);

				g_Memory.ReadString(eemem + czAmmo.p_name, result);

				return result;
			}

			Matrix4x4 Transform::BuildViewToClip(const zdb::Classes::CZCamera& camera)
			{
				/*
				
				* ViewToClip
					1.428			0				0				0
					0				1.020			0				0
					0				0				1				1
					0				0				-8.002			0

				crucial components
				key
				 - [0][0] = 457.007 / 320 = 1.428146875
				 - [1][1] = 457.007 / 448 = 1.020105
				*/

				const float viewportWidth = camera.m_ScreenRect.right - camera.m_ScreenRect.left;
				const float viewportHeight = camera.m_ScreenRect.bottom - camera.m_ScreenRect.top;

				const float viewToClip_00 = camera.m_scrZ / (viewportWidth / 2) * camera.m_ScreenAspect.x;
				const float viewToClip_11 = camera.m_scrZ / viewportHeight * camera.m_ScreenAspect.y;

				return {
					viewToClip_00, 0.0f, 0.0f, 0.0f,
					0.0f, viewToClip_11, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f, 0.0f
				};
			}

			Matrix4x4 Transform::BuildViewToScreen(const zdb::Classes::CZCamera& camera)
			{
				/*
				* ViewToScreen
					457.007			0				0				0
					0				457.007			0				0
					2048.000		2048.000		-3.722			1
					0				0				262154.900		0

				crucial components
				 - scaleX	= 457.007	[0][0]
				 - scaleY	= 457.007	[1][1]
				 - centerX	=	2048	[2][0]
				 - centerY	=	2048	[2][1]
				 - depth	= -3.722
				 - ZMapping = 262154.900

				key
				 - scale = m_scrZ * m_screenAspect
				 - center = m_screenCenter + m_screenOffset
				*/

				const float scaleX = camera.m_scrZ * camera.m_ScreenAspect.x;
				const float scaleY = camera.m_scrZ * camera.m_ScreenAspect.y;

				const float centerX = camera.m_ScreenCenter.x + camera.m_ScreenOffset.x;
				const float centerY = camera.m_ScreenCenter.y + camera.m_ScreenOffset.y;

				/*
					Depth mapping still unknown.

					These values do not affect WorldToScreen because
					ScreenSpaceToNormalized only consumes x, y, and w.
				*/
				const float depthScale = 1.f; // unknown how to obtain , seems to come from viewtoclip
				const float depthBias = camera.m_Zmax * (camera.m_CameraParams.m_near_plane/* + depthScale*/); // 65535.0000 * (4.00000000) = 262140.000

				return {
					scaleX, 0.0f, 0.0f, 0.0f,
					0.0f, scaleY, 0.0f, 0.0f,
					centerX, centerY, depthScale, 1.0f,
					0.0f, 0.0f, depthBias, 0.0f
				};
			}

			/* */
			Vec4 Transform::WorldToView(const Vec3& worldPosition, const Matrix4x4& worldToView)
			{
				return worldToView.TransformPoint(worldPosition);
			}

			/* */
			Vec3 Transform::WorldToViewFromModel(const Vec3& worldPosition, const Matrix4x4& modelMatrix)
			{
				const Vec3 cam_right{
					modelMatrix.m[0][0],
					modelMatrix.m[0][1],
					modelMatrix.m[0][2]
				};

				const Vec3 cam_up{
					modelMatrix.m[1][0],
					modelMatrix.m[1][1],
					modelMatrix.m[1][2]
				};

				const Vec3 cam_forward{
					modelMatrix.m[2][0],
					modelMatrix.m[2][1],
					modelMatrix.m[2][2]
				};

				const Vec3 cam_pos{
					modelMatrix.m[3][0],
					modelMatrix.m[3][1],
					modelMatrix.m[3][2]
				};

				Vec3 heading = worldPosition - cam_pos;

				return {
					heading.dot(cam_right),
					-heading.dot(cam_up),
					-heading.dot(cam_forward)
				};
			}

			/* */
			Vec4 Transform::ViewToScreenSpace(const Vec4& view, const Matrix4x4& viewToScreen)
			{
				return viewToScreen.TransformPoint(view);
			}

			/* */
			bool Transform::ScreenSpaceToNormalized(const Vec4& screenSpace, const Structs::tag_RECT& viewport, Vec2* out)
			{
				/* check if behind camera */
				if (screenSpace.w <= 0.001f)
					return false;

				const float x = screenSpace.x / screenSpace.w;
				const float y = screenSpace.y / screenSpace.w;

				const float width = viewport.right - viewport.left;
				const float height = viewport.bottom - viewport.top;

				if (width <= 0.0f || height <= 0.0f)
					return false;

				const Vec2 result
				{
					(x - viewport.left) / width,
					(y - viewport.top) / height
				};

				if (result.x < 0.0f || result.x > 1.0f ||
					result.y < 0.0f || result.y > 1.0f)
				{
					return false;
				}

				*out = result;
				return true;
			}

			/* */
			bool Transform::NormalizedToScreen(const Vec2& normalized, const Vec2& screenSize, Vec2* out)
			{
				Vec2 result{ normalized };
				if (result.x < 0.0f || result.x > 1.0f ||
					result.y < 0.0f || result.y > 1.0f)
				{
					return false;
				}

				*out = (result *= screenSize);

				return true;
			}

			/* */
			bool Transform::WorldToScreen(const Vec3& worldLocation, Vec2* out)
			{
				Classes::CZCamera camera;
				if (!Tools::Camera::GetCamera(camera))
					return false;

				return WorldToScreen(worldLocation, camera, out);
			}

			/* */
			bool Transform::WorldToScreen(const Vec3& worldLocation, const Vec2& szScreen, Vec2* out)
			{
				Classes::CZCamera camera;
				if (!Tools::Camera::GetCamera(camera))
					return false;

				return WorldToScreen(worldLocation, camera, szScreen, out);
			}


			/*
			 * 0 = current camera matrix
			 * 1 = previous camera update
			 * 2 = two updates ago
			 * 3 = three updates ago
			 */
			bool GetMatrixFromHistory(const zdb::Classes::CZCamera& camera, const int& index, Matrix4x4* result)
			{


				if (!result)
					return false;

				const int frameIndex = (index >= 0 && index < 4) ? index : 0;
				const Matrix4x4 currentMatrix = camera.m_mtxSet.mtxWorldToView * camera.m_mtxSet.mtxViewToScreen;

				static Matrix4x4 history[4]{};
				static Matrix4x4 previousMatrix{};
				static bool initialized = false;

				/*
				* init matrix history
				*/
				if (!initialized)
				{
					previousMatrix = currentMatrix;

					history[0] = currentMatrix;
					history[1] = currentMatrix;
					history[2] = currentMatrix;
					history[3] = currentMatrix;

					initialized = true;
				}

				/*
				 * Only advance history when the WorldToScreen matrix actually changes.
				 */
				if (std::memcmp( &currentMatrix, &previousMatrix, sizeof(Matrix4x4)) != 0)
				{
					history[3] = history[2];
					history[2] = history[1];
					history[1] = history[0];
					history[0] = currentMatrix;

					previousMatrix = currentMatrix;
				}

				*result = history[frameIndex];

				return true;
			}

			/* */
			bool Transform::WorldToScreen(const Vec3& worldLocation, zdb::Classes::CZCamera& camera, Vec2* out)
			{
				Vec4 gs;
				Matrix4x4 mtxWorldToScreen;
				if (GetMatrixFromHistory(camera, 2, &mtxWorldToScreen) == false)
				{
					/* transform world point to view space */
					Vec4 view = WorldToView(worldLocation, camera.m_mtxSet.mtxWorldToView);

					/* get native screen space (GS) */
					gs = ViewToScreenSpace(view, camera.m_mtxSet.mtxViewToScreen);

				}
				else
					gs = mtxWorldToScreen.TransformPoint(worldLocation);

				/* screen space to normalized */
				Vec2 result;
				if (ScreenSpaceToNormalized(gs, camera.m_ScreenRect, &result) == false)
					return false;

				*out = result;

				return true;
			}

			/* */
			bool Transform::WorldToScreen(const Vec3& worldLocation, zdb::Classes::CZCamera& camera, const Vec2& szScreen, Vec2* out)
			{
				Vec2 normalized{};
				if (WorldToScreen(worldLocation, camera, &normalized) == false)
					return false;

				return NormalizedToScreen(normalized, szScreen, out);
			}

			/* */
			bool Transform::Debug::ProjectWorldToScreenFromModelMtx(const Vec3& worldLocation, zdb::Classes::CZCamera& camera, const Engine::Matrix4x4& modelMatrix, const Vec2& szScreen, Vec2* out)
			{
				/* transform world point to view space */
				auto view = WorldToView(worldLocation, modelMatrix);

				/* get native screen space (GS) */
				auto gs = ViewToScreenSpace(view, camera.m_mtxSet.mtxViewToScreen);

				Vec2 normalized{};
				if (ScreenSpaceToNormalized(gs, camera.m_ScreenRect, &normalized) == false)
					return false;

				return NormalizedToScreen(normalized, szScreen, out);
			}
		}
	}
}

SOCOMMemory::SOCOMMemory(const std::string& name) : SOCOMMemory(name, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ) {}

SOCOMMemory::SOCOMMemory(const std::string& name, const DWORD& access) : exMemory(), targetName(name), targetAccess(access)
{
    bAttached = false;
}

bool SOCOMMemory::Attach(const std::string& name, const DWORD& access)
{
    targetName = name;
    
	targetAccess = access;
    
	Detach();
    
	procInfo_t process{};
    if (!AttachEx(targetName, &process, access) || !process.hProc || process.hProc == INVALID_HANDLE_VALUE) 
		return false;
    
	vmProcess = process;
    
	static_cast<PROCESSINFO64&>(SocomInfo) = process;
    
	bAttached = true;
    
	printf("[memory] Attached to %s (PID %lu)\n", targetName.c_str(), process.dwPID);
    
	ResolveRdram();
    
	return true; // Process attachment and RDRAM readiness are separate states.
}

bool SOCOMMemory::Detach()
{
    DetachEx(vmProcess);
    SocomInfo = {};
    bAttached = false;
    return true;
}

bool SOCOMMemory::ResolveRdram()
{
	i64_t rdram = 0;
	i64_t runtime = 0;
    const i64_t prev = SocomInfo.dwEEBase;
	const i64_t slot = vmProcess.dwModuleBase + RuntimeForCrashRva;
    if (!bAttached) 
		return false;

	if (!ReadMemoryEx(vmProcess.hProc, slot, &runtime, sizeof(runtime)) || !runtime)
		return false;

	if (!ReadMemoryEx(vmProcess.hProc, runtime, &rdram, sizeof(rdram)) || !rdram)
		return false;

	if (prev == rdram)
		return false;

    SocomInfo.dwEEBase = rdram;

	printf("[SOCOMMemory] Runtime: EE base: 0x%llX\n", rdram);

    return true;
}

void SOCOMMemory::update()
{
	/* @TODO: maybe set a flag when a read fails , then perform an update. works for now */

    const auto now = GetTickCount64();
    
	if (lastRefresh && now - lastRefresh < 1000) 
		return;
    
	lastRefresh = now;
    
	if (bAttached)
	{
		DWORD code = 0;
		if (!GetExitCodeProcess(vmProcess.hProc, &code) || code != STILL_ACTIVE)
		{
			Detach();
			return;
		}
	}
    
	if (!bAttached)
    {
        Attach(targetName, targetAccess);
        return;
    }

	/* always resolve RDRAM */

    ResolveRdram();
    
	/* this is done in DXWindow every frame as well */

	EnumWindowData data{};
	data.procId = vmProcess.dwPID;
	EnumWindows(GetProcWindowEx, reinterpret_cast<LPARAM>(&data));
    
	SocomInfo.hWnd = vmProcess.hWnd = data.hwnd;
    
	char title[MAX_PATH]{};
    if (data.hwnd && GetWindowTextA(data.hwnd, title, MAX_PATH)) 
		SocomInfo.mWndwTitle = title;
}

void SOCOM::Update()
{
	using namespace Engine::zdb;

	static auto reset = [this](const char* reason) 
	{ 
		{
			std::lock_guard<std::mutex> lock(this->m_cacheMutex);
			this->m_cache = SGlobalSnapshot();
		} // free lock

		static std::string lastReason;
		if (lastReason == reason)
			return;

        printf("[!] SOCOM::Update - reset `%s`\n", reason);

        lastReason = reason;
	};

	SGlobalSnapshot globals;
	auto& game = globals.m_ctx;
	auto& player = globals.m_localPlayer;

	globals.m_EE = g_Memory.GetEEMemory();
	if (!globals.m_EE)
		return reset("failed to obtain eemem");

	//	GET LOCAL PLAYER
	u32_t pLocalPlayer = g_Memory.Read<_int32>(globals.m_EE + Offsets::gLocalSeal);
	if (!pLocalPlayer)
		return reset("failed to obtain local player");

	Classes::CZSealBody localSeal = g_Memory.Read<Classes::CZSealBody>(pLocalPlayer);

	player.m_RVA = pLocalPlayer;
	player.m_pos = localSeal.m_wsOrigin;
	player.m_seal = localSeal;
	if (!g_Memory.ReadString(globals.m_EE + localSeal.p_name, player.m_name, 32))
		return reset("failed to read local player name");

	//	GET PLAYERS
	std::vector<SImGuiPlayer> imPlayers;
	std::vector<Classes::CZSealBody> seals;
	if (Tools::Entity::GetPlayers(&seals))
	{
		imPlayers.reserve(seals.size());
		for (auto& ent : seals)
		{
			SImGuiPlayer imPlayer;

			if (ent.p_name == localSeal.p_name)
				continue;	//	skip local player

			if (!g_Memory.ReadString(globals.m_EE + ent.p_name, imPlayer.m_name, 32))
				continue;

			imPlayer.m_pos = ent.m_wsOrigin;
			imPlayer.m_health = ent.m_health * 100.f;
			imPlayer.m_bAlive = (imPlayer.m_health > 0.f);
			imPlayer.m_stance = ent.m_stance;
			imPlayer.m_class = ent;

			imPlayers.push_back(imPlayer);
		}
	}

	game.m_bInGame = seals.size() > 1;
	game.m_playerCount = imPlayers.size();
	globals.m_players = std::move(imPlayers);

	// GET CAMERA
	if (!Tools::Camera::GetCamera(globals.m_camera))
		return reset("failed to obtain camera");

	globals.m_bValid = true;
	{
		std::lock_guard<std::mutex> lock(this->m_cacheMutex);
		this->m_cache = std::move(globals);
	} // free lock
}

void SOCOM::ShutDown()
{
	// disable any enabled patches

	g_Memory.Detach();
}