#pragma once
#include "engine.h"
#include <Memory/exMemory.hpp>

#include <cstdint>
#include <type_traits>

using u32_t = uint32_t;
constexpr i64_t RuntimeForCrashRva = 0x0D7FAD18; // 
#define PLAYNAME_MAXLEN 32

struct SOCOMPROCESSINFO : public PROCESSINFO64
{
    i64_t dwEEBase{0};
};
using SOCOMInfo_t = SOCOMPROCESSINFO;

class SOCOMMemory : public exMemory
{
public:
    static constexpr u32_t RamSize = 0x02000000u;
    static constexpr u32_t RamMask = RamSize - 1;
    explicit SOCOMMemory(const std::string& name = "socom2.exe");
    SOCOMMemory(const std::string& name, const DWORD& access);
    bool Attach(const std::string& name, const DWORD& access = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ) override;
    bool Detach() override;
    void update() override;
    const SOCOMInfo_t& GetSocomInfo() const { return SocomInfo; }
	i64_t GetEEMemory() const { return SocomInfo.dwEEBase; }

private:
    bool ResolveRdram();
    SOCOMInfo_t SocomInfo{};
    std::string targetName;
    DWORD targetAccess;
    ULONGLONG lastRefresh{0};
};

namespace Engine
{
	namespace zdb
	{
		namespace Offsets
		{
			/* ALL OFFSETS ARE FROM r0001 (SCUS-972.75) */
			constexpr auto gAppCamera{ 0x415FF0 };			//	
			constexpr auto gLocalSeal{ 0x440C38 };			//	
			constexpr auto gCamera{ 0x488DE8 };				//	
			constexpr auto gEntityArray{ 0x4362E0 };		//	
		}

		namespace Enums
		{
			enum EZoomState : char
			{
				EZoomState_default = 0,
				EZoomState_1stperson = 1,
				EZoomState_unknown = 2,
				EZoomState_binocs = 3,
				EZoomState_weapondef_1 = 4
			};

			enum class ESealTeam : u32_t
			{
				ETeam_SEALS		= 0x40000001,		//	Seal
				ETeam_TERRORIST = 0x80000100,		//	Terrorist
				ETeam_TURRET	= 0x48000000,		//	Turret
				ETeam_SPECTATOR = 0x00010000,		//	Spectator
				ETeam_SP_ABLE	= 0x84000006,		//	Alpha Team
				ETeam_SP_BRAVO	= 0x8400000A,		//	Bravo Team
			};

			enum ESealStance : char
			{
				ESealStance_Stand = 0,
				ESealStance_Crouch = 1,
				ESealStance_Prone = 2
			};

			enum EWeaponIndex : __int32
			{
				EWeaponIndex_Primary = 0,
				EWeaponIndex_Secondary = 1,
				EWeaponIndex_EqSlot1 = 2,
				EWeaponIndex_EqSlot2 = 3,
				EWeaponIndex_EqSlot3 = 4
			};

			enum EWeaponFireMode : __int32
			{
				EFireMode_Default = 0,
				EFireMode_Single = 1,
				EFireMode_Burst = 2,
				EFireMode_Auto = 3
			};
		}

		namespace Structs
		{
			using namespace Enums;

			struct SCameraFrustrum
			{
				Vec3 m_frustum[3];    //0x0000
				Vec3 m_fullfrustum[6];    //0x0024
				u32_t m_full_frustum_points;    //0x006C
			};    //Size: 0x0070

			struct tag_CAMERA_PARAMS
			{
			public:
				Vec4 m_quat; //0x0000
				Vec4 m_fog_color; //0x0010
				float m_hfov; //0x0020
				float m_vfov; //0x0024
				float m_near_plane; //0x0028
				float m_mid_plane; //0x002C
				float m_far_plane; //0x0030
				Vec3 m_px_fog_color; //0x0034
				Vec3 m_nx_fog_color; //0x0040
				Vec3 m_pz_fog_color; //0x004C
				Vec3 m_nz_fog_color; //0x0058
				float m_fog_near; //0x0064
				float m_fog_far; //0x0068
				float m_fog_mid; //0x006C
				float m_fogA; //0x0070
				float m_fogB; //0x0074
				float m_fog_density; //0x0078
				float m_fog_top; //0x007C
				float m_fog_bottom; //0x0080
				float m_landmark_fog_top; //0x0084
				float m_landmark_fog_bottom; //0x0088
				unsigned int m_fog_flags; //0x008C
			}; //Size: 0x0090

			struct tag_RECT
			{
				int left; //0x0000
				int top; //0x0004
				int right; //0x0008
				int bottom; //0x000C
			}; //Size: 0x0010

			struct tag_ZCAM_MTX_SET
			{
				Matrix4x4 mtxWorldToView;    //0x0000
				Matrix4x4 mtxWorldToClip;    //0x0040
				Matrix4x4 mtxViewToClip;    //0x0080
				Matrix4x4 mtxViewToScreen;    //0x00C0
			};    //Size: 0x0100

			struct ZCAM_CLIP_DATA
			{
				Vec4 ViewPlanePoint;    //0x0000
				Vec4 NearPlanePoint;    //0x0010
				Vec4 NearPlaneNormal;    //0x0020
				Vec4 LeftPlaneNormal;    //0x0030
				Vec4 RightPlaneNormal;    //0x0040
				Vec4 TopPlaneNormal;    //0x0050
				Vec4 BottompPlaneNormal;    //0x0060
			};    //Size: 0x0070

			typedef struct SSealStats
			{
				char pad_0000[4]; //0x0000
				int16_t m_ShotsHit; //0x0004
				int16_t m_ShotsHitOnFriendly; //0x0006
				int16_t m_ShotsFired; //0x0008
				int16_t m_TimesShotAt; //0x000A
				int16_t m_Kills; //0x000C
				char pad_000E[2]; //0x000E
				int16_t m_TimesHit; //0x0010
				int16_t m_Deaths; //0x0012
				char pad_0014[22]; //0x0014
				int16_t m_GrenadesThrown; //0x002A
				int16_t m_CQCTakedowns; //0x002C
				int16_t m_PrimaryRoundsFired; //0x002E
				int16_t m_SecondaryRoundsFired; //0x0030
				char pad_0032[18]; //0x0032
				float m_accuracy; //0x0044
			}; //Size: 0x0048

			struct ZIterator
			{
				u32_t next;	//	pointer to next position in array
				u32_t prev;	//	pointer to last position in array
				u32_t data;	//	pointer to object
			};

			struct ZArray
			{
				u32_t count{ 0 };	//	objects in array
				u32_t begin{ 0 };	//	pointer to first object in the array
				u32_t end{ 0 };	//	pointer to last object in the array
			};
		}

		namespace Classes
		{
			using namespace Structs;

			class CNode
			{
			public:
				Matrix4x4 m_mtxModel; //0x0000
				AABB m_bounds; //0x0040
				uint32_t m_type; //0x0058
				uint32_t m_bits; //0x005C
				u32_t vfTable; //0x0060	; vft*
				u32_t p_parent; //0x0064	; CNode*
				char pad_0068[40]; //0x0068
				u32_t p_name; //0x0090	; txt_pointer*
				u32_t p_nodeEx; //0x0094	; CNodeEx*
				int8_t m_GlobalLighting; //0x0098
				uint8_t m_FrameRendered; //0x0099
				char pad_009A[2]; //0x009A
				float m_opacity; //0x009C
				char pad_00A0[4]; //0x00A0
				int32_t m_tickCount; //0x00A4
				char pad_00A8[8]; //0x00A8
				u32_t p_model; //0x00B0	; CModel*
				u32_t p_modelName; //0x00B4
				char pad_00B8[8]; //0x00B8
			}; //Size: 0x00C0
			static_assert(sizeof(CNode) == 0xC0);

			class CTarget
			{
			public:
				u32_t p_entity; //0x0000 : CEntity*
				Vec3 m_wsOrigin; //0x0004
				float m_distanceSq; //0x0010
				float m_distance; //0x0014
				float m_visibility; //0x0018
				float m_aware; //0x001C
				int32_t m_DiHandle; //0x0020
				char pad_0024[12]; //0x0024
			}; //Size: 0x0030
			static_assert(sizeof(CTarget) == 0x30);

			class CEntity
			{
			public:
				u32_t vfTable; //0x0000 : vft*
				char pad_0004[16]; //0x0004
				u32_t p_name; //0x0014 : char*
				uint32_t m_UnitsSeenBy; //0x0018
				Vec3 m_wsOrigin; //0x001C
				u32_t p_node; //0x0028 : CNode*
				Vec3 m_velocity[3]; //0x002C
				Vec4 m_quat; //0x0050
				Vec3 m_nextVelocity; //0x0060
				char pad_006C[4]; //0x006C
				Vec4 m_nextQuat; //0x0070
				Matrix4x4 m_mtxModel; //0x0080
				u32_t p_sealCTRL; //0x00C0 : CZSealCTRL*
				char pad_00C4[4]; //0x00C4
				int32_t m_TeamID; //0x00C8
				float m_MaxTargetRange; //0x00CC
				int32_t m_MaxTargetCount; //0x00D0
				int32_t m_TargetCount; //0x00D4
				u32_t p_TargetArray; //0x00D8 : CTarget*
				char pad_00DC[4]; //0x00DC
				uint32_t m_EntityBits; //0x00E0
				char pad_00E4[284]; //0x00E4
			}; //Size: 0x0200
			static_assert(sizeof(CEntity) == 0x200);

			class CZCamera : public CNode
			{
			public:
				class tag_CAMERA_PARAMS m_CameraParams; //0x00C0
				Vec3 m_Frustrum[3]; //0x0150
				Vec3 m_FullFrustrum[6]; //0x0174
				uint32_t m_FullFrustrumPoints; //0x01BC
				Vec2 m_sin; //0x01C0
				Vec2 m_cos; //0x01C8
				Vec2 m_tan; //0x01D0
				Vec2 m_cot; //0x01D8
				char pad_01E0[176]; //0x01E0
				float m_scrZ; //0x0290
				char pad_0294[44]; //0x0294
				uint32_t m_AboveMaterial; //0x02C0
				float m_LandmarkFarPlane; //0x02C4
				float m_RangeScale; //0x02C8
				char pad_02CC[36]; //0x02CC
				tag_ZCAM_MTX_SET m_mtxSet; //0x02F0
				uint16_t p_spr; //0x03F0 : spr_MATRIX*
				char pad_03F2[14]; //0x03F2
				ZCAM_CLIP_DATA m_ClipSet; //0x0400
				Vec2 m_ScreenAspect; //0x0470
				Vec2 m_ScreenCenter; //0x0478
				Vec2 m_ScreenOffset; //0x0480
				tag_RECT m_ScreenRect; //0x0488
				Vec2 m_ScreenContant; //0x0498
				float m_Zmin; //0x04A0
				float m_Zmax; //0x04A4
			}; //Size: 0x04A8
			static_assert(sizeof(CZCamera) == 0x4A8);

			class CAppCamera
			{
			public:
				char pad_0000[180]; //0x0000
				u32_t p_camera; //0x00B4 : CZCamera*
				char pad_00B8[4]; //0x00B8
				u32_t p_AttachedPlayer; //0x00BC : CZSealBody*
			}; //Size: 0x00C0
			static_assert(sizeof(CAppCamera) == 0xC0);

			class CZKit
			{
			public:
				uint16_t m_bits; //0x0000
				char pad_0002[22]; //0x0002
				Vec2 m_RecoilPunch; //0x0018
				Vec2 m_PrevRecoilPunch; //0x0020
				char pad_0028[8]; //0x0028
				Vec3 m_RifleKick; //0x0030
				int32_t m_RecoilKickState; //0x003C
				int32_t m_HeartbeatState; //0x0040
				Vec2 m_Heartbeat; //0x0044
				char pad_004C[4]; //0x004C
				Vec2 m_ReticleScreenOffset; //0x0050
				char pad_0058[132]; //0x0058
				u32_t p_weapons[10]; //0x00DC : CZWeapon*
				char pad_0104[80]; //0x0104
				u32_t p_AmmoTypes[10]; //0x0154 : CZAmmo*
				char pad_017C[80]; //0x017C
				int32_t m_PrimaryMags[10]; //0x01CC
				int32_t m_SecondaryMags[10]; //0x01F4
				int32_t m_EQSlot1Ammo; //0x021C
				char pad_0220[36]; //0x0220
				int32_t m_EQSlot2Ammo; //0x0244
				char pad_0248[36]; //0x0248
				int32_t m_EQSlot3Ammo; //0x026C
				char pad_0270[1036]; //0x0270
				int32_t m_PrimaryMagIndex; //0x067C
				int32_t m_SecondaryMagIndex; //0x0680
				char pad_0684[112]; //0x0684
				int32_t m_WeaponFireTypes[2]; //0x06F4
				char pad_06FC[276]; //0x06FC
				int32_t m_WeaponFireCount; //0x0810
				float m_WeaponFireDelta; //0x0814
				char pad_0818[4]; //0x0818
				int32_t m_CurrentWeaponIndex; //0x081C
				char pad_0820[4]; //0x0820
				int32_t m_MaxWeaponIndex; //0x0824
				u32_t p_SealBody; //0x0828 : CZSealBody*
				char pad_082C[100]; //0x082C
			}; //Size: 0x0890
			static_assert(sizeof(CZKit) == 0x890);

			class CZBodyPart
			{
			public:
				Vec3 m_translation; //0x0000
				u32_t p_node; //0x000C : CNode*
				Vec3 m_NextTranslation; //0x0010
				u32_t p_parent; //0x001C : CZBodyPart*
				Vec4 m_quat; //0x0020
				Vec4 m_NextQuat; //0x0030
			}; //Size: 0x0040
			static_assert(sizeof(CZBodyPart) == 0x40);

			class CZSealBody : public CEntity
			{
			public:
				int8_t m_ZoomIndex; //0x0200
				int8_t m_LastZoomIndex; //0x0201
				char pad_0202[2]; //0x0202
				float m_ZoomModifier; //0x0204
				char pad_0208[224]; //0x0208
				u32_t a_skeleton[32]; //0x02E8 : CZBodyPart*
				char pad_0368[12]; //0x0368
				ESealStance m_stance; //0x0374
				char pad_0375[3]; //0x0375
				float m_ShoulderRecoil; //0x0378
				char pad_037C[68]; //0x037C
				Vec3 m_AimWorldPosition; //0x03C0
				char pad_03CC[4]; //0x03CC
				Vec3 m_AimWorldAngles; //0x03D0
				char pad_03DC[68]; //0x03DC
				float m_lifetime; //0x0420
				char pad_0424[184]; //0x0424
				Vec3 m_RelativeRotation; //0x04DC
				char pad_04E8[60]; //0x04E8
				float m_TimeSinceLastAction; //0x0524
				char pad_0528[100]; //0x0528
				SSealStats m_stats; //0x058C
				char pad_05D4[20]; //0x05D4
				CZKit m_kit; //0x05E8
				char pad_0E78[52]; //0x0E78
				u32_t p_carry; //0x0EAC : CZSealBody*
				char pad_0EB0[404]; //0x0EB0
				float m_health; //0x1044
				char pad_1048[1016]; //0x1048
				Vec3 m_AimDir; //0x1440
				Vec3 m_AimGoal; //0x144C
				Vec3 m_AimPoint; //0x1458
				Vec3 m_ReticlePoint; //0x1464
				Vec3 m_AimNorm; //0x1470
				Vec3 m_PrevAimPoint; //0x147C
				Vec3 m_PrevAimNorm; //0x1488
				Vec3 m_PrevReticlePt; //0x1494
				Vec3 m_CurrentAimPos; //0x14A0
				char pad_14AC[4]; //0x14AC
				Vec3 m_CurrentFirePos; //0x14B0
				char pad_14BC[4]; //0x14BC
				Vec3 m_SkeletonRoot; //0x14C0
			}; //Size: 0x14CC
			static_assert(sizeof(CZSealBody) == 0x14CC);

			class CZWeapon
			{
			public:
				u32_t vfTable; //0x0000 : vft*
				u32_t p_name; //0x0004 : char*
				u32_t p_DisplayName; //0x0008 : char*
				u32_t p_TextureName; //0x000C : char*
				char pad_0010[4]; //0x0010
				u32_t p_IconName; //0x0014 : char*
				u32_t p_GearName; //0x0018 : char*
				u32_t p_ModelName; //0x001C : char*
				u32_t p_BulletImpactName; //0x0020
				int32_t m_MaxFireMode; //0x0024
				int32_t m_szMag; //0x0028
				int32_t m_defaultMags; //0x002C
				float m_SoundRadius; //0x0030
				float m_SoundRadiusSq; //0x0034
				int8_t m_encumberance; //0x0038
				char pad_0039[3]; //0x0039
				float m_MaxRange; //0x003C
				float m_EffectiveRange; //0x0040
				float m_MuzzleVelocity; //0x0044
				float m_ImpactRadius; //0x0048
				char pad_004C[4]; //0x004C
				float m_FireWait; //0x0050
				char pad_0054[16]; //0x0054
				float m_DamageMultiplier; //0x0064
				char pad_0068[92]; //0x0068
				ZArray m_LegalAmmoList; //0x00C4
				bool b_HasFireMode[4]; //0x00D0
				char pad_00D4[36]; //0x00D4
				float m_bloom; //0x00F8
				float m_RecoilResetDelay; //0x00FC
				float m_Recoil; //0x0100
				char pad_0104[44]; //0x0104
				float m_BloomResetTimer; //0x0130
			}; //Size: 0x0134
			static_assert(sizeof(CZWeapon) == 0x134);

			class CZAmmo
			{
			public:
				u32_t p_name; //0x0000 : char*
				u32_t p_DisplayName; //0x0004 : char*
				u32_t p_Name2; //0x0008 : char*
				float m_ImpactDMG; //0x000C
				float m_Stun; //0x0010
				float m_Piercing; //0x0014
				float m_ExplosionDMG; //0x0018
				float m_ExplosionRadius; //0x001C
				char pad_0020[16]; //0x0020
				int32_t m_ID; //0x0030
				char pad_0034[24]; //0x0034
			}; //Size: 0x004C
			static_assert(sizeof(CZAmmo) == 0x4C);
		}

		namespace Tools
		{
			namespace Camera
			{
				bool GetCamera(Classes::CZCamera& pCamera);
				bool GetModelMtx(Matrix4x4& ModelView);
				bool GetMtxSet(Structs::tag_ZCAM_MTX_SET& mtxSet);
				bool GetViewport(Structs::tag_RECT& viewport);
			}

			namespace Transform
			{
				Matrix4x4 BuildViewToClip(const zdb::Classes::CZCamera& camera);
				Matrix4x4 BuildViewToScreen(const zdb::Classes::CZCamera& camera);
				Vec4 WorldToView(const Vec3& worldPosition, const Matrix4x4& worldToView);
				Vec3 WorldToViewFromModel(const Vec3& worldPosition, const Matrix4x4& modelMatrix);
				Vec4 ViewToScreenSpace(const Vec4& view, const Matrix4x4& viewToScreen);
				bool ScreenSpaceToNormalized(const Vec4& screenSpace, const Structs::tag_RECT& viewport, Vec2* out);
				bool NormalizedToScreen(const Vec2& normalized, const Vec2& screenSize, Vec2* out);
				bool WorldToScreen(const Vec3& worldPosition, Vec2* out);
				bool WorldToScreen(const Vec3& worldPosition, const Vec2& szScreen, Vec2* out);
				bool WorldToScreen(const Vec3& worldPosition, zdb::Classes::CZCamera& camera, Vec2* out);
				bool WorldToScreen(const Vec3& worldPosition, zdb::Classes::CZCamera& camera, const Vec2& szScreen, Vec2* out);

				namespace Debug
				{
					bool ProjectWorldToScreenFromModelMtx(const Vec3& worldPosition, zdb::Classes::CZCamera& camera, const Matrix4x4& modelMatrix, const Vec2& szScreen, Vec2* out);
				}
			}

			namespace Entity
			{
				bool GetLocalSeal(Classes::CZSealBody& pSeal, i64_t* pAddr);
				bool GetPlayers(std::vector<Classes::CZSealBody>* players);
			}
			
			namespace Weapon
			{
				bool GetWeapon(const int& weaponIndex, Classes::CZWeapon& weapon, i64_t* pWeaponAddr);
				std::string GetWeaponName(u32_t weapon);
				std::string GetAmmoName(u32_t weapon);
			}
		}
	}
}

class SOCOM
{
public:

	struct SImGuiPlayer
	{
		std::string m_name;
		bool m_bAlive{ false };
		float m_health{ 0.0f };
		Engine::Vec3 m_pos{ 0.0f, 0.0f, 0.0f };
		Engine::zdb::Enums::ESealStance m_stance{ Engine::zdb::Enums::ESealStance::ESealStance_Stand };
		Engine::zdb::Classes::CZSealBody m_class;
	};

	struct SImGuiPickup
	{
		std::string m_name;
		Engine::Vec3 m_pos{ 0.0f, 0.0f, 0.0f };
		//Engine::zdb::Classes::CPickup m_class;
	};
	
	struct SGameContext
	{
		bool m_bInGame{ false };
		__int32 m_playerCount{ 0 };
		__int32 m_pickupCount{ 0 };
	};

	struct SLocalPlayer
	{
		__int64 m_RVA{ 0 };
		std::string m_name;
		Engine::Vec3 m_pos;
		Engine::zdb::Classes::CZSealBody m_seal;
	};

	struct SGlobalSnapshot
	{
		bool m_bValid{ false };
		_int64 m_EE{ 0 };
		SGameContext m_ctx;
		SLocalPlayer m_localPlayer;
		std::vector<SImGuiPlayer> m_players;
		std::vector<SImGuiPickup> m_pickups;
		Engine::zdb::Classes::CZCamera m_camera;
	};

public:
	std::mutex m_cacheMutex;
	SGlobalSnapshot m_cache;

public:
	void Update();
	void ShutDown();
}; 

inline SOCOMMemory g_Memory = SOCOMMemory("socom2.exe");
inline std::unique_ptr<SOCOM> g_SOCOM;