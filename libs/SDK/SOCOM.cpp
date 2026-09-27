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
                uint32_t address{};
                return g_Memory.ReadGuest(Offsets::gCamera, address) && address &&
                    g_Memory.ReadGuest(address, camera);
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
                if (pSealAddr) *pSealAddr = 0;
                uint32_t address{};
                if (!g_Memory.ReadGuest(Offsets::gLocalSeal, address) || !address ||
                    !g_Memory.ReadGuest(address, seal)) return false;
                // Preserve the existing output contract: a remote host address.
                if (pSealAddr) *pSealAddr = g_Memory.GuestToHost(address);
                return true;
			}

			/* */
			bool Entity::GetPlayers(std::vector<Classes::CZSealBody>*players)
			{
                if (!players) return false;
                players->clear();
                Structs::ZArray list{};
                if (!g_Memory.ReadGuest(Offsets::gEntityArray, list) ||
                    !list.count || list.count > 2048 || !list.begin || !list.end) return false;
                Structs::ZIterator first{};
                if (!g_Memory.ReadGuest(list.begin, first) || !first.prev) return false;
                // Preserve the existing begin.prev sentinel convention, but compare node
                // addresses and bound traversal rather than comparing data pointers.
                const uint32_t sentinel = first.prev & SOCOMMemory::RamMask;
                uint32_t node = list.begin;
                std::unordered_set<uint32_t> visited;
                std::vector<Classes::CZSealBody> result;
                for (uint32_t i = 0; i <= list.count; ++i)
                {
                    if (i && (node & SOCOMMemory::RamMask) == sentinel)
                    { *players = std::move(result); return !players->empty(); }
                    if (!node || i == list.count ||
                        !visited.insert(node & SOCOMMemory::RamMask).second) return false;
                    Structs::ZIterator it{};
                    if (!g_Memory.ReadGuest(node, it)) return false;
                    if (it.data)
                    {
                        Classes::CZSealBody seal{};
                        if (!g_Memory.ReadGuest(it.data, seal)) return false;
                        if (seal.p_name) result.push_back(seal);
                    }
                    node = it.next;
                }
                return false;
			}

			/* */
			bool Weapon::GetWeapon(const int& weaponIndex, Classes::CZWeapon& weapon, i64_t* pWeaponAddr)
			{
                if (pWeaponAddr) *pWeaponAddr = 0;
                Classes::CZSealBody seal{};
                if (!Entity::GetLocalSeal(seal, nullptr) || weaponIndex < 0 ||
                    weaponIndex >= 10 || weaponIndex >= seal.m_kit.m_MaxWeaponIndex) return false;
                const uint32_t address = seal.m_kit.p_weapons[weaponIndex];
                if (!address || !g_Memory.ReadGuest(address, weapon)) return false;
                if (pWeaponAddr) *pWeaponAddr = g_Memory.GuestToHost(address);
                return true;
			}

			/* */
			std::string Weapon::GetWeaponName(u32_t weapon)
			{
                std::string result;
                Classes::CZWeapon value{};
                if (weapon && g_Memory.ReadGuest(weapon, value))
                    g_Memory.ReadGuestString(value.p_Name, result);
                return result;
			}

			/* */
			std::string Weapon::GetAmmoName(u32_t ammo)
			{
                std::string result;
                Classes::CZAmmo value{};
                if (ammo && g_Memory.ReadGuest(ammo, value))
                    g_Memory.ReadGuestString(value.p_name, result);
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

// test
namespace
{
    constexpr uintptr_t SignatureOffset = 0x00180000;
    constexpr std::array<unsigned char, 24> RdramSignature = {
        0x28,0x0C,0x00,0x70, 0x28,0x14,0x00,0x70, 0x28,0x1C,0x00,0x70,
        0x28,0x24,0x00,0x70, 0x28,0x2C,0x00,0x70, 0x28,0x34,0x00,0x70
    };

    bool Readable(const MEMORY_BASIC_INFORMATION& info)
    {
        if (info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        const DWORD p = info.Protect & 0xFF;
        return p == PAGE_READONLY || p == PAGE_READWRITE || p == PAGE_WRITECOPY ||
            p == PAGE_EXECUTE_READ || p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
    }

    bool RamRange(HANDLE process, uintptr_t base)
    {
        if (!base || base > UINTPTR_MAX - SOCOMMemory::RamSize) return false;
        const uintptr_t end = base + SOCOMMemory::RamSize;
        uintptr_t allocation = 0;
        for (uintptr_t p = base; p < end;)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQueryEx(process, reinterpret_cast<void*>(p), &info, sizeof(info)) ||
                !Readable(info) || info.Type != MEM_PRIVATE) return false;
            if (!allocation) allocation = reinterpret_cast<uintptr_t>(info.AllocationBase);
            if (allocation != reinterpret_cast<uintptr_t>(info.AllocationBase)) return false;
            const DWORD protection = info.Protect & 0xFF;
            if (protection != PAGE_READWRITE && protection != PAGE_EXECUTE_READWRITE) return false;
            const uintptr_t next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
            if (next <= p) return false;
            p = next;
        }
        return true;
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
    SocomInfo.dwEEBase = 0;
    constexpr size_t ChunkSize = 1024 * 1024;
    std::vector<unsigned char> buffer(ChunkSize + RdramSignature.size() - 1);
    std::unordered_set<uintptr_t> candidates;
    size_t carry = 0;
    uintptr_t previousEnd = 0;
    MEMORY_BASIC_INFORMATION info{};
    for (uintptr_t region = 0; VirtualQueryEx(vmProcess.hProc,
        reinterpret_cast<void*>(region), &info, sizeof(info));)
    {
        const uintptr_t begin = reinterpret_cast<uintptr_t>(info.BaseAddress);
        const uintptr_t end = begin + info.RegionSize;
        if (end <= region) break;
        if (Readable(info) && info.Type == MEM_PRIVATE)
        {
            for (uintptr_t address = begin; address < end;)
            {
                if (address != previousEnd) 
					carry = 0;
                
				const size_t amount = (std::min)(ChunkSize, static_cast<size_t>(end - address));
                
				SIZE_T got = 0;
                ReadProcessMemory(vmProcess.hProc, reinterpret_cast<void*>(address),
                    buffer.data() + carry, amount, &got);

                const size_t total = carry + got;
                
				if (got && total >= RdramSignature.size())
                {
                    auto cursor = buffer.begin();
                    
					const auto finish = buffer.begin() + total;
                    
					while ((cursor = std::search(cursor, finish, RdramSignature.begin(), RdramSignature.end())) != finish)
                    {
                        const uintptr_t match = address - carry + (cursor - buffer.begin());
                        
						if (match >= SignatureOffset && RamRange(vmProcess.hProc, match - SignatureOffset))
                            candidates.insert(match - SignatureOffset);
                        
						++cursor;
                    }
                }
                carry = (std::min)(total, RdramSignature.size() - 1);
                
				if (carry) 
					std::memmove(buffer.data(), buffer.data() + total - carry, carry);
                
				previousEnd = address + got;
                
				address += got ? got : (std::min)(amount, size_t(4096));
                
				if (!got) 
					carry = 0;
            }
        }
        else 
		{ 
			carry = 0; 
			previousEnd = 0; 
		}
        region = end;
    }
    
	if (candidates.size() != 1)
    {
        printf("[memory] RDRAM candidates: %zu; waiting for one unambiguous allocation\n", candidates.size());
        return false;
    }
    
	SocomInfo.dwEEBase = *candidates.begin();
    
	printf("[memory] EE base: 0x%llX; AOB: 0x%llX\n",
        static_cast<unsigned long long>(GetEEMemory()),
        static_cast<unsigned long long>(GetEEMemory() + SignatureOffset)
	);

    return true;
}

void SOCOMMemory::update()
{
    const auto now = GetTickCount64();
    
	if (lastRefresh && now - lastRefresh < 1000) 
		return;
    
	lastRefresh = now;
    
	if (bAttached)
    {
        DWORD code = 0;
        if (!GetExitCodeProcess(vmProcess.hProc, &code) || code != STILL_ACTIVE) 
			Detach();
    }
    
	if (!bAttached)
    {
        Attach(targetName, targetAccess);
        return;
    }
    
	if (GetEEMemory())
    {
        std::array<unsigned char, RdramSignature.size()> bytes{};
        if (!ReadMemoryEx(vmProcess.hProc, GetEEMemory() + SignatureOffset, bytes.data(), bytes.size()) ||
            bytes != RdramSignature || !RamRange(vmProcess.hProc, GetEEMemory()))
            SocomInfo.dwEEBase = 0;
    }

    if (!GetEEMemory()) 
		ResolveRdram();
    
	EnumWindowData data{};
	data.procId = vmProcess.dwPID;
	EnumWindows(GetProcWindowEx, reinterpret_cast<LPARAM>(&data));
    
	SocomInfo.hWnd = vmProcess.hWnd = data.hwnd;
    
	char title[MAX_PATH]{};
    if (data.hwnd && GetWindowTextA(data.hwnd, title, MAX_PATH)) 
		SocomInfo.mWndwTitle = title;
}

bool SOCOMMemory::ReadGuestBytes(uint32_t guest, void* output, size_t size)
{
    const uint32_t offset = guest & RamMask;
    if (!IsReady() || !output || size > RamSize - offset) return false;
    return ReadMemoryEx(vmProcess.hProc, GuestToHost(guest), output, size);
}

bool SOCOMMemory::ReadGuestString(uint32_t guest, std::string& output, size_t maxLength)
{
    output.clear();
    if (!guest || !IsReady() || !maxLength || maxLength > 4096) return false;
    const size_t limit = (std::min)(maxLength, size_t(RamSize - (guest & RamMask)));
    std::string value;
    for (size_t i = 0; i < limit; ++i)
    {
        char ch{};
        if (!ReadGuest(guest + static_cast<uint32_t>(i), ch)) return false;
        if (!ch) { output = std::move(value); return true; }
        value += ch;
    }
    // A bounded, unterminated string is not a valid guest string.
    return false;
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
        if (lastReason != reason)
        {
            printf("[!] SOCOM::Update - reset `%s`\n", reason);
            lastReason = reason;
        }
	};

	SGlobalSnapshot globals;
	auto& game = globals.m_ctx;
	auto& player = globals.m_localPlayer;

	globals.m_EE = g_Memory.GetEEMemory();
	if (!globals.m_EE)
		return reset("failed to obtain eemem");

	//	GET LOCAL PLAYER
	uint32_t pLocalPlayer{};
	if (!g_Memory.ReadGuest(Offsets::gLocalSeal, pLocalPlayer) || !pLocalPlayer)
		return reset("failed to obtain local player");

	Classes::CZSealBody localSeal{};
	if (!g_Memory.ReadGuest(pLocalPlayer, localSeal))
		return reset("failed to read local player");

	player.m_RVA = pLocalPlayer;
	player.m_pos = localSeal.m_wsOrigin;
	player.m_seal = localSeal;
	if (!g_Memory.ReadGuestString(localSeal.p_name, player.m_name, 32))
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

			if (!g_Memory.ReadGuestString(ent.p_name, imPlayer.m_name, 32))
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
