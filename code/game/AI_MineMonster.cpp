/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "b_local.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		54
#define MIN_DISTANCE_SQR	(MIN_DISTANCE * MIN_DISTANCE)

#define MAX_DISTANCE		128
#define MAX_DISTANCE_SQR	(MAX_DISTANCE * MAX_DISTANCE)

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

/*
-------------------------
NPC_MineMonster_Precache
-------------------------
*/
void NPC_MineMonster_Precache(void)
{
	for (int i = 0; i < 4; i++)
	{
		G_SoundIndex(va("sound/chars/mine/misc/bite%i.wav", i+1));
		G_SoundIndex(va("sound/chars/mine/misc/miss%i.wav", i+1));
	}
}


/*
-------------------------
MineMonster_Idle
-------------------------
*/
void MineMonster_Idle(void)
{
	if (UpdateGoal())
	{
		ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}
}


/*
-------------------------
MineMonster_Patrol
-------------------------
*/
void MineMonster_Patrol(void)
{
	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}

	vec3_t dif;
	VectorSubtract(g_entities[0].currentOrigin, NPC->currentOrigin, dif);

	if (VectorLengthSquared(dif) < 256 * 256)
	{
		G_SetEnemy(NPC, &g_entities[0]);
	}

	if (NPC_CheckEnemyExt(qtrue) == qfalse)
	{
		MineMonster_Idle();
		return;
	}
}

/*
-------------------------
MineMonster_Move
-------------------------
*/
void MineMonster_Move(qboolean visible)
{
	if (NPCInfo->localState != LSTATE_WAITING)
	{
		NPCInfo->goalEntity = NPC->enemy;
		NPC_MoveToGoal(qtrue);
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}

//---------------------------------------------------------
void MineMonster_TryDamage(gentity_t *enemy, int damage)
{
	vec3_t	end, dir;
	trace_t	tr;

	if (!enemy)
	{
		return;
	}

	AngleVectors(NPC->client->ps.viewangles, dir, NULL, NULL);
	VectorMA(NPC->currentOrigin, MIN_DISTANCE, dir, end);

	// Should probably trace from the mouth, but, ah well.
	gi.trace(&tr, NPC->currentOrigin, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT, (EG2_Collision)0, 0);

	if (tr.entityNum >= 0 && tr.entityNum < ENTITYNUM_NONE)
	{
		G_Damage(&g_entities[tr.entityNum], NPC, NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE);
		G_SoundOnEnt(NPC, CHAN_VOICE_ATTEN, va("sound/chars/mine/misc/bite%i.wav", Q_irand(1,4)));
	}
	else
	{
		G_SoundOnEnt(NPC, CHAN_VOICE_ATTEN, va("sound/chars/mine/misc/miss%i.wav", Q_irand(1,4)));
	}
}

//------------------------------
void MineMonster_Attack(void)
{
	if (!TIMER_Exists(NPC, "attacking"))
	{
		// usually try and play a jump attack if the player somehow got above them....or just really rarely
		if (NPC->enemy && ((NPC->enemy->currentOrigin[2] - NPC->currentOrigin[2] > 10 && Q_flrand(0.0f, 1.0f) > 0.1f)
						|| Q_flrand(0.0f, 1.0f) > 0.8f))
		{
			// Going to do ATTACK4
			TIMER_Set(NPC, "attacking", 1750 + Q_flrand(0.0f, 1.0f) * 200);
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			TIMER_Set(NPC, "attack2_dmg", 950); // level two damage
		}
		else if (Q_flrand(0.0f, 1.0f) > 0.5f)
		{
			if (Q_flrand(0.0f, 1.0f) > 0.8f)
			{
				// Going to do ATTACK3, (rare)
				TIMER_Set(NPC, "attacking", 850);
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

				TIMER_Set(NPC, "attack2_dmg", 400); // level two damage
			}
			else
			{
				// Going to do ATTACK1
				TIMER_Set(NPC, "attacking", 850);
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

				TIMER_Set(NPC, "attack1_dmg", 450); // level one damage
			}
		}
		else
		{
			// Going to do ATTACK2
			TIMER_Set(NPC, "attacking", 1250);
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			TIMER_Set(NPC, "attack1_dmg", 700); // level one damage
		}
	}
	else
	{
		// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
		if (TIMER_Done2(NPC, "attack1_dmg", qtrue))
		{
			MineMonster_TryDamage(NPC->enemy, 5);
		}
		else if (TIMER_Done2(NPC, "attack2_dmg", qtrue))
		{
			MineMonster_TryDamage(NPC->enemy, 10);
		}
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2(NPC, "attacking", qtrue);
}

//----------------------------------
void MineMonster_Combat(void)
{
	// If we cannot see our target or we have somewhere to go, then do that
	if (!NPC_ClearLOS(NPC->enemy) || UpdateGoal())
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

		NPC_MoveToGoal(qtrue);
		return;
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(qtrue);

	float	distance	= DistanceHorizontalSquared(NPC->currentOrigin, NPC->enemy->currentOrigin);

	qboolean	advance = (qboolean)(distance > MIN_DISTANCE_SQR ? qtrue : qfalse );

	if ((advance || NPCInfo->localState == LSTATE_WAITING) && TIMER_Done(NPC, "attacking")) // waiting monsters can't attack
	{
		if (TIMER_Done2(NPC, "takingPain", qtrue))
		{
			NPCInfo->localState = LSTATE_CLEAR;
		}
		else
		{
			MineMonster_Move(qtrue);
		}
	}
	else
	{
		MineMonster_Attack();
	}
}

/*
-------------------------
NPC_MineMonster_Pain
-------------------------
*/
void NPC_MineMonster_Pain(gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc)
{
	G_AddEvent(self, EV_PAIN, floor((float)self->health/self->max_health*100.0f));

	if (damage >= 10)
	{
		TIMER_Remove(self, "attacking");
		TIMER_Remove(self, "attacking1_dmg");
		TIMER_Remove(self, "attacking2_dmg");
		TIMER_Set(self, "takingPain", 1350);

		VectorCopy(self->NPC->lastPathAngles, self->s.angles);

		NPC_SetAnim(self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

		if (self->NPC)
		{
			self->NPC->localState = LSTATE_WAITING;
		}
	}
}


/*
-------------------------
NPC_BSMineMonster_Default
-------------------------
*/
void NPC_BSMineMonster_Default(void)
{
	if (NPC->enemy)
	{
		MineMonster_Combat();
	}
	else if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
	{
		MineMonster_Patrol();
	}
	else
	{
		MineMonster_Idle();
	}

	NPC_UpdateAngles(qtrue, qtrue);
}
