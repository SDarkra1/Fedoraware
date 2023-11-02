#include "AutoUber.h"

#include "../../Vars.h"
#include "../AutoGlobal/AutoGlobal.h"

// This code is terrible and unoptimized

constexpr static int CHANGE_TIMER = 5; // i am lazy to change code, this should be fine.

int vaccChangeState = 0;
int vaccChangeTicks = 0;
int vaccIdealResist = 0;
int vaccChangeTimer = 0;

float BlastDangerValue = 0.f;
float FireDangerValue = 0.f;

// Find dangerous players on the other team
// None of these values correctly account for ramp-up / fall-off
float BulletDangerValue(CBaseEntity* pPatient)
{
	for (const auto& player : g_EntityCache.GetGroup(EGroupType::PLAYERS_ENEMIES))
	{
		const auto& pWeapon = player->GetActiveWeapon();
		const float ThreatDistance = pPatient->GetVecOrigin().DistTo(player->GetVecOrigin());

		const Vec3 vAngleTo = Math::CalcAngle(player->GetEyePosition(), pPatient->GetWorldSpaceCenter());
		const float flFOVTo = Math::CalcFov(player->GetEyeAngles(), vAngleTo);

		if (player && player->IsAlive() && !player->IsAGhost() && !player->IsTaunting() && !player->InCond(TF_COND_PHASE) && !F::AutoGlobal.ShouldIgnore(player) && pWeapon)
		{
			switch (pWeapon->GetWeaponID()) // Potential Threats TODO: Get viewangles and pop if they're looking at us or marked as a cheater.
			{
			case TF_WEAPON_SNIPERRIFLE:
			case TF_WEAPON_SNIPERRIFLE_DECAP:
			case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			{
				const float SniperDamage = Math::RemapValClamped(pWeapon->GetChargeDamage(), 0.0f, 150.0f, 0.0f, 450.0f);
				if ((player->InCond(TF_COND_ZOOMED)) && (Utils::VisPos(pPatient, player, pPatient->GetHitboxPos(HITBOX_HEAD), player->GetEyePosition()) || G::PlayerPriority[pi.friendsID].Mode() = 4))
					return SniperDamage;
				else
					return SniperDamage / 3;
				break;
			}
			case TF_WEAPON_SCATTERGUN:
			case TF_WEAPON_SODA_POPPER:
			case TF_WEAPON_PEP_BRAWLER_BLASTER:
			{
				if (pPatient->GetVecOrigin().DistTo(player->GetVecOrigin()) < 250.f)
					return 104.f;
				else
					return 20.f;
				break;
			}
			case TF_WEAPON_MINIGUN:
			{
				if (pWeapon->GetWeaponState() != 0)
					return 12.f;
				break;
			}
			case TF_WEAPON_SHOTGUN_PYRO:
			case TF_WEAPON_SHOTGUN_HWG:
			case TF_WEAPON_SHOTGUN_SOLDIER:
			case TF_WEAPON_SHOTGUN_PRIMARY:
			case TF_WEAPON_SENTRY_REVENGE:
			{
				if (pPatient->GetVecOrigin().DistTo(player->GetVecOrigin()) < 250.f)
					return 90.f;
				else
					return 20.f;
				break;
			}
			case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
			{
				return ;
				break;
			}
			case TF_WEAPON_PISTOL:
			case TF_WEAPON_PISTOL_SCOUT:
			case TF_WEAPON_REVOLVER:
			case TF_WEAPON_SENTRY_BULLET:
			case TF_WEAPON_HANDGUN_SCOUT_SECONDARY:
			case TF_WEAPON_CHARGED_SMG:
			case TF_WEAPON_SMG:
			{
				return ;
				break;
			}
			default:
				return 0.f;
				break;
			}
		}
	}

	for (const auto& pProjectile : g_EntityCache.GetGroup(EGroupType::WORLD_PROJECTILES))
	{
		if (!pProjectile->GetVelocity().IsZero() && !(pProjectile->GetTeamNum() == pPatient->GetTeamNum()))
		{
			float ProjectileDamage = 0.f;

			switch (pProjectile->GetClassID())
			{
			case ETFClassID::CTFProjectile_Arrow:
			{
				ProjectileDamage = 120.f;
			}
			case ETFClassID::CTFProjectile_EnergyRing:
			{
				ProjectileDamage = 16.f;
			}
			case ETFClassID::CTFProjectile_Cleaver:
			{
				ProjectileDamage = 50.f;
			}
			case ETFClassID::CTFProjectile_HealingBolt:
			{
				ProjectileDamage = .f;
			}
			case ETFClassID::CTFProjectile_Flare:
			{
				ProjectileDamage = .f;
			}
			case ETFClassID::CTFProjectile_Throwable:
			{
				ProjectileDamage = .f;
			}
			default:
				ProjectileDamage = 0.f;
				break;
			}

			const Vec3 vPredicted = (pProjectile->GetAbsOrigin() + pProjectile->GetVelocity());
			const float flHypPred = sqrtf(pPatient->GetVecOrigin().DistToSqr(vPredicted));
			const float flHyp = sqrtf(pPatient->GetVecOrigin().DistToSqr(pProjectile->GetVecOrigin()));
			if (flHypPred < flHyp && pPatient->GetVecOrigin().DistTo(vPredicted) < pProjectile->GetVelocity().Length())
			{
				float ProjectileDamageFinal = ProjectileDamage;
				if (pProjectile->IsCritBoosted()) { ProjectileDamageFinal = ProjectileDamage * 3.f; }
				else { return ProjectileDamageFinal; }
			}
			else { return 0; }
		}
	}
}

float BlastDangerValue(CBaseEntity* pPatient)
{

}

float FireDangerValue(CBaseEntity* pPatient)
{

}


int CurrentResistance()
{
	if (const auto& pWeapon = g_EntityCache.GetWeapon())
	{
		return pWeapon->GetChargeResistType();
	}
	return 0;
}

int ChargeCount()
{
	if (const auto& pWeapon = g_EntityCache.GetWeapon())
	{
		if (G::CurItemDefIndex == Medic_s_TheVaccinator) { return pWeapon->GetUberCharge() / 0.25f; }
		return pWeapon->GetUberCharge() / 1.f;
	}
	return 1;
}

int OptimalResistance(CBaseEntity* pPatient, bool* pShouldPop)
{
	const float bulletDanger = BulletDangerValue(pPatient);
	const float fireDanger = FireDangerValue(pPatient);
	const float blastDanger = BlastDangerValue(pPatient);

	const float bulletSensivity = Vars::Triggerbot::Uber::BulletSensitivity.Value;
	const float fireSensivity = Vars::Triggerbot::Uber::FireSensitivity.Value;
	const float blastSensivity = Vars::Triggerbot::Uber::BlastSensitivity.Value;

	m_flHealth = static_cast<float>(pTarget->GetHealth());

	if (pShouldPop)
	{
		int charges = ChargeCount();
		if (bulletDanger >= (bulletSensivity || m_flHealth) && Vars::Triggerbot::Uber::BulletRes.Value) { *pShouldPop = true; }
		if (fireDanger >= (fireSensivity || m_flHealth) && Vars::Triggerbot::Uber::FireRes.Value) { *pShouldPop = true; }
		if (blastDanger >= (blastSensivity || m_flHealth) && Vars::Triggerbot::Uber::BlastRes.Value) { *pShouldPop = true; }
	}

	if (!(bulletDanger || fireDanger || blastDanger))
	{
		return -1;
	}

	vaccChangeTimer = CHANGE_TIMER;

	// vaccinator_change_timer = (int) change_timer;
	if (bulletDanger >= fireDanger && bulletDanger >= blastDanger) { return 0; }
	if (blastDanger >= fireDanger && blastDanger >= bulletDanger) { return 1; }
	if (fireDanger >= bulletDanger && fireDanger >= blastDanger) { return 2; }
	return -1;
}

void SetResistance(int pResistance)
{
	Math::Clamp(pResistance, 0, 2);
	vaccChangeTimer = CHANGE_TIMER;
	vaccIdealResist = pResistance;

	const int curResistance = CurrentResistance();
	if (pResistance == curResistance) { return; }
	if (pResistance > curResistance)
	{
		vaccChangeState = pResistance - curResistance;
	}
	else
	{
		vaccChangeState = 3 - curResistance + pResistance;
	}
}

void DoResistSwitching(CUserCmd* pCmd)
{
	if (vaccChangeTimer > 0)
	{
		vaccChangeTimer--;
	}
	else
	{
		vaccChangeTimer = CHANGE_TIMER;
	}

	if (!vaccChangeState) { return; }
	if (CurrentResistance() == vaccIdealResist)
	{
		vaccChangeTicks = 0;
		vaccChangeState = 0;
		return;
	}
	if (pCmd->buttons & IN_RELOAD)
	{
		vaccChangeTicks = 8;
		return;
	}
	if (vaccChangeTicks <= 0)
	{
		pCmd->buttons |= IN_RELOAD;
		vaccChangeState--;
		vaccChangeTicks = 8;
	}
	else
	{
		vaccChangeTicks--;
	}
}

void CAutoUber::Run(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon, CUserCmd* pCmd)
{
	if (!Vars::Triggerbot::Uber::Active.Value || //Not enabled, return
		pWeapon->GetWeaponID() != TF_WEAPON_MEDIGUN || //Not medigun, return
		G::CurItemDefIndex == Medic_s_TheKritzkrieg || //Kritzkrieg,  return
		ChargeCount() < 1) //Not charged
	{
		return;
	}

	//Check local status, if enabled. Don't pop if local already is not vulnerable
	if (Vars::Triggerbot::Uber::PopLocal.Value && pLocal->IsVulnerable())
	{
		m_flHealth = static_cast<float>(pLocal->GetHealth());
		m_flMaxHealth = static_cast<float>(pLocal->GetMaxHealth());

		if (Vars::Triggerbot::Uber::AutoVacc.Value && G::CurItemDefIndex == Medic_s_TheVaccinator)
		{
			// Auto vaccinator
			bool shouldPop = false;


			DoResistSwitching(pCmd);

			const int optResistance = OptimalResistance(pLocal, &shouldPop);
			if (optResistance >= 0 && optResistance != CurrentResistance())
			{
				SetResistance(optResistance);
			}

			if (shouldPop && CurrentResistance() == optResistance)
			{
				pCmd->buttons |= IN_ATTACK2;
			}
		}
		else
		{
			// Default medigun
			if (((m_flHealth / m_flMaxHealth) * 100.0f) <= Vars::Triggerbot::Uber::HealthLeft.Value)
			{
				pCmd->buttons |= IN_ATTACK2; //We under the wanted health percentage, pop
				return; //Popped, no point checking our target's status
			}
		}
	}

	//Will be null as long as we aren't healing anyone
	if (const auto& pTarget = pWeapon->GetHealingTarget())
	{
		//Ignore if target is somehow dead, or already not vulnerable
		if (!pTarget->IsAlive() || !pTarget->IsVulnerable() || pTarget->GetDormant())
		{
			return;
		}

		//Dont waste if not a friend, fuck off scrub
		if (Vars::Triggerbot::Uber::OnlyFriends.Value && !g_EntityCache.IsFriend(pTarget->GetIndex()))
		{
			return;
		}

		//Check target's status
		m_flHealth = static_cast<float>(pTarget->GetHealth());
		m_flMaxHealth = static_cast<float>(pTarget->GetMaxHealth());

		if (Vars::Triggerbot::Uber::VoiceCommand.Value)
		{
			const int iTargetIndex = pTarget->GetIndex();
			for (const auto& iEntity : m_MedicCallers)
			{
				if (iEntity == iTargetIndex)
				{
					pCmd->buttons |= IN_ATTACK2;
					break;
				}
			}
		}
		m_MedicCallers.clear();

		if (Vars::Triggerbot::Uber::AutoVacc.Value && G::CurItemDefIndex == Medic_s_TheVaccinator)
		{
			// Auto vaccinator
			bool shouldPop = false;
			DoResistSwitching(pCmd);

			const int optResistance = OptimalResistance(pTarget, &shouldPop);
			if (optResistance >= 0 && optResistance != CurrentResistance())
			{
				SetResistance(optResistance);
			}

			if (shouldPop && CurrentResistance() == optResistance)
			{
				pCmd->buttons |= IN_ATTACK2;
			}
		}
		else
		{
			// Default mediguns
			if (((m_flHealth / m_flMaxHealth) * 100.0f) <= Vars::Triggerbot::Uber::HealthLeft.Value)
			{
				pCmd->buttons |= IN_ATTACK2; //Target under wanted health percentage, pop
			}
		}
	}
}
