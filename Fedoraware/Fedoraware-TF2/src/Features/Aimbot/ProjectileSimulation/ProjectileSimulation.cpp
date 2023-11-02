#include "ProjectileSimulation.h"
#pragma warning (disable : 4018)
#pragma warning (disable : 4552)

IPhysicsEnvironment* env = nullptr;
IPhysicsObject* obj = nullptr;

void CProjectileSimulation::GetProjectileFireSetup(CBaseEntity* player, const Vec3& ang_in, Vec3 offset, Vec3& pos_out, Vec3& ang_out, bool pipes, bool interp)
{
	ConVar* cl_flipviewmodels = g_ConVars.cl_flipviewmodels;

	if (!cl_flipviewmodels)
		return;

	if (cl_flipviewmodels->GetBool())
		offset.y *= -1.0f;

	Vec3 forward, right, up;
	Math::AngleVectors(ang_in, &forward, &right, &up);

	Vec3 shoot_pos;
	if (!interp)
		shoot_pos = player->GetShootPos();
	else
		shoot_pos = player->GetEyePosition();

	pos_out = shoot_pos + (forward * offset.x) + (right * offset.y) + (up * offset.z);

	if (pipes)
		ang_out = ang_in;
	else
	{
		Vec3 end_pos = shoot_pos + (forward * 2000.0f);

		CGameTrace trace = {};
		CTraceFilterHitscan filter = {};
		filter.pSkip = player;
		Utils::Trace(shoot_pos, end_pos, MASK_SHOT, &filter, &trace);
		if (trace.DidHit() && trace.flFraction > 0.1f)
			end_pos = trace.vEndPos;

		Math::VectorAngles(end_pos - pos_out, ang_out);
	}
}

bool CProjectileSimulation::GetInfo(CBaseEntity* player, CBaseCombatWeapon* pWeapon, const Vec3& vAngles, ProjectileInfo& out, bool bQuick) //possibly refine values and magic numbers
{
	if (!player || !player->IsAlive() || player->IsAGhost() || player->IsTaunting() || !pWeapon)
		return false;

	ConVar* cl_flipviewmodels = g_ConVars.cl_flipviewmodels;
	if (!cl_flipviewmodels)
		return false;

	bool bDucking = player->m_fFlags() & FL_DUCKING;
	const bool bFlipped = cl_flipviewmodels->GetBool();

	Vec3 pos, ang;

	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_DIRECTHIT:
	case TF_WEAPON_ROCKETLAUNCHER:
	{
		if (pWeapon->GetItemDefIndex() == Soldier_m_TheOriginal)
			GetProjectileFireSetup(player, vAngles, { 23.5f, 0.f, bDucking ? 8.f : -3.f }, pos, ang, false, bQuick);
		else
			GetProjectileFireSetup(player, vAngles, Vec3(23.5f, 12.f, bDucking ? 8.f : -3.f), pos, ang, false, bQuick);
		out = { TF_PROJECTILE_ROCKET, pos, ang, { 0.2f, 0.2f, 0.2f /*0.f, 0.f, 0.f is the real size*/ }, bQuick ? 1081344.f : Utils::ATTRIB_HOOK_FLOAT(1100.f, "mult_projectile_speed", pWeapon, 0, true), 0.f, true };
		return true;
	}
	case TF_WEAPON_GRENADELAUNCHER:
	{
		GetProjectileFireSetup(player, vAngles, { 16.f, 8.f, -6.f }, pos, ang, true, bQuick);
		const bool is_lochnload = G::CurItemDefIndex == Demoman_m_TheLochnLoad;
		float speed = is_lochnload ? 1490.f : 1200.f;
		if (player->IsPrecisionRune())
			speed *= 2.5f;
		float lifetime = 2.2f; // estimates
		if (G::CurItemDefIndex == Demoman_m_TheIronBomber)
			lifetime *= 0.7f;
		out = { TF_PROJECTILE_PIPEBOMB, pos, ang, { 4.f, 4.f, 4.f }, speed, 1.f, is_lochnload, lifetime };
		return true;
	}
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	{
		GetProjectileFireSetup(player, vAngles, { 16.f, 8.f, -6.f }, pos, ang, true, bQuick);
		float charge = I::GlobalVars->curtime - pWeapon->GetChargeBeginTime();
		float speed = Math::RemapValClamped(charge, 0.f, Utils::ATTRIB_HOOK_FLOAT(4.f, "stickybomb_charge_rate", pWeapon, 0, true), 900.f, 2400.f);
		if (pWeapon->GetChargeBeginTime() <= 0.f)
			speed = 900.f;
		out = { TF_PROJECTILE_PIPEBOMB_REMOTE, pos, ang, { 4.f, 4.f, 4.f }, speed, 1.f, false };
		return true;
	}
	case TF_WEAPON_CANNON:
	{
		GetProjectileFireSetup(player, vAngles, { 16.f, 8.f, -6.f }, pos, ang, true, bQuick);
		float speed = 1454.f;
		if (player->IsPrecisionRune())
			speed *= 2.5f;
		float lifetime = pWeapon->GetDetonateTime() - I::GlobalVars->curtime;
		if (pWeapon->GetDetonateTime() <= 0.f)
			lifetime = 1.06f;
		out = { TF_PROJECTILE_CANNONBALL, pos, ang, { 4.f, 4.f, 4.f }, speed, 1.f, false, lifetime };
		return true;
	}
	case TF_WEAPON_FLAREGUN:
	{
		GetProjectileFireSetup(player, vAngles, { 23.5f, 12.f, bDucking ? 8.f : -3.f }, pos, ang, false, bQuick);
		out = { TF_PROJECTILE_FLARE, pos, ang, { 1.f, 1.f, 1.f }, 2000.f, 0.3f, true };
		return true;
	}
	case TF_WEAPON_RAYGUN_REVENGE:
	{
		GetProjectileFireSetup(player, vAngles, { 23.5f, 12.f, bDucking ? 8.f : -3.f }, pos, ang, false, bQuick);
		out = { TF_PROJECTILE_FLARE, pos, ang, { 1.f, 1.f, 1.f }, 3000.f, 0.45f, true };
		return true;
	}
	case TF_WEAPON_COMPOUND_BOW:
	{
		GetProjectileFireSetup(player, vAngles, { 23.5f, 8.f, -3.f }, pos, ang, false, bQuick);
		float charge = I::GlobalVars->curtime - pWeapon->GetChargeBeginTime();
		float speed = Math::RemapValClamped(charge, 0.f, 1.f, 1800.f, 2600.f);
		float gravity = Math::RemapValClamped(charge, 0.f, 1.f, 0.5f, 0.1f);
		if (pWeapon->GetChargeBeginTime() <= 0.f) {
			speed = 1800.f;
			gravity = 0.5f;
		}
		out = { TF_PROJECTILE_ARROW, pos, ang, { 1.f, 1.f, 1.f }, speed, gravity, true };
		return true;
	}
	case TF_WEAPON_CROSSBOW:
	{
		GetProjectileFireSetup(player, vAngles, { 23.5f, 8.f, -3.f }, pos, ang, false, bQuick);
		out = { TF_PROJECTILE_ARROW, pos, ang, { 3.f, 3.f, 3.f }, 2400.f, 0.2f, true };
		return true;
	}
	case TF_WEAPON_SHOTGUN_BUILDING_RESCUE:
	{
		GetProjectileFireSetup(player, vAngles, { 23.5f, 8.f, -3.f }, pos, ang, false, bQuick);
		out = { TF_PROJECTILE_BUILDING_REPAIR_BOLT, pos, ang, { 1.f, 1.f, 1.f }, 2400.f, 0.2f, true };
		return true;
	}
	case TF_WEAPON_SYRINGEGUN_MEDIC:
	{
		GetProjectileFireSetup(player, vAngles, { 16.f, 6.f, -8.f }, pos, ang, false, bQuick);
		out = { TF_PROJECTILE_SYRINGE, pos, ang, { 0.5f, 0.5f, 0.5f }, 1000.f, 0.3f, true }; 	// probably inaccurate
		return true;
	}
	case TF_WEAPON_FLAME_BALL:
	{
		GetProjectileFireSetup(player, vAngles, {70.f, bFlipped ? -7.f : 7.f /*doesn't flip*/, -9.f}, pos, ang, false, bQuick);
		out = { TF_PROJECTILE_BALLOFFIRE, pos, ang, { 1.f, 1.f, 1.f }, 3000.f, 0.f, true, 0.2f };
		// unrelated note to self: flamethrower offset { 40, 5, 0 }
		return true;
	}
	case TF_WEAPON_RAYGUN:
	case TF_WEAPON_DRG_POMSON:
	case TF_WEAPON_PARTICLE_CANNON:
	{
		GetProjectileFireSetup(player, vAngles, { 23.5f, 8.f, bDucking ? 8.f : -3.f }, pos, ang, false, bQuick);
		float speed = 1200.f;
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_PARTICLE_CANNON: speed = Utils::ATTRIB_HOOK_FLOAT(1100.f, "mult_projectile_speed", pWeapon); break;
		case TF_WEAPON_DRG_POMSON: pos.z -= 13.f; break;
		}
			pos.z -= 13.f;
		out = { TF_PROJECTILE_ENERGY_RING, pos, ang, { 1.f, 1.f, 1.f }, bQuick ? 1081344.f : speed, 0.f, true };
		return true;
	}
	case TF_WEAPON_CLEAVER:
	{
		GetProjectileFireSetup(player, vAngles, { 32.f, 0.f, -15.f }, pos, ang, true, bQuick);
		out = { TF_PROJECTILE_CLEAVER, pos, ang, { 1.5f, 1.5f, 1.5f }, 3000.f, 2.f, false };
		return true;
	}
	case TF_WEAPON_BAT_WOOD:
	case TF_WEAPON_BAT_GIFTWRAP:
	{
		auto pLocal = g_EntityCache.GetLocal();
		if (!pLocal)
			return false;

		GetProjectileFireSetup(player, vAngles, { 0.f, 0.f, 0.f }, pos, ang, true, bQuick);
		Vec3 forward; Math::AngleVectors(ang, &forward);
		pos = (bQuick ? pLocal->GetAbsOrigin() : pLocal->GetVecOrigin()) + (Vec3(0, 0, 50) + forward * 32.f) * pLocal->m_flModelScale(); // why?
		out = { TF_PROJECTILE_THROWABLE, pos, ang, { 3.f, 3.f, 3.f }, 2000.f, 1.f, false };
		return true;
	}
	case TF_WEAPON_JAR:
	case TF_WEAPON_JAR_MILK:
	{
		GetProjectileFireSetup(player, vAngles, { 16.f, 8.f, -6.f }, pos, ang, true, bQuick);
		out = { TF_PROJECTILE_JAR, pos, ang, { 1.5f, 1.5f, 1.5f }, 1000.f, 1.f, false, 2.2f };
		return true;
	}
	case TF_WEAPON_JAR_GAS:
	{
		GetProjectileFireSetup(player, vAngles, { 16.f, 8.f, -6.f }, pos, ang, true, bQuick);
		out = { TF_PROJECTILE_JAR_GAS, pos, ang, { 1.5f, 1.5f, 1.5f }, 2000.f, 1.f, false, 2.2f };
		return true;
	}
	case TF_WEAPON_GRAPPLINGHOOK:
	{ // I was bored
		GetProjectileFireSetup(player, vAngles, { 23.5f, bFlipped ? -8.f : 8.f /*doesn't flip*/, -3.f }, pos, ang, false, bQuick);
		static auto tf_grapplinghook_projectile_speed = g_ConVars.FindVar("tf_grapplinghook_projectile_speed");
		float speed = tf_grapplinghook_projectile_speed->GetFloat();
		if (player->IsAgilityRune())
		{
			switch (player->GetClassNum())
			{
				case CLASS_SOLDIER:
				case CLASS_HEAVY:
					speed = 2600.f;
				default: // That's how Volvo works
					speed = 3000.f;
			}
		}

		out = { TF_PROJECTILE_GRAPPLINGHOOK, pos, ang, { 1.f, 1.f, 1.f }, speed, 0.5f, true }; // I think this is correct
		return true;
	}
	}

	switch (pWeapon->GetItemDefIndex())
	{
	case Heavy_s_RoboSandvich:
	case Heavy_s_Sandvich:
	case Heavy_s_FestiveSandvich:
	case Heavy_s_Fishcake:
	case Heavy_s_TheDalokohsBar:
	case Heavy_s_SecondBanana:
	{
		GetProjectileFireSetup(player, vAngles, { 0.f, 0.f, -8.f }, pos, ang, true, bQuick);
		ang -= Vec3(10, 0, 0);
		out = { TF_PROJECTILE_NONE, pos, ang, bQuick ? Vec3( 4.f, 4.f, 4.f ) : Vec3( 17.f, 17.f, 17.f ), 500.f, 1.f, false, 10.f };
		return true;
	}
	}

	return false;
}

bool CProjectileSimulation::Initialize(const ProjectileInfo& info)
{
	if (!env)
		env = I::Physics->CreateEnvironment();

	if (!obj)
	{
		//it doesn't matter what the size is for non drag affected projectiles
		//pipes use the size below so it works out just fine
		auto col{ I::PhysicsCollision->BBoxToCollide({ -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f }) };

		auto params{ g_PhysDefaultObjectParams };

		params.damping = 0.f;
		params.rotdamping = 0.f;
		params.inertia = 0.f;
		params.rotInertiaLimit = 0.f;
		params.enableCollisions = false;

		obj = env->CreatePolyObject(col, 0, info.m_pos, info.m_ang, &params);

		obj->Wake();
	}

	if (!env || !obj)
		return false;

	//set position and velocity
	{
		Vec3 forward, up;

		Math::AngleVectors(info.m_ang, &forward, nullptr, &up);

		Vec3 vel = { forward * info.m_velocity };
		Vec3 ang_vel;

		switch (info.m_type)
		{
		case TF_PROJECTILE_PIPEBOMB:
		case TF_PROJECTILE_PIPEBOMB_REMOTE:
		case TF_PROJECTILE_PIPEBOMB_PRACTICE:
		case TF_PROJECTILE_CANNONBALL:
		{
			//CTFWeaponBaseGun::FirePipeBomb
			//pick your poison

			vel += up * 200.f;
			ang_vel = { 600.f, -1200.f, 0.f };

			break;
		}
		case TF_PROJECTILE_CLEAVER:
		{
			// CTFCleaver::GetVelocityVector
			vel += up * 300.f;
			// CTFProjectile_Throwable::GetAngularImpulse
			ang_vel = { 300.f, 0.f, 0.f };

			break;
		}
		case TF_PROJECTILE_THROWABLE:
		{
			// CTFBat_Wood::GetBallDynamics
			vel += up * 200.f;
			ang_vel = { 100.f, 0.f, 0.f };

			break;
		}
		case TF_PROJECTILE_JAR:
		case TF_PROJECTILE_JAR_GAS:
		{
			vel += up * 200.f;
			ang_vel = { 300.f, 0.f, 0.f };

			break;
		}
		default: break;
		}

		if (info.no_spin)
			ang_vel.Zero();

		obj->SetPosition(info.m_pos, info.m_ang, true);
		obj->SetVelocity(&vel, &ang_vel);
	}

	//set drag
	{
		float drag;
		Vec3 drag_basis;
		Vec3 ang_drag_basis;

		//these values were dumped from the server by firing the projectiles with 0 0 0 angles
		//they are calculated in CPhysicsObject::RecomputeDragBases
		switch (info.m_type)
		{
		case TF_PROJECTILE_PIPEBOMB:
		{
			drag = 1.f;
			drag_basis = { 0.003902f, 0.009962f, 0.009962f };
			ang_drag_basis = { 0.003618f, 0.001514f, 0.001514f };

			break;
		}
		case TF_PROJECTILE_PIPEBOMB_REMOTE:
		case TF_PROJECTILE_PIPEBOMB_PRACTICE:
		{
			drag = 1.f;
			drag_basis = { 0.007491f, 0.007491f, 0.007306f };
			ang_drag_basis = { 0.002777f, 0.002842f, 0.002812f };

			break;
		}
		case TF_PROJECTILE_CANNONBALL:
		{
			drag = 1.f;
			drag_basis = { 0.020971f, 0.019420f, 0.020971f };
			ang_drag_basis = { 0.012997f, 0.013496f, 0.013714f };

			break;
		}
		case TF_PROJECTILE_CLEAVER:
		{
			// guesstimate
			drag = 1.f;
			drag_basis = { 0.020971f, 0.f, 0.f };

			break;
		}
		case TF_PROJECTILE_THROWABLE:
		{
			// guesstimate
			drag = 1.f;
			drag_basis = { 0.010500f, 0.f, 0.f };

			break;
		}
		case TF_PROJECTILE_JAR:
		{
			// guesstimate (there are different drags for different models, though shouldn't matter here)
			drag = 1.f;
			drag_basis = { 0.027101f, 0.067938f, 0.f };

			break;
		}
		case TF_PROJECTILE_JAR_GAS:
		{
			// who
			drag = 1.f;
			drag_basis = { 0.027101f, 0.067938f, 0.f };

			break;
		}
		default: break;
		}

		obj->SetDragCoefficient(&drag, &drag);

		obj->m_dragBasis = drag_basis;
		obj->m_angDragBasis = ang_drag_basis;
	}

	//set env params
	{
		float max_vel = 1000000.f;
		float max_ang_vel = 1000000.f;

		//only pipes need k_flMaxVelocity and k_flMaxAngularVelocity
		switch (info.m_type)
		{
		case TF_PROJECTILE_PIPEBOMB:
		case TF_PROJECTILE_PIPEBOMB_REMOTE:
		case TF_PROJECTILE_PIPEBOMB_PRACTICE:
		case TF_PROJECTILE_CANNONBALL:
		{
			max_vel = k_flMaxVelocity;
			max_ang_vel = k_flMaxAngularVelocity;

			break;
		}
		default: break;
		}

		physics_performanceparams_t params{};
		params.Defaults();

		params.maxVelocity = max_vel;
		params.maxAngularVelocity = max_ang_vel;

		env->SetPerformanceSettings(&params);
		env->SetAirDensity(2.f);
		env->SetGravity({ 0.f, 0.f, -(800.f * info.m_gravity) });

		env->ResetSimulationClock(); //not needed?
	}

	return true;
}

void CProjectileSimulation::RunTick(ProjectileInfo& info)
{
	if (!env)
		return;

	info.PredictionLines.push_back({ GetOrigin(), Math::GetRotatedPosition(GetOrigin(), Math::VelocityToAngles(GetVelocity() * Vec3(1, 1, 0)).Length2D() + 90, Vars::Visuals::SeperatorLength.Value) });

	env->Simulate(TICK_INTERVAL);
}

Vec3 CProjectileSimulation::GetOrigin()
{
	if (!obj)
		return {};

	Vec3 out;

	obj->GetPosition(&out, nullptr);

	return out;
}

Vec3 CProjectileSimulation::GetVelocity()
{
	if (!obj)
		return {};

	Vec3 out;

	obj->GetVelocity(&out, nullptr);

	return out;
}