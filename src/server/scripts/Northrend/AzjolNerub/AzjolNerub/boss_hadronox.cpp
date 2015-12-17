/*
 *TER-Server
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "azjol_nerub.h"

enum Spells
{
    SPELL_ACID_CLOUD                              = 53400, // Victim
    SPELL_LEECH_POISON                            = 53030, // Victim
    SPELL_PIERCE_ARMOR                            = 53418, // Victim
    SPELL_WEB_GRAB                                = 57731, // Victim
    SPELL_WEB_FRONT_DOORS                         = 53177, // Self
    SPELL_WEB_SIDE_DOORS                          = 53185, // Self
    H_SPELL_ACID_CLOUD                            = 59419,
    H_SPELL_LEECH_POISON                          = 59417,
    H_SPELL_WEB_GRAB                              = 59421
};

class boss_hadronox : public CreatureScript
{
public:
    boss_hadronox() : CreatureScript("boss_hadronox") { }

    struct boss_hadronoxAI : public ScriptedAI
    {
        boss_hadronoxAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            fMaxDistance = 50.0f;
            bFirstTime = true;
        }

        InstanceScript* instance;

        uint32 uiAcidTimer;
        uint32 uiLeechTimer;
        uint32 uiPierceTimer;
        uint32 uiGrabTimer;
        uint32 uiDoorsTimer;
        uint32 uiCheckDistanceTimer;

        bool bFirstTime;

        float fMaxDistance;

        void Reset()
        {
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 9.0f);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 9.0f);

            uiAcidTimer = urand(10*IN_MILLISECONDS, 14*IN_MILLISECONDS);
            uiLeechTimer = urand(3*IN_MILLISECONDS, 9*IN_MILLISECONDS);
            uiPierceTimer = urand(1*IN_MILLISECONDS, 3*IN_MILLISECONDS);
            uiGrabTimer = urand(15*IN_MILLISECONDS, 19*IN_MILLISECONDS);
            uiDoorsTimer = urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            uiCheckDistanceTimer = 2*IN_MILLISECONDS;

            if (instance && (instance->GetData(DATA_HADRONOX_EVENT) != DONE && !bFirstTime))
                instance->SetData(DATA_HADRONOX_EVENT, FAIL);

            bFirstTime = false;
        }

        //when Hadronox kills any enemy (that includes a party member) she will regain 10% of her HP if the target had Leech Poison on
        void KilledUnit(Unit* Victim)
        {
            // not sure if this aura check is correct, I think it is though
            if (!Victim || !Victim->HasAura(DUNGEON_MODE(SPELL_LEECH_POISON, H_SPELL_LEECH_POISON)) || !me->isAlive())
                return;

            me->ModifyHealth(int32(me->CountPctFromMaxHealth(10)));
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
                instance->SetData(DATA_HADRONOX_EVENT, DONE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            if (instance)
                instance->SetData(DATA_HADRONOX_EVENT, IN_PROGRESS);
            me->SetInCombatWithZone();
        }

        void CheckDistance(float dist, const uint32 uiDiff)
        {
            if (!me->isInCombat())
                return;

            float x=0.0f, y=0.0f, z=0.0f;
            me->GetRespawnPosition(x, y, z);

            if (uiCheckDistanceTimer <= uiDiff)
                uiCheckDistanceTimer = 5*IN_MILLISECONDS;
            else
            {
                uiCheckDistanceTimer -= uiDiff;
                return;
            }
            if (me->IsInEvadeMode() || !me->GetVictim())
                return;
            if (me->GetDistance(x, y, z) > dist)
                EnterEvadeMode();
        }

        void UpdateAI(const uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            // Without he comes up through the air to players on the bridge after krikthir if players crossing this bridge!
            CheckDistance(fMaxDistance, diff);

            if (me->HasAura(SPELL_WEB_FRONT_DOORS) || me->HasAura(SPELL_WEB_SIDE_DOORS))
            {
                if (IsCombatMovementAllowed())
                    SetCombatMovement(false);
            }
            else if (!IsCombatMovementAllowed())
                SetCombatMovement(true);

            if (uiPierceTimer <= diff)
            {
                DoCastVictim(SPELL_PIERCE_ARMOR);
                uiPierceTimer = 8*IN_MILLISECONDS;
            } else uiPierceTimer -= diff;

            if (uiAcidTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_ACID_CLOUD);

                uiAcidTimer = urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            } else uiAcidTimer -= diff;

            if (uiLeechTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_LEECH_POISON);

                uiLeechTimer = urand(11*IN_MILLISECONDS, 14*IN_MILLISECONDS);
            } else uiLeechTimer -= diff;

            if (uiGrabTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0)) // Draws all players (and attacking Mobs) to itself.
                    DoCast(target, SPELL_WEB_GRAB);

                uiGrabTimer = urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            } else uiGrabTimer -= diff;

            if (uiDoorsTimer <= diff)
            {
                uiDoorsTimer = urand(30*IN_MILLISECONDS, 60*IN_MILLISECONDS);
            } else uiDoorsTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (HealthBelowPct(90))
                if (attacker && (attacker->GetTypeId() != TYPEID_PLAYER || !attacker->isPet()))
                    return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_hadronoxAI(creature);
    }
};

void AddSC_boss_hadronox()
{
    new boss_hadronox;
}
