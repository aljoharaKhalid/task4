
#include "ammo.h"
#include "bionics.h"
#include "catacharset.h"
#include "compatibility.h" // needed for the workaround for the std::to_string bug in some compilers
#include "construction.h"
#include "coordinate_conversions.h"
#include "craft_command.h"
#include "debug.h"
#include "dialogue.h"
#include "editmap.h"
#include "game.h"
#include "iexamine.h"
#include "item_group.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "messages.h"
#include "mission.h"
#include "mission_companion.h"
#include "mtype.h"
#include "npc.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "recipe.h"
#include "recipe_groups.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "string_input_popup.h"
#include "translations.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_range.h"
#include "vpart_reference.h"

#include "faction_camp.h"
#include "basecamp.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

const skill_id skill_dodge( "dodge" );
const skill_id skill_gun( "gun" );
const skill_id skill_unarmed( "unarmed" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_stabbing( "stabbing" );
const skill_id skill_bashing( "bashing" );
const skill_id skill_melee( "melee" );
const skill_id skill_fabrication( "fabrication" );
const skill_id skill_survival( "survival" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_speech( "speech" );
const skill_id skill_tailor( "tailor" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_traps( "traps" );
const skill_id skill_archery( "archery" );
const skill_id skill_swimming( "swimming" );

using npc_ptr = std::shared_ptr<npc>;
using comp_list = std::vector<npc_ptr>;

static const int COMPANION_SORT_POINTS = 12;
enum class sort_pt_ids : int {
    ufood, cfood, seeds,
    weapons, clothing, bionics,
    tools, wood, trash,
    books, meds, ammo
};

static std::array<std::pair<const std::string, point>, COMPANION_SORT_POINTS> sort_point_data = { {
        { _( "food for you" ), point( -3, -1 ) }, { _( "food for companions" ), point( 1, 0 ) },
        { _( "seeds" ), point( -1, -1 ) }, { _( "weapons" ), point( -1, 1 ) },
        { _( "clothing" ), point( -3, -2 ) }, { _( "bionics" ), point( -3, 1 ) },
        { _( "all kinds of tools" ), point( -3, 2 ) },
        { _( "wood of various sorts" ), point( -5, 2 ) },
        { _( "trash and rotting food" ), point( -6, -3 ) }, { _( "books" ), point( -3, 1 ) },
        { _( "medication" ), point( -3, 1 ) }, { _( "ammo" ), point( -3, 1 ) }
    }
};

static bool update_time_left( std::string &entry, const comp_list &npc_list )
{
    bool avail = false;
    for( auto &comp : npc_list ) {
        if( comp->companion_mission_time_ret < calendar:: turn ) {
            entry = entry +  _( " [DONE]\n" );
            avail = true;
        } else {
            entry = entry + " [" +
                    to_string( comp->companion_mission_time_ret - calendar::turn ) +
                    _( " left]\n" );
        }
    }
    entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
    return avail;
}

static bool update_time_fixed( std::string &entry, const comp_list &npc_list,
                               const time_duration &duration )
{
    bool avail = false;
    for( auto &comp : npc_list ) {
        time_duration elapsed = calendar::turn - comp->companion_mission_time;
        entry = entry + " " +  comp->name + " [" + to_string( elapsed ) + "/" +
                to_string( duration ) + "]\n";
        avail |= elapsed >= duration;
    }
    entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
    return avail;
}

static basecamp *get_basecamp( npc &p )
{
    basecamp *bcp = g->m.camp_at( p.pos(), 60 );
    if( bcp ) {
        return bcp;
    }
    g->m.add_camp( p.pos(), "faction_camp" );
    bcp = g->m.camp_at( p.pos(), 60 );
    if( bcp ) {
        bcp->define_camp( p );
    }
    return bcp;
}

void talk_function::become_overseer( npc &p )
{
    add_msg( _( "%s has become a camp manager." ), p.name );
    if( p.name.find( _( ", Camp Manager" ) ) == std::string::npos ) {
        p.name = p.name + _( ", Camp Manager" );
    }
    p.companion_mission_role_id = "FACTION_CAMP";
    p.set_attitude( NPCATT_NULL );
    p.mission = NPC_MISSION_GUARD_ALLY;
    p.chatbin.first_topic = "TALK_CAMP_OVERSEER";
    p.set_destination();
    get_basecamp( p );
}


void talk_function::camp_missions( mission_data &mission_key, npc &p )
{
    std::string entry;
    basecamp *bcp = get_basecamp( p );
    if( !bcp ) {
        return;
    }

    const std::string camp_ctr = "camp";
    const std::string base_dir = "[B]";
    std::string bldg = bcp->next_upgrade( base_dir );

    if( bldg != "null" ) {
        comp_list npc_list = companion_list( p, "_faction_upgrade_camp" );
        entry = om_upgrade_description( bldg );
        mission_key.add_start( "Upgrade Camp", _( "Upgrade Camp" ), "", entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Working to expand your camp!\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Ally from Upgrading",
                                    _( "Recover Ally from Upgrading" ), "", entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 0, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_crafting_" + base_dir );
        //This handles all crafting by the base, regardless of level
        std::map<std::string, std::string> craft_r = bcp->recipe_deck( base_dir );
        inventory found_inv = g->u.crafting_inventory();
        if( npc_list.empty() ) {
            for( std::map<std::string, std::string>::const_iterator it = craft_r.begin();
                 it != craft_r.end(); ++it ) {
                std::string title_e = base_dir + it->first;
                entry = om_craft_description( it->second );
                const recipe &recp = recipe_id( it->second ).obj();
                bool craftable = recp.requirements().can_make_with_inventory( found_inv, 1 );
                mission_key.add_start( title_e, "", base_dir, entry, craftable );
            }
        } else {
            entry = _( "Busy crafting!\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( base_dir + " (Finish) Crafting",
                                    base_dir + _( " (Finish) Crafting" ), base_dir, entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 1, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_gathering" );
        entry = om_gathering_description( p, bldg );
        mission_key.add_start( "Gather Materials", _( "Gather Materials" ), "", entry,
                               npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = _( "Searching for materials to upgrade the camp.\n" );
            bool avail = update_time_fixed( entry, npc_list, 3_hours );
            mission_key.add_return( "Recover Ally from Gathering",
                                    _( "Recover Ally from Gathering" ), "", entry, avail );
        }

        entry = string_format( _( "Notes:\n"
                                  "Distribute food to your follower and fill you larders.  Place "
                                  "the food you wish to distribute on the companion food sort "
                                  "point.  By default, that is opposite the tent door between "
                                  "the manager and wall.\n \n"
                                  "Effects:\n"
                                  "> Increases your faction's food supply value which in turn is "
                                  "used to pay laborers for their time\n \n"
                                  "Must have enjoyability >= -6\n"
                                  "Perishable food liquidated at penalty depending on upgrades "
                                  "and rot time:\n"
                                  "> Rotten: 0%%\n"
                                  "> Rots in < 2 days: 60%%\n"
                                  "> Rots in < 5 days: 80%%\n \n"
                                  "Total faction food stock: %d kcal or %d day's rations" ),
                               10 * camp_food_supply(), camp_food_supply( 0, true ) );
        mission_key.add( "Distribute Food", _( "Distribute Food" ), entry );

        entry = string_format( _( "Notes:\n"
                                  "Reset the points that items are sorted to using the [ Menial "
                                  "Labor ] mission.\n \n"
                                  "Effects:\n"
                                  "> Assignable Points: food, food for distribution, seeds, "
                                  "weapons, clothing, bionics, all kinds of tools, wood, trash, "
                                  "books, medication, and ammo.\n"
                                  "> Items sitting on any type of furniture will not be moved.\n"
                                  "> Items that are not listed in one of the categories are "
                                  "defaulted to the tools group." ) );
        mission_key.add( "Reset Sort Points", _( "Reset Sort Points" ), entry );
    }

    if( bcp->has_level( camp_ctr, 2, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_firewood" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to gather light brush and heavy sticks.\n \n"
                                  "Skill used: survival\n"
                                  "Difficulty: N/A \n"
                                  "Gathering Possibilities:\n"
                                  "> heavy sticks\n"
                                  "> withered plants\n"
                                  "> splintered wood\n \n"
                                  "Risk: Very Low\n"
                                  "Time: 3 Hours, Repeated\n"
                                  "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( "Collect Firewood", _( "Collect Firewood" ), "", entry,
                               npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = _( "Searching for firewood.\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Firewood Gatherers",
                                    _( "Recover Firewood Gatherers" ), "", entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 3, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_menial" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to do low level chores and sort supplies.\n \n"
                                  "Skill used: fabrication\n"
                                  "Difficulty: N/A \n"
                                  "Effects:\n"
                                  "> Material left outside on the ground will be sorted into the "
                                  "four crates in front of the tent.\n"
                                  "Default, top to bottom:  Clothing, Food, Books/Bionics, and "
                                  "Tools.  Wood will be piled to the south.  Trash to the north."
                                  "\n\nRisk: None\n"
                                  "Time: 3 Hours\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( "Menial Labor", _( "Menial Labor" ), "", entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Performing menial labor...\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Menial Laborer", _( "Recover Menial Laborer" ), "",
                                    entry, avail );
        }
    }

    if( bcp->can_expand() ) {
        comp_list npc_list = companion_list( p, "_faction_camp_expansion" );
        entry = string_format( _( "Notes:\n"
                                  "Your base has become large enough to support an expansion."
                                  "Expansions open up new opportunities but can be expensive and "
                                  "time consuming.  Pick them carefully, only 8 can be built at "
                                  "each camp.\n \n"
                                  "Skill used: N/A \n"
                                  "Effects:\n"
                                  "> Choose any one of the available expansions.  Starting with a "
                                  "farm is always a solid choice since food is used to support "
                                  "companion missions and little is needed to get it going.  "
                                  "With minimal investment, a mechanic can be useful as a "
                                  "chop-shop to rapidly dismantle large vehicles, and a forge "
                                  "provides the resources to make charcoal.  \n \n"
                                  "NOTE: Actions available through expansions are located in "
                                  "separate tabs of the Camp Manager window.  \n \n"
                                  "Risk: None\n"
                                  "Time: 3 Hours \n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( "Expand Base", _( "Expand Base" ), "", entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Surveying for expansion...\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Surveyor", _( "Recover Surveyor" ), "",
                                    entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 5, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_cut_log" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to a nearby forest to cut logs.\n \n"
                                  "Skill used: fabrication\n"
                                  "Difficulty: 1 \n"
                                  "Effects:\n"
                                  "> 50%% of trees/trunks at the forest position will be cut down.\n"
                                  "> 100%% of total material will be brought back.\n"
                                  "> Repeatable with diminishing returns.\n"
                                  "> Will eventually turn forests into fields.\n"
                                  "Risk: None\n"
                                  "Time: 6 Hour Base + Travel Time + Cutting Time\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( "Cut Logs", _( "Cut Logs" ), "", entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Cutting logs in the woods...\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Log Cutter", _( "Recover Log Cutter" ), "", entry,
                                    avail );
        }
    }

    if( bcp->has_level( camp_ctr, 5, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_clearcut" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to a clear a nearby forest.\n \n"
                                  "Skill used: fabrication\n"
                                  "Difficulty: 1 \n"
                                  "Effects:\n"
                                  "> 95%% of trees/trunks at the forest position"
                                  " will be cut down.\n"
                                  "> 0%% of total material will be brought back.\n"
                                  "> Forest should become a field tile.\n"
                                  "> Useful for clearing land for another faction camp.\n \n"
                                  "Risk: None\n"
                                  "Time: 6 Hour Base + Travel Time + Cutting Time\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( "Clearcut", _( "Clear a forest" ), "", entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Clearing a forest...\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Clearcutter", _( "Recover Clear Cutter" ), "", entry,
                                    avail );
        }
    }

    if( bcp->has_level( camp_ctr, 7, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_hide_site" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to build an improvised shelter and stock it "
                                  "with equipment at a distant map location.\n \n"
                                  "Skill used: survival\n"
                                  "Difficulty: 3\n"
                                  "Effects:\n"
                                  "> Good for setting up resupply or contingency points.\n"
                                  "> Gear is left unattended and could be stolen.\n"
                                  "> Time dependent on weight of equipment being sent forward.\n \n"
                                  "Risk: Medium\n"
                                  "Time: 6 Hour Construction + Travel\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( "Setup Hide Site", _( "Setup Hide Site" ), "", entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Setting up a hide site...\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Hide Setup", _( "Recover Hide Setup" ), "", entry,
                                    avail );
        }
        npc_list = companion_list( p, "_faction_camp_hide_trans" );
        entry = string_format( _( "Notes:\n"
                                  "Push gear out to a hide site or bring gear back from one.\n \n"
                                  "Skill used: survival\n"
                                  "Difficulty: 1\n"
                                  "Effects:\n"
                                  "> Good for returning equipment you left in the hide site "
                                  "shelter.\n"
                                  "> Gear is left unattended and could be stolen.\n"
                                  "> Time dependent on weight of equipment being sent forward or "
                                  "back.\n \n"
                                  "Risk: Medium\n"
                                  "Time: 1 Hour Base + Travel\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( "Relay Hide Site", _( "Relay Hide Site" ), "", entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Transferring gear to a hide site...\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Hide Relay", _( "Recover Hide Relay" ), "", entry,
                                    avail );
        }
    }

    if( bcp->has_level( camp_ctr, 2, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_foraging" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to forage for edible plants.\n \n"
                                  "Skill used: survival\n"
                                  "Difficulty: N/A \n"
                                  "Foraging Possibilities:\n"
                                  "> wild vegetables\n"
                                  "> fruits and nuts depending on season\n"
                                  "May produce less food than consumed!\n"
                                  "Risk: Very Low\n"
                                  "Time: 4 Hours, Repeated\n"
                                  "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( "Camp Forage", _( "Forage for plants" ), "", entry,
                               npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = _( "Foraging for edible plants.\n" );
            bool avail = update_time_fixed( entry, npc_list, 4_hours );
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            mission_key.add_return( "Recover Foragers", _( "Recover Foragers" ), "",
                                    entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 6, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_trapping" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to set traps for small game.\n \n"
                                  "Skill used: trapping\n"
                                  "Difficulty: N/A \n"
                                  "Trapping Possibilities:\n"
                                  "> small and tiny animal corpses\n"
                                  "May produce less food than consumed!\n"
                                  "Risk: Low\n"
                                  "Time: 6 Hours, Repeated\n"
                                  "Positions: %d/2\n" ), npc_list.size() );
        mission_key.add_start( "Trap Small Game", _( "Trap Small Game" ), "", entry,
                               npc_list.size() < 2 );
        if( !npc_list.empty() ) {
            entry = _( "Trapping Small Game.\n" );
            bool avail = update_time_fixed( entry, npc_list, 6_hours );
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            mission_key.add_return( "Recover Trappers", _( "Recover Trappers" ), "",
                                    entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 8, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_hunting" );
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to hunt large animals.\n \n"
                                  "Skill used: marksmanship\n"
                                  "Difficulty: N/A \n"
                                  "Hunting Possibilities:\n"
                                  "> small, medium, or large animal corpses\n"
                                  "May produce less food than consumed!\n"
                                  "Risk: Medium\n"
                                  "Time: 6 Hours, Repeated\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( "Hunt Large Animals", _( "Hunt Large Animals" ), "", entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Hunting large animals.\n" );
            bool avail = update_time_fixed( entry, npc_list, 6_hours );
            mission_key.add_return( "Recover Hunter", _( "Recover Hunter" ), "", entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 9, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_om_fortifications" );
        entry = om_upgrade_description( "faction_wall_level_N_0" );
        mission_key.add_start( "Construct Map Fort", _( "Construct Map Fortifications" ), "",
                               entry, npc_list.empty() );
        entry = om_upgrade_description( "faction_wall_level_N_1" );
        mission_key.add_start( "Construct Trench", _( "Construct Spiked Trench" ), "", entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Constructing fortifications...\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Finish Map Fort", _( "Finish Map Fortifications" ), "", entry,
                                    avail );
        }
    }

    if( bcp->has_level( camp_ctr, 11, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_recruit_0" );
        entry = bcp->recruit_start( npc_list.size() );
        mission_key.add_start( "Recruit Companions", _( "Recruit Companions" ), "", entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = _( "Searching for recruits.\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Recruiter", _( "Recover Recruiter" ), "", entry,
                                    avail );
        }
    }

    if( bcp->has_level( camp_ctr, 13, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_scout_0" );
        entry =  string_format( _( "Notes:\n"
                                   "Send a companion out into the great unknown.  High survival "
                                   "skills are needed to avoid combat but you should expect an "
                                   "encounter or two.\n \n"
                                   "Skill used: survival\n"
                                   "Difficulty: 3\n"
                                   "Effects:\n"
                                   "> Select checkpoints to customize path.\n"
                                   "> Reveals terrain around the path.\n"
                                   "> Can bounce off hide sites to extend range.\n \n"
                                   "Risk: High\n"
                                   "Time: Travel\n"
                                   "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( "Scout Mission", _( "Scout Mission" ), "", entry,
                               npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = _( "Scouting the region.\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Scout", _( "Recover Scout" ), "", entry, avail );
        }
    }

    if( bcp->has_level( camp_ctr, 15, base_dir ) ) {
        comp_list npc_list = companion_list( p, "_faction_camp_combat_0" );
        entry =  string_format( _( "Notes:\n"
                                   "Send a companion to purge the wasteland.  Their goal is to "
                                   "kill anything hostile they encounter and return when "
                                   "their wounds are too great or the odds are stacked against "
                                   "them.\n \n"
                                   "Skill used: survival\n"
                                   "Difficulty: 4\n"
                                   "Effects:\n"
                                   "> Pulls creatures encountered into combat instead of "
                                   "fleeing.\n"
                                   "> Select checkpoints to customize path.\n"
                                   "> Can bounce off hide sites to extend range.\n \n"
                                   "Risk: Very High\n"
                                   "Time: Travel\n"
                                   "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( "Combat Patrol", _( "Combat Patrol" ), "", entry,
                               npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = _( "Patrolling the region.\n" );
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Combat Patrol", _( "Recover Combat Patrol" ), "",
                                    entry, avail );
        }
    }

    //This starts all of the expansion missions
    for( const std::string &dir : bcp->directions ) {
        const std::string bldg_exp = bcp->next_upgrade( dir );
        if( bldg_exp != "null" ) {
            comp_list npc_list = companion_list( p, "_faction_upgrade_exp_" + dir );
            std::string title_e = dir + " Expansion Upgrade";
            entry = om_upgrade_description( bldg_exp );
            mission_key.add_start( title_e, dir + _( " Expansion Upgrade" ), dir, entry,
                                   npc_list.empty() );
            if( !npc_list.empty() ) {
                entry = _( "Working to upgrade your expansions!\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( "Recover Ally, " + dir + " Expansion",
                                        _( "Recover Ally, " ) + dir + _( " Expansion" ), dir,
                                        entry, avail );
            }
        }

        if( bcp->has_level( "garage", 1, dir ) ) {
            comp_list npc_list = companion_list( p, "_faction_exp_chop_shop_" + dir );
            std::string title_e = dir + " Chop Shop";
            entry = _( "Notes:\n"
                       "Have a companion attempt to completely dissemble a vehicle into "
                       "components.\n \n"
                       "Skill used: mechanics\n"
                       "Difficulty: 2 \n"
                       "Effects:\n"
                       "> Removed parts placed on the furniture in the garage.\n"
                       "> Skill plays a huge role to determine what is salvaged.\n \n"
                       "Risk: None\n"
                       "Time: 5 days \n" );
            mission_key.add_start( title_e, dir + _( " Chop Shop" ), dir, entry,
                                   npc_list.empty() );
            if( !npc_list.empty() ) {
                entry = _( "Working at the chop shop...\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( dir + " (Finish) Chop Shop",
                                        dir + _( " (Finish) Chop Shop" ), dir, entry, avail );
            }
        }

        std::map<std::string, std::string> cooking_recipes = bcp->recipe_deck( dir );
        if( bcp->has_level( "kitchen", 1, dir ) ) {
            inventory found_inv = g->u.crafting_inventory();
            comp_list npc_list = companion_list( p, "_faction_exp_kitchen_cooking_" + dir );
            if( npc_list.empty() ) {
                for( std::map<std::string, std::string>::const_iterator it = cooking_recipes.begin();
                     it != cooking_recipes.end(); ++it ) {
                    std::string title_e = dir + it->first;
                    entry = om_craft_description( it->second );
                    const recipe &recp = recipe_id( it->second ).obj();
                    bool craftable = recp.requirements().can_make_with_inventory( found_inv, 1 );
                    mission_key.add_start( title_e, "", dir, entry, craftable );
                }
            } else {
                entry = _( "Working in your kitchen!\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( dir + " (Finish) Cooking", dir + _( " (Finish) Cooking" ),
                                        dir, entry, avail );
            }
        }

        if( bcp->has_level( "blacksmith", 1, dir ) ) {
            inventory found_inv = g->u.crafting_inventory();
            comp_list npc_list = companion_list( p, "_faction_exp_blacksmith_crafting_" + dir );
            if( npc_list.empty() ) {
                for( std::map<std::string, std::string>::const_iterator it = cooking_recipes.begin();
                     it != cooking_recipes.end(); ++it ) {
                    std::string title_e = dir + it->first;
                    entry = om_craft_description( it->second );
                    const recipe &recp = recipe_id( it->second ).obj();
                    bool craftable = recp.requirements().can_make_with_inventory( found_inv, 1 );
                    mission_key.add_start( title_e, "", dir, entry, craftable );
                }
            } else {
                entry = _( "Working in your blacksmith shop!\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( dir + " (Finish) Smithing",
                                        dir + _( " (Finish) Smithing" ),
                                        dir, entry, avail );
            }
        }

        if( bcp->has_level( "farm", 1, dir ) ) {
            const tripoint farm_pos = bcp->camp_pos() + om_dir_to_offset( dir );
            comp_list npc_list = companion_list( p, "_faction_exp_plow_" + dir );
            if( npc_list.empty() ) {
                std::string title_e = dir + " Plow Fields";
                entry = _( "Notes:\n"
                           "Plow any spaces that have reverted to dirt or grass.\n \n" ) +
                        camp_farm_description( farm_pos, farm_ops::plow ) +
                        _( "\n \n"
                           "Skill used: fabrication\n"
                           "Difficulty: N/A \n"
                           "Effects:\n"
                           "> Restores only the plots created in the last expansion upgrade.\n"
                           "> Does not damage existing crops.\n \n"
                           "Risk: None\n"
                           "Time: 5 Min / Plot \n"
                           "Positions: 0/1 \n" );
                mission_key.add_start( title_e, dir + _( " Plow Fields" ), dir, entry, true );
            } else {
                entry = _( "Working to plow your fields!\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( dir + " (Finish) Plow Fields",
                                        dir + _( " (Finish) Plow Fields" ), dir, entry, avail );
            }

            npc_list = companion_list( p, "_faction_exp_plant_" + dir );
            if( npc_list.empty() && g->get_temperature( farm_pos ) > 50 ) {
                std::string title_e = dir + " Plant Fields";
                entry = _( "Notes:\n"
                           "Plant designated seeds in the spaces that have already been "
                           "tilled.\n \n" ) +
                        camp_farm_description( farm_pos, farm_ops::plant ) +
                        _( "\n \n"
                           "Skill used: survival\n"
                           "Difficulty: N/A \n"
                           "Effects:\n"
                           "> Choose which seed type or all of your seeds.\n"
                           "> Stops when out of seeds or planting locations.\n"
                           "> Will plant in ALL dirt mounds in the expansion.\n \n"
                           "Risk: None\n"
                           "Time: 1 Min / Plot \n"
                           "Positions: 0/1 \n" );
                mission_key.add_start( title_e, dir + _( " Plant Fields" ), dir, entry,
                                       npc_list.empty() );
            } else if( !npc_list.empty() ) {
                entry = _( "Working to plant your fields!\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( dir + " (Finish) Plant Fields",
                                        dir + _( " (Finish) Plant Fields" ), dir, entry, avail );
            }

            npc_list = companion_list( p, "_faction_exp_harvest_" + dir );
            if( npc_list.empty() ) {
                std::string title_e = dir + " Harvest Fields";
                entry = _( "Notes:\n"
                           "Harvest any plants that are ripe and bring the produce back.\n \n" ) +
                        camp_farm_description( farm_pos, farm_ops::harvest ) +
                        _( "\n \n"
                           "Skill used: survival\n"
                           "Difficulty: N/A \n"
                           "Effects:\n"
                           "> Will dump all harvesting products onto your location.\n \n"
                           "Risk: None\n"
                           "Time: 3 Min / Plot \n"
                           "Positions: 0/1 \n" );
                mission_key.add_start( title_e, dir + _( " Harvest Fields" ), dir, entry, true );
            } else {
                entry = _( "Working to harvest your fields!\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( dir + " (Finish) Harvest Fields",
                                        dir + _( " (Finish) Harvest Fields" ), dir, entry, avail );
            }
        }

        if( bcp->has_level( "farm", 4, dir ) ) {
            inventory found_inv = g->u.crafting_inventory();
            comp_list npc_list = companion_list( p, "_faction_exp_farm_crafting_" + dir );
            if( npc_list.empty() ) {
                for( std::map<std::string, std::string>::const_iterator it = cooking_recipes.begin();
                     it != cooking_recipes.end(); ++it ) {
                    std::string title_e = dir + it->first;
                    entry = om_craft_description( it->second );
                    const recipe &recp = recipe_id( it->second ).obj();
                    bool craftable = recp.requirements().can_make_with_inventory( found_inv, 1 );
                    mission_key.add_start( title_e, "", dir, entry, craftable );
                }
            } else {
                entry = _( "Working on your farm!\n" );
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( dir + " (Finish) Crafting",
                                        dir + _( " (Finish) Crafting" ), dir, entry, avail );
            }
        }
    }
}

bool talk_function::handle_camp_mission( mission_entry &cur_key, npc &p )
{
    basecamp *bcp = get_basecamp( p );
    if( !bcp ) {
        return false;
    }
    const std::string base_dir = "[B]";

    std::string mission_role = p.companion_mission_role_id;
    npc_ptr comp = nullptr;

    if( cur_key.id == "Distribute Food" ) {
        bcp->distribute_food();
    }

    if( cur_key.id == "Reset Sort Points" ) {
        bcp->set_sort_points( false, true );
    }

    if( cur_key.id == "Upgrade Camp" ) {
        const std::string bldg = bcp->next_upgrade( base_dir );
        start_camp_upgrade( p, bldg, "_faction_upgrade_camp" );
    } else if( cur_key.id == "Recover Ally from Upgrading" ) {
        bcp->upgrade_return( p, base_dir, "_faction_upgrade_camp" );
    }

    if( cur_key.id == "Gather Materials" ) {
        individual_mission( p, _( "departs to search for materials..." ),
                            "_faction_camp_gathering", false, {}, "survival" );
    } else if( cur_key.id == "Recover Ally from Gathering" ) {
        camp_gathering_return( p, "_faction_camp_gathering", 3_hours );
    }

    if( cur_key.id == "Collect Firewood" ) {
        individual_mission( p, _( "departs to search for firewood..." ),
                            "_faction_camp_firewood", false, {}, "survival" );
    }
    if( cur_key.id == "Recover Firewood Gatherers" ) {
        camp_gathering_return( p, "_faction_camp_firewood", 3_hours );
    }

    if( cur_key.id == "Menial Labor" ) {
        int need_food = time_to_food( 3_hours );
        if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else {
            comp = individual_mission( p, _( "departs to dig ditches and scrub toilets..." ),
                                       "_faction_camp_menial" );
            if( comp != nullptr ) {
                comp->companion_mission_time_ret = calendar::turn + 3_hours;
            }
        }
    } else if( cur_key.id == "Recover Menial Laborer" ) {
        bcp->menial_return( p );
    }

    if( cur_key.id == "Cut Logs" ) {
        start_cut_logs( p );
    } else if( cur_key.id == "Recover Log Cutter" ) {
        comp = companion_choose_return( p, "_faction_camp_cut_log",
                                        calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from working in the woods..." ), comp->name );
            camp_companion_return( *comp );
        }
    }

    if( cur_key.id == "Clearcut" ) {
        start_clearcut( p );
    } else if( cur_key.id == "Recover Clearcutter" ) {
        comp = companion_choose_return( p, "_faction_camp_clearcut", calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from working in the woods..." ), comp->name );
            camp_companion_return( *comp );
        }
    }

    if( cur_key.id == "Setup Hide Site" ) {
        start_setup_hide_site( p );
    } else if( cur_key.id == "Recover Hide Setup" ) {
        comp = companion_choose_return( p, "_faction_camp_hide_site",
                                        calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from working on the hide site..." ), comp->name );
            camp_companion_return( *comp );
        }
    }

    if( cur_key.id == "Relay Hide Site" ) {
        start_relay_hide_site( p );
    } else if( cur_key.id == "Recover Hide Transport" ) {
        comp = companion_choose_return( p, "_faction_camp_hide_trans",
                                        calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from shuttling gear between the hide site..." ), comp->name );
            camp_companion_return( *comp );
        }
    }

    if( cur_key.id == "Camp Forage" ) {
        individual_mission( p, _( "departs to search for edible plants..." ),
                            "_faction_camp_foraging", false, {}, "survival" );
    } else if( cur_key.id == "Recover Foragers" ) {
        camp_gathering_return( p, "_faction_camp_foraging", 4_hours );
    }

    if( cur_key.id == "Trap Small Game" ) {
        individual_mission( p, _( "departs to set traps for small animals..." ),
                            "_faction_camp_trapping", false, {}, "traps" );
    } else if( cur_key.id == "Recover Trappers" ) {
        camp_gathering_return( p, "_faction_camp_trapping", 6_hours );
    }

    if( cur_key.id == "Hunt Large Animals" ) {
        individual_mission( p, _( "departs to hunt for meat..." ), "_faction_camp_hunting",
                            false, {}, "gun" );
    } else if( cur_key.id == "Recover Hunter" ) {
        camp_gathering_return( p, "_faction_camp_hunting", 6_hours );
    }

    if( cur_key.id == "Construct Map Fort" || cur_key.id == "Construct Trench" ) {
        std::string bldg_exp = "faction_wall_level_N_0";
        if( cur_key.id == "Construct Trench" ) {
            bldg_exp = "faction_wall_level_N_1";
        }
        start_fortifications( bldg_exp, p );
    } else if( cur_key.id == "Finish Map Fort" ) {
        camp_fortifications_return( p );
    }

    if( cur_key.id == "Recruit Companions" ) {
        int need_food = time_to_food( 4_days );
        if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else {
            comp = individual_mission( p, _( "departs to search for recruits..." ),
                                       "_faction_camp_recruit_0", false, {}, "speech", 2 );
            if( comp != nullptr ) {
                comp->companion_mission_time_ret = calendar::turn + 4_days;
            }
        }
    } else if( cur_key.id == "Recover Recruiter" ) {
        camp_recruit_return( p, "_faction_camp_recruit_0", bcp->recruit_evaluation() );
    }

    if( cur_key.id == "Scout Mission" ) {
        start_combat_mission( "_faction_camp_scout_0", p );
    } else if( cur_key.id == "Combat Patrol" ) {
        start_combat_mission( "_faction_camp_combat_0", p );
    } else if( cur_key.id == "Recover Scout" ) {
        combat_mission_return( "_faction_camp_scout_0", p );
    } else if( cur_key.id == "Recover Combat Patrol" ) {
        combat_mission_return( "_faction_camp_combat_0", p );
    }

    if( cur_key.id == "Expand Base" ) {
        int need_food = time_to_food( 3_hours );
        if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else {
            comp = individual_mission( p, _( "departs to survey land..." ),
                                       "_faction_camp_expansion" );
            if( comp != nullptr ) {
                comp->companion_mission_time_ret = calendar::turn + 3_hours;
            }
        }
    } else if( cur_key.id == "Recover Surveyor" ) {
        bcp->survey_return( p );
    }

    bcp->craft_construction( p, cur_key.id, cur_key.dir, "BASE", "_faction_camp_crafting_" );
    if( cur_key.id == base_dir + " (Finish) Crafting" ) {
        comp = companion_choose_return( p, "_faction_camp_crafting_" + base_dir,
                                        calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns to you with something..." ), comp->name );
            camp_companion_return( *comp );
        }
    }

    for( const std::string &dir : bcp->directions ) {
        if( dir == cur_key.dir ) {
            if( cur_key.id == cur_key.dir + " Expansion Upgrade" ) {
                std::string bldg_exp = bcp->next_upgrade( cur_key.dir );
                start_camp_upgrade( p, bldg_exp, "_faction_upgrade_exp_" + cur_key.dir );
            } else if( cur_key.id == "Recover Ally, " + cur_key.dir + " Expansion" ) {
                bcp->upgrade_return( p, dir, "_faction_upgrade_exp_" + cur_key.dir );
            }

            bcp->craft_construction( p, cur_key.id, cur_key.dir, "FARM",
                                     "_faction_exp_farm_crafting_" );
            if( cur_key.id == dir + " (Finish) Crafting" && dir != base_dir ) {
                comp = companion_choose_return( p, "_faction_exp_farm_crafting_" + cur_key.dir,
                                                calendar::before_time_starts );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your farm with something..." ), comp->name );
                    camp_companion_return( *comp );
                }
            }

            bcp->craft_construction( p, cur_key.id, dir, "COOK", "_faction_exp_kitchen_cooking_" );
            if( cur_key.id == cur_key.dir + " (Finish) Cooking" ) {
                comp = companion_choose_return( p, "_faction_exp_kitchen_cooking_" + cur_key.dir,
                                                calendar::before_time_starts );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your kitchen with something..." ), comp->name );
                    camp_companion_return( *comp );
                }
            }

            bcp->craft_construction( p, cur_key.id, cur_key.dir, "SMITH",
                                     "_faction_exp_blacksmith_crafting_" );
            if( cur_key.id == cur_key.dir + " (Finish) Smithing" ) {
                comp = companion_choose_return( p, "_faction_exp_blacksmith_crafting_" +
                                                cur_key.dir, calendar::before_time_starts );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your blacksmith shop with something..." ),
                           comp->name );
                    camp_companion_return( *comp );
                }
            }

            if( cur_key.id == cur_key.dir + " Plow Fields" ) {
                individual_mission( p, _( "begins plowing the field..." ),
                                    "_faction_exp_plow_" + cur_key.dir );
            } else if( cur_key.id == dir + " (Finish) Plow Fields" ) {
                camp_farm_return( p, "_faction_exp_plow_" + cur_key.dir, farm_ops::plow );
            }

            if( cur_key.id == cur_key.dir + " Plant Fields" ) {
                inventory total_inv = g->u.crafting_inventory();
                std::vector<item *> seed_inv = total_inv.items_with( []( const item & itm ) {
                    return itm.is_seed() && itm.typeId() != "marloss_seed" &&
                           itm.typeId() != "fungal_seeds";
                } );
                if( seed_inv.empty() ) {
                    popup( _( "You have no additional seeds to give your companions..." ) );
                    individual_mission( p, _( "begins planting the field..." ),
                                        "_faction_exp_plant_" + cur_key.dir );
                } else {
                    std::vector<item *> lost_equipment =
                        individual_mission_give_equipment( seed_inv,
                                                           _( "Which seeds do you wish to have planted?" ) );
                    individual_mission( p, _( "begins planting the field..." ),
                                        "_faction_exp_plant_" + cur_key.dir, false, lost_equipment );
                }
            } else if( cur_key.id == cur_key.dir + " (Finish) Plant Fields" ) {
                camp_farm_return( p, "_faction_exp_plant_" + cur_key.dir, farm_ops::plant );
            }

            if( cur_key.id == cur_key.dir + " Harvest Fields" ) {
                individual_mission( p, _( "begins to harvest the field..." ),
                                    "_faction_exp_harvest_" + cur_key.dir, false, {}, "survival" );
            }  else if( cur_key.id == dir + " (Finish) Harvest Fields" ) {
                camp_farm_return( p, "_faction_exp_harvest_" + cur_key.dir, farm_ops::harvest );
            }

            if( cur_key.id == cur_key.dir + " Chop Shop" ) {
                int need_food = time_to_food( 5_days );
                if( camp_food_supply() < need_food ) {
                    popup( _( "You don't have enough food stored to feed your companion." ) );
                } else {
                    camp_garage_chop_start( p, "_faction_exp_chop_shop_" + dir );
                }
            } else if( cur_key.id == cur_key.dir + " (Finish) Chop Shop" ) {
                comp = companion_choose_return( p, "_faction_exp_chop_shop_" + dir,
                                                calendar::turn - 5_days );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your garage..." ), comp->name );
                    camp_companion_return( *comp );
                }
            }
            break;
        }
    }

    g->draw_ter();
    wrefresh( g->w_terrain );

    return true;
}

// camp faction companion mission start functions
void talk_function::start_camp_upgrade( npc &p, const std::string &bldg, const std::string &key )
{
    const recipe &making = recipe_id( bldg ).obj();
    //Stop upgrade if you don't have materials
    inventory total_inv = g->u.crafting_inventory();
    if( making.requirements().can_make_with_inventory( total_inv, 1 ) ) {
        time_duration making_time = time_duration::from_turns( making.time / 100 );
        int need_food = time_to_food( making_time );
        if( camp_food_supply() < need_food && bldg != "faction_base_camp_1" ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
            return;
        }
        npc_ptr comp = individual_mission( p, _( "begins to upgrade the camp..." ),
                                           key, false, {}, making.skill_used.str(),
                                           making.difficulty );
        if( comp != nullptr ) {
            comp->companion_mission_time_ret = calendar::turn + making_time;
            g->u.consume_components_for_craft( &making, 1, true );
            g->u.invalidate_crafting_inventory();
        }
    } else {
        popup( _( "You don't have the materials for the upgrade." ) );
    }
}

void talk_function::start_cut_logs( npc &p )
{
    std::vector<std::string> log_sources = { "forest", "forest_thick", "forest_water" };
    popup( _( "Forests and swamps are the only valid cutting locations." ) );
    const tripoint omt_pos = p.global_omt_location();
    tripoint forest = om_target_tile( omt_pos, 1, 50, log_sources );
    if( forest != tripoint( -999, -999, -999 ) ) {
        int tree_est = om_cutdown_trees_est( forest, 50 );
        int tree_young_est = om_harvest_ter_est( p, forest, ter_id( "t_tree_young" ), 50 );
        int dist = rl_dist( forest.x, forest.y, omt_pos.x, omt_pos.y );
        //Very roughly what the player does + 6 hours for prep, clean up, breaks
        time_duration chop_time = 6_hours + 1_hours * tree_est + 7_minutes * tree_young_est;
        //Generous to believe the NPC can move ~ 2 logs or ~8 heavy sticks (3 per young tree?)
        //per trip, each way is 1 trip 20 young trees => ~60 sticks which can be carried 8 at
        //a time, so 8 round trips or 16 trips total
        //This all needs to be in an om_carry_weight_over_distance function eventually...
        int trips = tree_est + tree_young_est * 3  / 4;
        //Always have to come back so no odd number of trips
        trips += trips & 1 ? 1 : 0;
        time_duration travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes,
                                    trips );
        time_duration work_time = travel_time + chop_time;
        int need_food = time_to_food( work_time );
        if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time, chop_time,
                       travel_time, dist, trips, need_food ) ) ) {
            return;
        } else if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
            return;
        }
        g->draw_ter();
        //wrefresh( g->w_terrain );
        npc_ptr comp = individual_mission( p, _( "departs to cut logs..." ),
                                           "_faction_camp_cut_log", false, {},
                                           "fabrication", 2 );
        if( comp != nullptr ) {
            units::mass carry_m = comp->weight_capacity() - comp->weight_carried();
            // everyone gets at least a makeshift sling of storage
            units::volume carry_v = comp->volume_capacity() - comp->volume_carried() + item(
                                        itype_id( "makeshift_sling" ) ).get_storage();
            om_cutdown_trees_logs( forest, 50 );
            om_harvest_ter( *comp, forest, ter_id( "t_tree_young" ), 50 );
            std::pair<units::mass, units::volume> harvest = om_harvest_itm( comp, forest, 95 );
            // recalculate trips based on actual load and capacity
            trips = om_carry_weight_to_trips( harvest.first, harvest.second, carry_m, carry_v );
            travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes, trips );
            work_time = travel_time + chop_time;
            comp->companion_mission_time_ret = calendar::turn + work_time;
            //If we cleared a forest...
            if( om_cutdown_trees_est( forest ) < 5 ) {
                oter_id &omt_trees = overmap_buffer.ter( forest );
                //Do this for swamps "forest_wet" if we have a swamp without trees...
                if( omt_trees.id() == "forest" || omt_trees.id() == "forest_thick" ) {
                    omt_trees = oter_id( "field" );
                }
            }
        }
    }
}

void talk_function::start_clearcut( npc &p )
{
    std::vector<std::string> log_sources = { "forest", "forest_thick" };
    popup( _( "Forests are the only valid cutting locations." ) );
    const tripoint omt_pos = p.global_omt_location();
    tripoint forest = om_target_tile( omt_pos, 1, 50, log_sources );
    if( forest != tripoint( -999, -999, -999 ) ) {
        int tree_est = om_cutdown_trees_est( forest, 95 );
        int tree_young_est = om_harvest_ter_est( p, forest, ter_id( "t_tree_young" ), 95 );
        int dist = rl_dist( forest.x, forest.y, omt_pos.x, omt_pos.y );
        //Very roughly what the player does + 6 hours for prep, clean up, breaks
        time_duration chop_time = 6_hours + 1_hours * tree_est + 7_minutes * tree_young_est;
        time_duration travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes, 2 );
        time_duration work_time = travel_time + chop_time;
        int need_food = time_to_food( work_time );
        if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time, chop_time,
                       travel_time, dist, 2, need_food ) ) ) {
            return;
        } else if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
            return;
        }
        g->draw_ter();
        //wrefresh( g->w_terrain );
        npc_ptr comp = individual_mission( p, _( "departs to clear a forest..." ),
                                           "_faction_camp_clearcut", false, {}, "fabrication",
                                           1 );
        if( comp != nullptr ) {
            comp->companion_mission_time_ret = calendar::turn + work_time;
            om_cutdown_trees_trunks( forest, 95 );
            om_harvest_ter_break( p, forest, ter_id( "t_tree_young" ), 95 );
            //If we cleared a forest...
            if( om_cutdown_trees_est( forest ) < 5 ) {
                oter_id &omt_trees = overmap_buffer.ter( forest );
                omt_trees = oter_id( "field" );
            }
        }
    }
}

void talk_function::start_setup_hide_site( npc &p )
{
    std::vector<std::string> hide_locations = { "forest", "forest_thick", "forest_water",
                                                "field"
                                              };
    popup( _( "Forests, swamps, and fields are valid hide site locations." ) );
    const tripoint omt_pos = p.global_omt_location();
    tripoint forest = om_target_tile( omt_pos, 10, 90, hide_locations, true, true, omt_pos, true );
    if( forest != tripoint( -999, -999, -999 ) ) {
        int dist = rl_dist( forest.x, forest.y, omt_pos.x, omt_pos.y );
        inventory tgt_inv = g->u.inv;
        std::vector<item *> pos_inv = tgt_inv.items_with( []( const item & itm ) {
            return !itm.can_revive();
        } );
        if( !pos_inv.empty() ) {
            std::vector<item *> losing_equipment = individual_mission_give_equipment( pos_inv );
            int trips = om_carry_weight_to_trips( losing_equipment );
            time_duration build_time = 6_hours;
            time_duration travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes,
                                        trips );
            time_duration work_time = travel_time + build_time;
            int need_food = time_to_food( work_time );
            if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time,
                           build_time, travel_time, dist, trips, need_food ) ) ) {
                return;
            } else if( camp_food_supply() < need_food ) {
                popup( _( "You don't have enough food stored to feed your companion." ) );
                return;
            }
            npc_ptr comp = individual_mission( p, _( "departs to build a hide site..." ),
                                               "_faction_camp_hide_site", false,
                                               {}, "survival", 3 );
            if( comp != nullptr ) {
                trips = om_carry_weight_to_trips( losing_equipment, comp );
                work_time = build_time + companion_travel_time_calc( forest, omt_pos, 0_minutes,
                            trips );
                comp->companion_mission_time_ret = calendar::turn + work_time;
                om_set_hide_site( *comp, forest, losing_equipment );
            }
        } else {
            popup( _( "You need equipment to setup a hide site..." ) );
        }
    }
}

void talk_function::start_relay_hide_site( npc &p )
{
    std::vector<std::string> hide_locations = { "faction_hide_site_0" };
    popup( _( "You must select an existing hide site." ) );
    const tripoint omt_pos = p.global_omt_location();
    tripoint forest = om_target_tile( omt_pos, 10, 90, hide_locations, true, true, omt_pos, true );
    if( forest != tripoint( -999, -999, -999 ) ) {
        int dist = rl_dist( forest.x, forest.y, omt_pos.x, omt_pos.y );
        inventory tgt_inv = g->u.inv;
        std::vector<item *> pos_inv = tgt_inv.items_with( []( const item & itm ) {
            return !itm.can_revive();
        } );
        std::vector<item *> losing_equipment;
        if( !pos_inv.empty() ) {
            losing_equipment = individual_mission_give_equipment( pos_inv );
        }

        //Check items in improvised shelters at hide site
        tinymap target_bay;
        target_bay.load( forest.x * 2, forest.y * 2, forest.z, false );
        std::vector<item *> hide_inv;
        for( item &i : target_bay.i_at( 11, 10 ) ) {
            hide_inv.push_back( &i );
        }
        std::vector<item *> gaining_equipment;
        if( !hide_inv.empty() ) {
            gaining_equipment = individual_mission_give_equipment( hide_inv,
                                _( "Bring gear back?" ) );
        }
        if( !losing_equipment.empty() || !gaining_equipment.empty() ) {
            //Only get charged the greater trips since return is free for both
            int trips = std::max( om_carry_weight_to_trips( gaining_equipment ),
                                  om_carry_weight_to_trips( losing_equipment ) );
            time_duration build_time = 6_hours;
            time_duration travel_time = companion_travel_time_calc( forest, omt_pos,
                                        0_minutes, trips );
            time_duration work_time = travel_time + build_time;
            int need_food = time_to_food( work_time );
            if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time,
                           build_time, travel_time, dist, trips, need_food ) ) ) {
                return;
            } else if( camp_food_supply() < need_food ) {
                popup( _( "You don't have enough food stored to feed your companion." ) );
                return;
            }
            npc_ptr comp = individual_mission( p, _( "departs for the hide site..." ),
                                               "_faction_camp_hide_site",
                                               false, {}, "survival", 3 );
            if( comp != nullptr ) {
                // recalculate trips based on actual load
                trips = std::max( om_carry_weight_to_trips( gaining_equipment, comp ),
                                  om_carry_weight_to_trips( losing_equipment, comp ) );
                work_time = build_time + companion_travel_time_calc( forest, omt_pos, 0_minutes,
                            trips );
                comp->companion_mission_time_ret = calendar::turn + work_time;
                om_set_hide_site( *comp, forest, losing_equipment, gaining_equipment );
            }
        } else {
            popup( _( "You need equipment to transport between the hide site..." ) );
        }
    }
}

void talk_function::start_fortifications( std::string &bldg_exp, npc &p )
{
    std::vector<std::string> allowed_locations;
    if( bldg_exp == "faction_wall_level_N_1" ) {
        allowed_locations = {
            "faction_wall_level_N_0", "faction_wall_level_E_0",
            "faction_wall_level_S_0", "faction_wall_level_W_0",
            "faction_wall_level_N_1", "faction_wall_level_E_1",
            "faction_wall_level_S_1", "faction_wall_level_W_1"
        };
    } else {
        allowed_locations = {
            "forest", "forest_thick", "forest_water", "field",
            "faction_wall_level_N_0", "faction_wall_level_E_0",
            "faction_wall_level_S_0", "faction_wall_level_W_0",
            "faction_wall_level_N_1", "faction_wall_level_E_1",
            "faction_wall_level_S_1", "faction_wall_level_W_1"
        };
    }
    popup( _( "Select a start and end point.  Line must be straight.  Fields, forests, and "
              "swamps are valid fortification locations.  In addition to existing fortification "
              "constructions." ) );
    const tripoint omt_pos = p.global_omt_location();
    tripoint start = om_target_tile( omt_pos, 2, 90, allowed_locations );
    popup( _( "Select an end point." ) );
    tripoint stop = om_target_tile( omt_pos, 2, 90, allowed_locations, true, false, start );
    if( start != tripoint( -999, -999, -999 ) && stop != tripoint( -999, -999, -999 ) ) {
        const recipe &making = recipe_id( bldg_exp ).obj();
        bool change_x = ( start.x != stop.x );
        bool change_y = ( start.y != stop.y );
        if( change_x && change_y ) {
            popup( "Construction line must be straight!" );
            return;
        }
        std::vector<tripoint> fortify_om;
        if( ( change_x && stop.x < start.x ) || ( change_y && stop.y < start.y ) ) {
            //line_to doesn't include the origin point
            fortify_om.push_back( stop );
            std::vector<tripoint> tmp_line = line_to( stop, start, 0 );
            fortify_om.insert( fortify_om.end(), tmp_line.begin(), tmp_line.end() );
        } else {
            fortify_om.push_back( start );
            std::vector<tripoint> tmp_line = line_to( start, stop, 0 );
            fortify_om.insert( fortify_om.end(), tmp_line.begin(), tmp_line.end() );
        }
        int trips = 0;
        time_duration build_time = 0_hours;
        time_duration travel_time = 0_hours;
        int dist = 0;
        for( auto fort_om : fortify_om ) {
            bool valid = false;
            oter_id &omt_ref = overmap_buffer.ter( fort_om );
            for( const std::string &pos_om : allowed_locations ) {
                if( omt_ref.id().c_str() == pos_om ) {
                    valid = true;
                    break;
                }
            }

            if( !valid ) {
                popup( _( "Invalid terrain in construction path." ) );
                return;
            }
            trips += 2;
            build_time += time_duration::from_turns( making.time / 100 );
            dist += rl_dist( fort_om.x, fort_om.y, omt_pos.x, omt_pos.y );
            travel_time += companion_travel_time_calc( fort_om, omt_pos, 0_minutes, 2 );
        }
        time_duration total_time = travel_time + build_time;
        inventory total_inv = g->u.crafting_inventory();
        int need_food = time_to_food( total_time );
        if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( total_time, build_time,
                       travel_time, dist, trips, need_food ) ) ) {
            return;
        } else if( !making.requirements().can_make_with_inventory( total_inv,
                   ( fortify_om.size() * 2 ) - 2 ) ) {
            popup( _( "You don't have the material to build the fortification." ) );
        } else if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else {
            npc_ptr comp = individual_mission( p, _( "begins constructing fortifications..." ),
                                               "_faction_camp_om_fortifications", false, {},
                                               making.skill_used.str(), making.difficulty );
            if( comp != nullptr ) {
                g->u.consume_components_for_craft( &making, ( fortify_om.size() * 2 ) - 2, true );
                g->u.invalidate_crafting_inventory();
                companion_skill_trainer( *comp, "construction", build_time, 2 );
                comp->companion_mission_time_ret = calendar::turn + total_time;
                comp->companion_mission_role_id = bldg_exp;
                for( auto pt : fortify_om ) {
                    comp->companion_mission_points.push_back( pt );
                }
            }
        }
    }
}

void talk_function::start_combat_mission( const std::string &miss, npc &p )
{
    popup( _( "Select checkpoints until you reach maximum range or select the last point again to end." ) );
    tripoint start = p.global_omt_location();
    std::vector<tripoint> scout_points = om_companion_path( start, 90, true );
    if( scout_points.empty() ) {
        return;
    }
    int dist = scout_points.size();
    int trips = 2;
    time_duration travel_time = companion_travel_time_calc( scout_points, 0_minutes, trips );
    time_duration build_time = 0_hours;
    time_duration total_time = travel_time + build_time;
    int need_food = time_to_food( total_time );
    if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( total_time, build_time,
                   travel_time, dist, trips, need_food ) ) ) {
        return;
    } else if( camp_food_supply() < need_food ) {
        popup( _( "You don't have enough food stored to feed your companion." ) );
        return;
    }
    npc_ptr comp = individual_mission( p, _( "departs on patrol..." ), miss, false, {},
                                       "survival", 3 );
    if( comp != nullptr ) {
        comp->companion_mission_points = scout_points;
        comp->companion_mission_time_ret = calendar::turn + travel_time;
    }
}

// FIXME: Refactor to check the companion list first
void basecamp::craft_construction( npc &p, const std::string &cur_id, const std::string &cur_dir,
                                   const std::string &type, const std::string &miss_id ) const
{
    std::map<std::string, std::string> recipes = recipe_deck( type );
    for( auto it = recipes.begin(); it != recipes.end(); ++it ) {
        if( cur_id == cur_dir + it->first ) {
            const recipe &making = recipe_id( it->second ).obj();
            inventory total_inv = g->u.crafting_inventory();

            if( !making.requirements().can_make_with_inventory( total_inv, 1 ) ) {
                popup( _( "You don't have the materials to craft that" ) );
                continue;
            }

            int batch_size = 1;
            string_input_popup popup_input;
            popup_input
            .title( string_format( _( "Batch crafting %s [MAX: %d]: " ), making.result_name(),
                                   talk_function::camp_recipe_batch_max( making, total_inv ) ) )
            .edit( batch_size );

            if( popup_input.canceled() || batch_size <= 0 ) {
                continue;
            }
            if( batch_size > talk_function::camp_recipe_batch_max( making, total_inv ) ) {
                popup( _( "Your batch is too large!" ) );
                continue;
            }
            npc_ptr comp = talk_function::individual_mission( p, _( "begins to work..." ),
                           miss_id + cur_dir, false, {},
                           making.skill_used.str(), making.difficulty );
            if( comp != nullptr ) {
                time_duration making_time = time_duration::from_turns( making.time / 100 ) *
                                            batch_size;
                g->u.consume_components_for_craft( &making, batch_size, true );
                g->u.invalidate_crafting_inventory();
                for( const item &results : making.create_results( batch_size ) ) {
                    comp->companion_mission_inv.add_item( results );
                }
                comp->companion_mission_time_ret = calendar::turn + making_time;
            }
        }
    }
}

bool talk_function::camp_garage_chop_start( npc &p, const std::string &task )
{
    std::string dir = camp_direction( task );
    const tripoint omt_pos = p.global_omt_location();
    tripoint omt_trg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );
    for( const auto &e : om_expansions ) {
        if( dir == om_simple_dir( omt_pos, e.second ) ) {
            omt_trg = e.second;
        }
    }

    editmap edit;
    vehicle *car = edit.mapgen_veh_query( omt_trg );
    if( car == nullptr ) {
        return false;
    }

    if( !query_yn( _( "       Chopping this vehicle:\n%s" ), camp_car_description( car ) ) ) {
        return false;
    }

    npc_ptr comp = individual_mission( p, _( "begins working in the garage..." ),
                                       "_faction_exp_chop_shop_" + dir, false, {},
                                       "mechanics", 2 );
    if( comp == nullptr ) {
        return false;
    }
    comp->companion_mission_time_ret = calendar::turn + 5_days;

    //Chopping up the car!
    std::vector<vehicle_part> p_all = car->parts;
    int prt = 0;
    int skillLevel = comp->get_skill_level( skill_mechanics );
    while( !p_all.empty() ) {
        vehicle_stack contents = car->get_items( prt );
        for( auto iter = contents.begin(); iter != contents.end(); ) {
            comp->companion_mission_inv.add_item( *iter );
            iter = contents.erase( iter );
        }
        bool broken = p_all[ prt ].is_broken();
        bool skill_break = false;
        bool skill_destroy = false;

        int dice = rng( 1, 20 );
        dice += skillLevel - p_all[ prt].info().difficulty;

        if( dice >= 20 ) {
            skill_break = false;
            skill_destroy = false;
            companion_skill_trainer( *comp, skill_mechanics, 1_hours,
                                     p_all[ prt].info().difficulty );
        } else if( dice > 15 ) {
            skill_break = false;
        } else if( dice > 9 ) {
            skill_break = true;
            skill_destroy = false;
        } else {
            skill_break = true;
            skill_destroy = true;
        }

        if( !broken && !skill_break ) {
            //Higher level garages will salvage liquids from tanks
            if( !p_all[prt].is_battery() ) {
                p_all[prt].ammo_consume( p_all[prt].ammo_capacity(),
                                         car->global_part_pos3( p_all[prt] ) );
            }
            comp->companion_mission_inv.add_item( p_all[prt].properties_to_item() );
        } else if( !skill_destroy ) {
            for( const item &itm : p_all[prt].pieces_for_broken_part() ) {
                comp->companion_mission_inv.add_item( itm );
            }
        }
        p_all.erase( p_all.begin() + 0 );
    }
    companion_skill_trainer( *comp, skill_mechanics, 5_days, 2 );
    edit.mapgen_veh_destroy( omt_trg, car );
    return true;
}

// camp faction companion mission recovery functions
void talk_function::camp_companion_return( npc &comp )
{
    time_duration mission_time = calendar::turn - comp.companion_mission_time;
    int need_food = time_to_food( mission_time );
    if( camp_food_supply() < need_food ) {
        popup( _( "Your companion seems disappointed that your pantry is empty..." ) );
    }
    int avail_food = std::min( need_food, camp_food_supply() );
    camp_food_supply( -need_food );
    companion_return( comp );
    comp.mod_hunger( -avail_food );
    // TODO: more complicated calculation?
    comp.set_thirst( 0 );
    comp.set_fatigue( 0 );
    comp.set_sleep_deprivation( 0 );
}

bool basecamp::upgrade_return( npc &p, const std::string &dir, const std::string &miss )
{
    auto e = expansions.find( dir );
    if( e == expansions.end() ) {
        return false;
    }
    const tripoint upos = e->second.pos;
    //Ensure there are no vehicles before we update
    editmap edit;
    if( edit.mapgen_veh_has( upos ) ) {
        popup( _( "Engine cannot support merging vehicles from two overmaps, please remove them from the OM tile." ) );
        return false;
    }

    const std::string bldg = next_upgrade( dir );
    if( bldg == "null" ) {
        return false;
    }
    const recipe &making = recipe_id( bldg ).obj();

    npc_ptr comp = talk_function::companion_choose_return( p, miss, calendar::before_time_starts );

    if( comp == nullptr || !om_upgrade( p, bldg, upos ) ) {
        return false;
    }
    time_duration making_time = time_duration::from_turns( making.time / 100 );
    talk_function::companion_skill_trainer( *comp, "construction", making_time,
                                            making.difficulty );
    popup( _( "%s returns from upgrading the camp having earned a bit of experience..." ),
           comp->name );
    talk_function::camp_companion_return( *comp );
    e->second.cur_level += 1;
    return true;
}

bool basecamp::menial_return( npc &p )
{
    npc_ptr comp = talk_function::companion_choose_return( p, "_faction_camp_menial",
                   calendar::before_time_starts );
    if( comp == nullptr ) {
        return false;
    }

    talk_function::companion_skill_trainer( *comp, "menial", 3_hours, 2 );

    popup( _( "%s returns from doing the dirty work to keep the camp running..." ), comp->name );
    validate_sort_points();

    tripoint p_food = sort_points[ static_cast<size_t>( sort_pt_ids::ufood ) ];
    tripoint p_seed = sort_points[ static_cast<size_t>( sort_pt_ids::seeds ) ];
    tripoint p_weapon = sort_points[ static_cast<size_t>( sort_pt_ids::weapons ) ];
    tripoint p_clothing = sort_points[ static_cast<size_t>( sort_pt_ids::clothing ) ];
    tripoint p_bionic = sort_points[ static_cast<size_t>( sort_pt_ids::bionics ) ];
    tripoint p_tool = sort_points[ static_cast<size_t>( sort_pt_ids::tools ) ];
    tripoint p_wood = sort_points[ static_cast<size_t>( sort_pt_ids::wood ) ];
    tripoint p_trash = sort_points[ static_cast<size_t>( sort_pt_ids::trash ) ];
    tripoint p_book = sort_points[ static_cast<size_t>( sort_pt_ids::books ) ];
    tripoint p_medication = sort_points[ static_cast<size_t>( sort_pt_ids::meds ) ];
    tripoint p_ammo = sort_points[ static_cast<size_t>( sort_pt_ids::ammo ) ];

    //This prevents the loop from getting stuck on the piles in the open
    for( size_t spi = 0; spi < sort_points.size() ; spi++ ) {
        if( g->m.furn( sort_points[ spi ] ) == f_null ) {
            g->m.furn_set( sort_points[ spi ], f_ash );
        }
    }
    for( const tripoint &tmp : g->m.points_in_radius( g->u.pos(), 72 ) ) {
        if( g->m.has_furn( tmp ) ) {
            continue;
        }
        for( auto &i : g->m.i_at( tmp ) ) {
            if( i.made_of( LIQUID ) ) {
                continue;
            } else if( i.is_comestible() && i.rotten() ) {
                g->m.add_item_or_charges( p_trash, i, true );
            } else if( i.is_seed() ) {
                g->m.add_item_or_charges( p_seed, i, true );
            } else if( i.is_food() ) {
                g->m.add_item_or_charges( p_food, i, true );
            } else if( i.is_corpse() ) {
                g->m.add_item_or_charges( p_trash, i, true );
            } else if( i.is_book() ) {
                g->m.add_item_or_charges( p_book, i, true );
            } else if( i.is_bionic() ) {
                g->m.add_item_or_charges( p_bionic, i, true );
            } else if( i.is_medication() ) {
                g->m.add_item_or_charges( p_medication, i, true );
            } else if( i.is_tool() ) {
                g->m.add_item_or_charges( p_tool, i, true );
            } else if( i.is_gun() ) {
                g->m.add_item_or_charges( p_weapon, i, true );
            } else if( i.is_ammo() ) {
                g->m.add_item_or_charges( p_ammo, i, true );
            } else if( i.is_armor() ) {
                g->m.add_item_or_charges( p_clothing, i, true );
            } else if( i.typeId() == "log" || i.typeId() == "splinter" || i.typeId() == "stick" ||
                       i.typeId() == "2x4" ) {
                g->m.add_item_or_charges( p_wood, i, true );
            } else {
                g->m.add_item_or_charges( p_tool, i, true );
            }
        }
        g->m.i_clear( tmp );
    }
    //Undo our hack!
    for( size_t spi = 0; spi < sort_points.size() ; spi++ ) {
        if( g->m.furn( sort_points[ spi ] ) == f_ash ) {
            g->m.furn_set( sort_points[ spi ], f_null );
        }
    }
    talk_function::camp_companion_return( *comp );
    return true;
}

bool talk_function::camp_gathering_return( npc &p, const std::string &task,
        time_duration min_time )
{
    npc_ptr comp = companion_choose_return( p, task, calendar::turn - min_time );
    if( comp == nullptr ) {
        return false;
    }

    std::string task_description = _( "gathering materials" );
    int danger = 20;
    int favor = 2;
    int threat = 10;
    std::string skill_group = "gathering";
    int skill = 2 * comp->get_skill_level( skill_survival ) + comp->per_cur;
    int checks_per_cycle = 6;
    if( task == "_faction_camp_foraging" ) {
        task_description = _( "foraging for edible plants" );
        danger = 15;
        checks_per_cycle = 12;
    } else if( task == "_faction_camp_trapping" ) {
        task_description = _( "trapping small animals" );
        favor = 1;
        danger = 15;
        skill_group = "trapping";
        skill = comp->get_skill_level( skill_traps );
        checks_per_cycle = 4;
    } else if( task == "_faction_camp_hunting" ) {
        task_description = _( "hunting for meat" );
        danger = 10;
        favor = 0;
        skill_group = "hunting";
        skill = comp->get_skill_level( skill_gun );
        threat = 12;
        checks_per_cycle = 2;
    }

    if( one_in( danger ) && !survive_random_encounter( *comp, task_description, favor, threat ) ) {
        return false;
    }

    time_duration mission_time = calendar::turn - comp->companion_mission_time;
    companion_skill_trainer( *comp, skill_group, mission_time, 1 );

    popup( _( "%s returns from %s carrying supplies and has a bit more experience..." ),
           comp->name, task_description );

    std::string itemlist = "forest";
    if( task == "_faction_camp_firewood" ) {
        itemlist = "gathering_faction_base_camp_firewood";
    } else if( task == "_faction_camp_gathering" ) {
        basecamp *bcp = get_basecamp( p );
        itemlist = bcp->get_gatherlist();
    } else if( task == "_faction_camp_foraging" ) {
        switch( season_of_year( calendar::turn ) ) {
            case SPRING:
                itemlist = "forage_faction_base_camp_spring";
                break;
            case SUMMER:
                itemlist = "forage_faction_base_camp_summer";
                break;
            case AUTUMN:
                itemlist = "forage_faction_base_camp_autumn";
                break;
            case WINTER:
                itemlist = "forage_faction_base_camp_winter";
                break;
        }
    }
    if( task == "_faction_camp_trapping" || task == "_faction_camp_hunting" ) {
        camp_hunting_results( skill, task, checks_per_cycle * mission_time / min_time, 25 );
    } else {
        camp_search_results( skill, itemlist, checks_per_cycle * mission_time / min_time, 15 );
    }

    camp_companion_return( *comp );
    return true;
}

void talk_function::camp_fortifications_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, "_faction_camp_om_fortifications",
                                            calendar::before_time_starts );
    if( comp != nullptr ) {
        popup( _( "%s returns from constructing fortifications..." ), comp->name );
        editmap edit;
        bool build_dir_NS = ( comp->companion_mission_points[0].y !=
                              comp->companion_mission_points[1].y );
        //Ensure all tiles are generated before putting fences/trenches down...
        for( auto pt : comp->companion_mission_points ) {
            if( MAPBUFFER.lookup_submap( om_to_sm_copy( pt ) ) == nullptr ) {
                oter_id &omt_test = overmap_buffer.ter( pt );
                std::string om_i = omt_test.id().c_str();
                //The thick forests will make harsh boundaries since it won't recognize these
                //tiles when they become fortifications
                if( om_i == "forest_thick" ) {
                    om_i = "forest";
                }
                edit.mapgen_set( om_i, pt, false );
            }
        }
        //Add fences
        auto build_point = comp->companion_mission_points;
        for( size_t pt = 0; pt < build_point.size(); pt++ ) {
            //First point is always at top or west since they are built in a line and sorted
            std::string build_n = "faction_wall_level_N_0";
            std::string build_e = "faction_wall_level_E_0";
            std::string build_s = "faction_wall_level_S_0";
            std::string build_w = "faction_wall_level_W_0";
            if( comp->companion_mission_role_id == "faction_wall_level_N_1" ) {
                build_n = "faction_wall_level_N_1";
                build_e = "faction_wall_level_E_1";
                build_s = "faction_wall_level_S_1";
                build_w = "faction_wall_level_W_1";
            }
            if( pt == 0 ) {
                if( build_dir_NS ) {
                    edit.mapgen_set( build_s, build_point[pt] );
                } else {
                    edit.mapgen_set( build_e, build_point[pt] );
                }
            } else if( pt == build_point.size() - 1 ) {
                if( build_dir_NS ) {
                    edit.mapgen_set( build_n, build_point[pt] );
                } else {
                    edit.mapgen_set( build_w, build_point[pt] );
                }
            } else if( build_dir_NS ) {
                edit.mapgen_set( build_n, build_point[pt] );
                edit.mapgen_set( build_s, build_point[pt] );
            } else {
                edit.mapgen_set( build_e, build_point[pt] );
                edit.mapgen_set( build_w, build_point[pt] );
            }
        }
        camp_companion_return( *comp );
    }
}

void talk_function::camp_recruit_return( npc &p, const std::string &task, int score )
{
    npc_ptr comp = companion_choose_return( p, task, calendar::turn - 4_days );
    if( comp == nullptr ) {
        return;
    }
    std::string skill_group = "recruiting";
    companion_skill_trainer( *comp, skill_group, 4_days, 2 );
    popup( _( "%s returns from searching for recruits with a bit more experience..." ),
           comp->name );
    camp_companion_return( *comp );

    npc_ptr recruit;
    //Success of finding an NPC to recruit, based on survival/tracking
    int skill = comp->get_skill_level( skill_survival );
    if( rng( 1, 20 ) + skill > 17 ) {
        recruit = std::make_shared<npc>();
        recruit->normalize();
        recruit->randomize();
        popup( _( "%s encountered %s..." ), comp->name, recruit->name );
    } else {
        popup( _( "%s didn't find anyone to recruit..." ), comp->name );
        return;
    }
    //Chance of convincing them to come back
    skill = ( 100 * comp->get_skill_level( skill_speech ) + score ) / 100;
    if( rng( 1, 20 ) + skill  > 19 ) {
        popup( _( "%s convinced %s to hear a recruitment offer from you..." ), comp->name,
               recruit->name );
    } else {
        popup( _( "%s wasn't interested in anything %s had to offer..." ), recruit->name,
               comp->name );
        return;
    }
    //Stat window
    int rec_m = 0;
    int appeal = rng( -5, 3 ) + std::min( skill / 3, 3 );
    int food_desire = rng( 0, 5 );
    while( rec_m >= 0 ) {
        std::string description = string_format( _( "NPC Overview:\n \n" ) );
        description += string_format( _( "Name:  %20s\n \n" ), recruit->name );
        description += string_format( _( "Strength:        %10d\n" ), recruit->str_max );
        description += string_format( _( "Dexterity:       %10d\n" ), recruit->dex_max );
        description += string_format( _( "Intelligence:    %10d\n" ), recruit->int_max );
        description += string_format( _( "Perception:      %10d\n \n" ), recruit->per_max );
        description += string_format( _( "Top 3 Skills:\n" ) );

        const auto skillslist = Skill::get_skills_sorted_by( [&]( Skill const & a,
        Skill const & b ) {
            int const level_a = recruit->get_skill_level( a.ident() );
            int const level_b = recruit->get_skill_level( b.ident() );
            return level_a > level_b || ( level_a == level_b && a.name() < b.name() );
        } );

        description += string_format( "%12s:          %4d\n", skillslist[0]->name(),
                                      recruit->get_skill_level( skillslist[0]->ident() ) );
        description += string_format( "%12s:          %4d\n", skillslist[1]->name(),
                                      recruit->get_skill_level( skillslist[1]->ident() ) );
        description += string_format( "%12s:          %4d\n \n", skillslist[2]->name(),
                                      recruit->get_skill_level( skillslist[2]->ident() ) );

        description += string_format( _( "Asking for:\n" ) );
        description += string_format( _( "> Food:     %10d days\n \n" ), food_desire );
        description += string_format( _( "Faction Food:%9d days\n \n" ),
                                      camp_food_supply( 0, true ) );
        description += string_format( _( "Recruit Chance: %10d%%\n \n" ),
                                      100 * ( 10.0 + appeal ) / 20, 100 );
        description += _( "Select an option:" );

        std::vector<std::string> rec_options;
        rec_options.push_back( _( "Increase Food" ) );
        rec_options.push_back( _( "Decrease Food" ) );
        rec_options.push_back( _( "Make Offer" ) );
        rec_options.push_back( _( "Not Interested" ) );

        rec_m = uilist( description, rec_options );
        if( rec_m < 0 || rec_m == 3 || static_cast<size_t>( rec_m ) >= rec_options.size() ) {
            popup( _( "You decide you aren't interested..." ) );
            return;
        }

        if( rec_m == 0 && food_desire + 1 <= camp_food_supply( 0, true ) ) {
            food_desire++;
            appeal++;
        }
        if( rec_m == 1 ) {
            if( food_desire > 0 ) {
                food_desire--;
                appeal--;
            }
        }
        if( rec_m == 2 ) {
            break;
        }
    }
    //Roll for recruitment
    if( rng( 1, 20 ) + appeal >= 10 ) {
        popup( _( "%s has been convinced to join!" ), recruit->name );
    } else {
        popup( _( "%s wasn't interested..." ), recruit->name );
        return;// nullptr;
    }
    camp_food_supply( -1_days * food_desire );
    recruit->spawn_at_precise( { g->get_levx(), g->get_levy() }, g->u.pos() + point( -4, -4 ) );
    overmap_buffer.insert_npc( recruit );
    recruit->form_opinion( g->u );
    recruit->mission = NPC_MISSION_NULL;
    recruit->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC,
                              recruit->global_omt_location(),
                              recruit->getID() ) );
    recruit->set_attitude( NPCATT_FOLLOW );
    g->load_npcs();
}

void talk_function::combat_mission_return( const std::string &miss, npc &p )
{
    npc_ptr comp = companion_choose_return( p, miss, calendar::before_time_starts );
    if( comp != nullptr ) {
        bool patrolling = miss == "_faction_camp_combat_0";
        comp_list patrol;
        npc_ptr guy = overmap_buffer.find_npc( comp->getID() );
        patrol.push_back( guy );
        for( auto pt : comp->companion_mission_points ) {
            oter_id &omt_ref = overmap_buffer.ter( pt );
            int swim = comp->get_skill_level( skill_swimming );
            if( is_river( omt_ref ) && swim < 2 ) {
                if( swim == 0 ) {
                    popup( _( "Your companion hit a river and didn't know how to swim..." ) );
                } else {
                    popup( _( "Your companion hit a river and didn't know how to swim well enough to cross..." ) );
                }
                break;
            }
            comp->death_drops = false;
            bool outcome = companion_om_combat_check( patrol, pt, patrolling );
            comp->death_drops = true;
            if( !outcome ) {
                if( comp->is_dead() ) {
                    popup( _( "%s didn't return from patrol..." ), comp->name );
                    comp->place_corpse( pt );
                    overmap_buffer.add_note( pt, "DEAD NPC" );
                    overmap_buffer.remove_npc( comp->getID() );
                    return;
                } else {
                    popup( _( "%s returns from patrol..." ), comp->name );
                    companion_return( *comp );
                    return;
                }

            }
            overmap_buffer.reveal( pt, 2 );
        }
        popup( _( "%s returns from patrol..." ), comp->name );
        camp_companion_return( *comp );
    }
}

bool basecamp::survey_return( npc &p )
{
    npc_ptr comp = talk_function::companion_choose_return( p, "_faction_camp_expansion",
                   calendar::before_time_starts );
    if( comp == nullptr ) {
        return false;
    }
    std::vector<std::string> pos_expansion_name_id;
    std::vector<std::string> pos_expansion_name;
    std::map<std::string, std::string> pos_expansions =
        recipe_group::get_recipes( "all_faction_base_expansions" );
    for( std::map<std::string, std::string>::const_iterator it = pos_expansions.begin();
         it != pos_expansions.end(); ++it ) {
        pos_expansion_name.push_back( it->first );
        pos_expansion_name_id.push_back( it->second );
    }

    const int expan = uilist( _( "Select an expansion:" ), pos_expansion_name );
    if( expan < 0 || static_cast<size_t>( expan ) >= pos_expansion_name_id.size() ) {
        popup( _( "You choose to wait..." ) );
        return false;
    }
    editmap edit;
    tripoint omt_pos = p.global_omt_location();
    if( !edit.mapgen_set( pos_expansion_name_id[expan], omt_pos, 1 ) ) {
        return false;
    }
    add_expansion( pos_expansion_name_id[expan], omt_pos );
    talk_function::companion_skill_trainer( *comp, "construction", 3_hours, 2 );
    popup( _( "%s returns from surveying for the expansion." ), comp->name );
    talk_function::camp_companion_return( *comp );
    return true;
}

bool talk_function::camp_farm_return( npc &p, const std::string &task, farm_ops op )
{
    std::string dir = camp_direction( task );
    const tripoint omt_pos = p.global_omt_location();
    tripoint omt_trg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );
    for( const auto &e : om_expansions ) {
        if( dir == om_simple_dir( omt_pos, e.second ) ) {
            omt_trg = e.second;
            break;
        }
    }
    int harvestable = 0;
    int plots_empty = 0;
    int plots_plow = 0;
    bool plow = op & farm_ops::plow;
    bool plant = op & farm_ops::plant;
    bool harvest = op & farm_ops::harvest;

    //bay_json is what the are should look like according to jsons
    tinymap bay_json;
    bay_json.generate( omt_trg.x * 2, omt_trg.y * 2, omt_trg.z, calendar::turn );
    //bay is what the area actually looks like
    tinymap bay;
    bay.load( omt_trg.x * 2, omt_trg.y * 2, omt_trg.z, false );
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            //Needs to be plowed to match json
            if( bay_json.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                ( bay.ter( x, y ) == ter_str_id( "t_dirt" ) ||
                  bay.ter( x, y ) == ter_str_id( "t_grass" ) )
                && bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_plow++;
            }
            if( bay.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_empty++;
            }
            if( bay.furn( x, y ) == furn_str_id( "f_plant_harvest" ) &&
                !bay.i_at( x, y ).empty() ) {
                const item &seed = bay.i_at( x, y )[0];
                if( seed.is_seed() ) {
                    harvestable++;
                }
            }
        }
    }
    time_duration work = 0_minutes;
    if( harvest ) {
        work += 3_minutes * harvestable;
    }
    if( plant ) {
        work += 1_minutes * plots_empty;
    }
    if( plow ) {
        work += 5_minutes * plots_plow;
    }

    npc_ptr comp = companion_choose_return( p, task, calendar::turn - work );
    if( comp == nullptr ) {
        return false;
    }

    std::vector<item *> seed_inv = comp->companion_mission_inv.items_with( []( const item & itm ) {
        return itm.is_seed() && itm.typeId() != "marloss_seed" && itm.typeId() != "fungal_seeds";
    } );

    if( plant && seed_inv.empty() ) {
        popup( _( "No seeds to plant!" ) );
    }

    //Now that we know we have spent enough time working, we can update the map itself.
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            //Needs to be plowed to match json
            if( plow && bay_json.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                ( bay.ter( x, y ) == ter_str_id( "t_dirt" ) ||
                  bay.ter( x, y ) == ter_str_id( "t_grass" ) ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                bay.ter_set( x, y, t_dirtmound );
            }
            if( plant && bay.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                if( !seed_inv.empty() ) {
                    item *tmp_seed = seed_inv.back();
                    seed_inv.pop_back();
                    std::list<item> used_seed;
                    if( tmp_seed->count_by_charges() ) {
                        used_seed.push_back( *tmp_seed );
                        tmp_seed->charges -= 1;
                        if( tmp_seed->charges > 0 ) {
                            seed_inv.push_back( tmp_seed );
                        }
                    }
                    used_seed.front().set_age( 0 );
                    bay.add_item_or_charges( x, y, used_seed.front() );
                    bay.set( x, y, t_dirt, f_plant_seed );
                }
            }
            if( harvest && bay.furn( x, y ) == furn_str_id( "f_plant_harvest" ) &&
                !bay.i_at( x, y ).empty() ) {
                const item &seed = bay.i_at( x, y )[0];
                if( seed.is_seed() && seed.typeId() != "fungal_seeds" &&
                    seed.typeId() != "marloss_seed" ) {
                    const itype &type = *seed.type;
                    int skillLevel = comp->get_skill_level( skill_survival );
                    ///\EFFECT_SURVIVAL increases number of plants harvested from a seed
                    int plantCount = rng( skillLevel / 2, skillLevel );
                    //this differs from
                    if( plantCount >= 9 ) {
                        plantCount = 9;
                    } else if( plantCount <= 0 ) {
                        plantCount = 1;
                    }
                    const int seedCount = std::max( 1l, rng( plantCount / 4, plantCount / 2 ) );
                    for( auto &i : iexamine::get_harvest_items( type, plantCount, seedCount,
                            true ) ) {
                        g->m.add_item_or_charges( g->u.posx(), g->u.posy(), i );
                    }
                    bay.i_clear( x, y );
                    bay.furn_set( x, y, f_null );
                    bay.ter_set( x, y, t_dirt );
                }
            }
        }
    }
    bay.save();

    //Give any seeds the NPC didn't use back to you.
    for( size_t i = 0; i < comp->companion_mission_inv.size(); i++ ) {
        for( const auto &it : comp->companion_mission_inv.const_stack( i ) ) {
            if( it.charges > 0 ) {
                g->u.i_add( it );
            }
        }
    }
    comp->companion_mission_inv.clear();

    companion_skill_trainer( *comp, skill_survival, work, 2 );

    popup( _( "%s returns from working your fields..." ), comp->name );
    camp_companion_return( *comp );
    return true;
}

// window manipulation
void talk_function::draw_camp_tabs( const catacurses::window &win, const camp_tab_mode cur_tab,
                                    std::vector<std::vector<mission_entry>> &entries )
{
    werase( win );
    const int width = getmaxx( win );
    mvwhline( win, 2, 0, LINE_OXOX, width );

    std::vector<std::string> tabs;
    tabs.push_back( _( "MAIN" ) );
    tabs.push_back( _( "  [N] " ) );
    tabs.push_back( _( " [NE] " ) );
    tabs.push_back( _( "  [E] " ) );
    tabs.push_back( _( " [SE] " ) );
    tabs.push_back( _( "  [S] " ) );
    tabs.push_back( _( " [SW] " ) );
    tabs.push_back( _( "  [W] " ) );
    tabs.push_back( _( " [NW] " ) );
    const int tab_step = 3;
    int tab_space = 1;
    int tab_x = 0;
    for( auto &t : tabs ) {
        bool tab_empty = entries[tab_x + 1].empty();
        draw_subtab( win, tab_space, t, tab_x == cur_tab, false, tab_empty );
        tab_space += tab_step + utf8_width( t );
        tab_x++;
    }
    wrefresh( win );
}

std::string talk_function::name_mission_tabs( npc &p, const std::string &id,
        const std::string &cur_title, camp_tab_mode cur_tab )
{
    if( id != "FACTION_CAMP" ) {
        return cur_title;
    }
    basecamp *bcp = get_basecamp( p );
    std::string dir;
    switch( cur_tab ) {
        case TAB_MAIN:
            dir = "[B]";
            break;
        case TAB_N:
            dir = "[N]";
            break;
        case TAB_NE:
            dir = "[NE]";
            break;
        case TAB_E:
            dir = "[E]";
            break;
        case TAB_SE:
            dir = "[SE]";
            break;
        case TAB_S:
            dir = "[S]";
            break;
        case TAB_SW:
            dir = "[SW]";
            break;
        case TAB_W:
            dir = "[W]";
            break;
        case TAB_NW:
            dir = "[NW]";
            break;
    }
    return bcp->expansion_tab( dir );
}

// recipes and craft support functions
int talk_function::camp_recipe_batch_max( const recipe &making, const inventory &total_inv )
{
    int max_batch = 0;
    int max_checks = 9;
    int iter = 0;
    size_t batch_size = 1000;
    time_duration making_time = time_duration::from_turns( making.time / 100 );
    while( batch_size > 0 ) {
        while( iter < max_checks ) {
            if( making.requirements().can_make_with_inventory( total_inv,
                    max_batch + batch_size ) &&
                static_cast<size_t>( camp_food_supply() ) >
                ( max_batch + batch_size ) * time_to_food( making_time ) ) {
                max_batch += batch_size;
            }
            iter++;
        }
        iter = 0;
        batch_size /= 10;
    }
    return max_batch;
}

void talk_function::camp_search_results( int skill, const Group_tag &group_id, int attempts,
        int difficulty )
{
    for( int i = 0; i < attempts; i++ ) {
        if( skill > rng( 0, difficulty ) ) {
            auto result = item_group::item_from( group_id, calendar::turn );
            if( ! result.is_null() ) {
                g->m.add_item_or_charges( g->u.pos(), result, true );
            }
        }
    }
}

void talk_function::camp_hunting_results( int skill, const std::string &task, int attempts,
        int difficulty )
{
    // no item groups for corpses, so we'll have to improvise
    weighted_int_list<mtype_id> hunting_targets;
    hunting_targets.add( mtype_id( "mon_beaver" ), 10 );
    hunting_targets.add( mtype_id( "mon_fox_red" ), 10 );
    hunting_targets.add( mtype_id( "mon_fox_gray" ), 10 );
    hunting_targets.add( mtype_id( "mon_mink" ), 5 );
    hunting_targets.add( mtype_id( "mon_muskrat" ), 10 );
    hunting_targets.add( mtype_id( "mon_otter" ), 10 );
    hunting_targets.add( mtype_id( "mon_duck" ), 10 );
    hunting_targets.add( mtype_id( "mon_cockatrice" ), 1 );
    if( task == "_faction_camp_trapping" ) {
        hunting_targets.add( mtype_id( "mon_black_rat" ), 40 );
        hunting_targets.add( mtype_id( "mon_chipmunk" ), 30 );
        hunting_targets.add( mtype_id( "mon_groundhog" ), 30 );
        hunting_targets.add( mtype_id( "mon_hare" ), 20 );
        hunting_targets.add( mtype_id( "mon_lemming" ), 40 );
        hunting_targets.add( mtype_id( "mon_opossum" ), 10 );
        hunting_targets.add( mtype_id( "mon_rabbit" ), 20 );
        hunting_targets.add( mtype_id( "mon_squirrel" ), 20 );
        hunting_targets.add( mtype_id( "mon_weasel" ), 20 );
        hunting_targets.add( mtype_id( "mon_chicken" ), 10 );
        hunting_targets.add( mtype_id( "mon_grouse" ), 10 );
        hunting_targets.add( mtype_id( "mon_pheasant" ), 10 );
        hunting_targets.add( mtype_id( "mon_turkey" ), 20 );
    } else if( task == "_faction_camp_hunting" ) {
        hunting_targets.add( mtype_id( "mon_chicken" ), 20 );
        // good luck hunting upland game birds without dogs
        hunting_targets.add( mtype_id( "mon_grouse" ), 2 );
        hunting_targets.add( mtype_id( "mon_pheasant" ), 2 );
        hunting_targets.add( mtype_id( "mon_turkey" ), 10 );
        hunting_targets.add( mtype_id( "mon_bear" ), 1 );
        hunting_targets.add( mtype_id( "mon_cougar" ), 5 );
        hunting_targets.add( mtype_id( "mon_cow" ), 1 );
        hunting_targets.add( mtype_id( "mon_coyote" ), 15 );
        hunting_targets.add( mtype_id( "mon_deer" ), 2 );
        hunting_targets.add( mtype_id( "mon_moose" ), 1 );
        hunting_targets.add( mtype_id( "mon_pig" ), 1 );
        hunting_targets.add( mtype_id( "mon_wolf" ), 10 );
    }
    for( int i = 0; i < attempts; i++ ) {
        if( skill > rng( 0, difficulty ) ) {
            const mtype_id *target = hunting_targets.pick();
            auto result = item::make_corpse( *target, calendar::turn, "" );
            if( ! result.is_null() ) {
                g->m.add_item_or_charges( g->u.pos(), result, true );
            }
        }
    }
}

// map manipulation functions
bool basecamp::om_upgrade( npc &comp, const std::string &next_upgrade, const tripoint &upos )
{
    editmap edit;
    tripoint upgrade_pos = upos;
    if( !edit.mapgen_set( next_upgrade, upgrade_pos ) ) {
        return false;
    }

    if( upos == pos ) {
        tripoint np = tripoint( g->u.posx() + 1, g->u.posy(), pos.z );
        for( const tripoint &dest : g->m.points_in_radius( comp.pos(), 2 * g->m.getmapsize() ) ) {
            if( g->m.ter( dest ) == ter_id( "t_floor_green" ) ) {
                np = dest;
                break;
            }
        }
        comp.setpos( np );
        comp.set_destination();
    }
    g->load_npcs();
    return true;
}


int talk_function::om_harvest_furn_est( npc &comp, const tripoint &omt_tgt, const furn_id &f,
                                        int chance )
{
    return om_harvest_furn( comp, omt_tgt, f, chance, true, false );
}
int talk_function::om_harvest_furn_break( npc &comp, const tripoint &omt_tgt, const furn_id &f,
        int chance )
{
    return om_harvest_furn( comp, omt_tgt, f, chance, false, false );
}
int talk_function::om_harvest_furn( npc &comp, const tripoint &omt_tgt, const furn_id &f,
                                    int chance, bool estimate, bool bring_back )
{
    const furn_t &furn_tgt = f.obj();
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z, false );
    int harvested = 0;
    int total = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.furn( p ) == f && x_in_y( chance, 100 ) ) {
            total++;
            if( estimate ) {
                continue;
            }
            if( bring_back ) {
                for( const item &itm : item_group::items_from( furn_tgt.bash.drop_group,
                        calendar::turn ) ) {
                    comp.companion_mission_inv.push_back( itm );
                }
                harvested++;
            }
            if( bring_back || comp.str_cur > furn_tgt.bash.str_min + rng( -2, 2 ) ) {
                target_bay.furn_set( p, furn_tgt.bash.furn_set );
            }
        }
    }
    target_bay.save();
    if( bring_back ) {
        return harvested;
    }
    return total;
}

int talk_function::om_harvest_ter_est( npc &comp, const tripoint &omt_tgt, const ter_id &t,
                                       int chance )
{
    return om_harvest_ter( comp, omt_tgt, t, chance, true, false );
}
int talk_function::om_harvest_ter_break( npc &comp, const tripoint &omt_tgt, const ter_id &t,
        int chance )
{
    return om_harvest_ter( comp, omt_tgt, t, chance, false, false );
}
int talk_function::om_harvest_ter( npc &comp, const tripoint &omt_tgt, const ter_id &t,
                                   int chance, bool estimate, bool bring_back )
{
    const ter_t &ter_tgt = t.obj();
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z, false );
    int harvested = 0;
    int total = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.ter( p ) == t && x_in_y( chance, 100 ) ) {
            total++;
            if( estimate ) {
                continue;
            }
            if( bring_back ) {
                for( const item &itm : item_group::items_from( ter_tgt.bash.drop_group,
                        calendar::turn ) ) {
                    comp.companion_mission_inv.push_back( itm );
                }
                harvested++;
                target_bay.ter_set( p, ter_tgt.bash.ter_set );
            }
        }
    }
    target_bay.save();
    if( bring_back ) {
        return harvested;
    }
    return total;
}

int talk_function::om_cutdown_trees_est( const tripoint &omt_tgt, int chance )
{
    return om_cutdown_trees( omt_tgt, chance, true, false );
}
int talk_function::om_cutdown_trees_logs( const tripoint &omt_tgt, int chance )
{
    return om_cutdown_trees( omt_tgt, chance, false, true );
}
int talk_function::om_cutdown_trees_trunks( const tripoint &omt_tgt, int chance )
{
    return om_cutdown_trees( omt_tgt, chance, false, false );
}
int talk_function::om_cutdown_trees( const tripoint &omt_tgt, int chance, bool estimate,
                                     bool force_cut_trunk )
{
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z, false );
    int harvested = 0;
    int total = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.ter( p ).obj().has_flag( "TREE" ) && rng( 0, 100 ) < chance ) {
            total++;
            if( estimate ) {
                continue;
            }
            // get a random number that is either 1 or -1
            int dir_x = 3 * ( 2 * rng( 0, 1 ) - 1 ) + rng( -1, 1 );
            int dir_y = 3 * rng( -1, 1 ) + rng( -1, 1 );
            tripoint to = p + tripoint( dir_x, dir_y, omt_tgt.z );
            std::vector<tripoint> tree = line_to( p, to, rng( 1, 8 ) );
            for( auto &elem : tree ) {
                target_bay.destroy( elem );
                target_bay.ter_set( elem, t_trunk );
            }
            target_bay.ter_set( p, t_dirt );
            harvested++;
        }
    }
    if( estimate ) {
        return total;
    }
    if( !force_cut_trunk ) {
        target_bay.save();
        return harvested;
    }
    // having cut down the trees, cut the trunks into logs
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.ter( p ) == ter_id( "t_trunk" ) ) {
            target_bay.ter_set( p, t_dirt );
            target_bay.spawn_item( p, "log", rng( 2, 3 ), 0, calendar::turn );
            harvested++;
        }
    }
    target_bay.save();
    return harvested;
}

std::pair<units::mass, units::volume> talk_function::om_harvest_itm( npc_ptr comp,
        const tripoint &omt_tgt, int chance, bool take )
{
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z, false );
    units::mass harvested_m = 0;
    units::volume harvested_v = 0;
    units::mass total_m = 0;
    units::volume total_v = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        for( const item &i : target_bay.i_at( p ) ) {
            total_m += i.weight( true );
            total_v += i.volume( true );
            if( take && x_in_y( chance, 100 ) ) {
                if( comp ) {
                    comp->companion_mission_inv.push_back( i );
                }
                harvested_m += i.weight( true );
                harvested_v += i.volume( true );
            }
        }
        if( take ) {
            target_bay.i_clear( p );
        }
    }
    target_bay.save();
    std::pair<units::mass, units::volume> results = { total_m, total_v };
    if( take ) {
        results = { harvested_m, harvested_v };
    }
    return results;
}

tripoint talk_function::om_target_tile( const tripoint &omt_pos, int min_range, int range,
                                        const std::vector<std::string> &possible_om_types,
                                        bool must_see, bool popup_notice, const tripoint &source,
                                        bool bounce )
{
    bool errors = false;
    if( popup_notice ) {
        popup( _( "Select a location between  %d and  %d tiles away." ), min_range, range );
    }

    std::vector<std::string> bounce_locations = { "faction_hide_site_0" };

    tripoint where;
    om_range_mark( omt_pos, range );
    om_range_mark( omt_pos, min_range, true, "Y;X: MIN RANGE" );
    if( source == tripoint( -999, -999, -999 ) ) {
        where = ui::omap::choose_point();
    } else {
        where = ui::omap::choose_point( source );
    }
    om_range_mark( omt_pos, range, false );
    om_range_mark( omt_pos, min_range, false, "Y;X: MIN RANGE" );

    if( where == overmap::invalid_tripoint ) {
        return tripoint( -999, -999, -999 );
    }
    int dist = rl_dist( where.x, where.y, omt_pos.x, omt_pos.y );
    if( dist > range || dist < min_range ) {
        popup( _( "You must select a target between %d and %d range from the base.  Range: %d" ),
               min_range, range, dist );
        errors = true;
    }

    tripoint omt_tgt = tripoint( where );

    oter_id &omt_ref = overmap_buffer.ter( omt_tgt );

    if( must_see && !overmap_buffer.seen( omt_tgt.x, omt_tgt.y, omt_tgt.z ) ) {
        errors = true;
        popup( _( "You must be able to see the target that you select." ) );
    }

    if( !errors ) {
        for( const std::string &pos_om : bounce_locations ) {
            if( bounce && omt_ref.id().c_str() == pos_om && range > 5 ) {
                if( query_yn( _( "Do you want to bounce off this location to extend range?" ) ) ) {
                    om_line_mark( omt_pos, omt_tgt );
                    tripoint dest = om_target_tile( omt_tgt, 2, range * .75, possible_om_types,
                                                    true, false, omt_tgt, true );
                    om_line_mark( omt_pos, omt_tgt, false );
                    return dest;
                }
            }
        }

        if( possible_om_types.empty() ) {
            return omt_tgt;
        }

        for( const std::string &pos_om : possible_om_types ) {
            if( omt_ref.id().c_str() == pos_om ) {
                return omt_tgt;
            }
        }
    }

    return om_target_tile( omt_pos, min_range, range, possible_om_types );
}

void talk_function::om_range_mark( const tripoint &origin, int range, bool add_notes,
                                   const std::string &message )
{
    std::vector<tripoint> note_pts;
    //North Limit
    for( int x = origin.x - range - 1; x < origin.x + range + 2; x++ ) {
        note_pts.push_back( tripoint( x, origin.y - range - 1, origin.z ) );
    }
    //South
    for( int x = origin.x - range - 1; x < origin.x + range + 2; x++ ) {
        note_pts.push_back( tripoint( x, origin.y + range + 1, origin.z ) );
    }
    //West
    for( int y = origin.y - range - 1; y < origin.y + range + 2; y++ ) {
        note_pts.push_back( tripoint( origin.x - range - 1, y, origin.z ) );
    }
    //East
    for( int y = origin.y - range - 1; y < origin.y + range + 2; y++ ) {
        note_pts.push_back( tripoint( origin.x + range + 1, y, origin.z ) );
    }

    for( auto pt : note_pts ) {
        if( add_notes ) {
            if( !overmap_buffer.has_note( pt ) ) {
                overmap_buffer.add_note( pt, message );
            }
        } else {
            if( overmap_buffer.has_note( pt ) && overmap_buffer.note( pt ) == message ) {
                overmap_buffer.delete_note( pt );
            }
        }
    }

}

void talk_function::om_line_mark( const tripoint &origin, const tripoint &dest, bool add_notes,
                                  const std::string &message )
{
    std::vector<tripoint> note_pts = line_to( origin, dest );

    for( auto pt : note_pts ) {
        if( add_notes ) {
            if( !overmap_buffer.has_note( pt ) ) {
                overmap_buffer.add_note( pt, message );
            }
        } else {
            if( overmap_buffer.has_note( pt ) && overmap_buffer.note( pt ) == message ) {
                overmap_buffer.delete_note( pt );
            }
        }
    }
}

bool talk_function::om_set_hide_site( npc &comp, const tripoint &omt_tgt,
                                      const std::vector<item *> &itms,
                                      const std::vector<item *> &itms_rem )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt );
    omt_ref = oter_id( omt_ref.id().c_str() );
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z, false );
    target_bay.ter_set( 11, 10, t_improvised_shelter );
    for( auto i : itms_rem ) {
        comp.companion_mission_inv.add_item( *i );
        target_bay.i_rem( 11, 10, i );
    }
    for( auto i : itms ) {
        target_bay.add_item_or_charges( 11, 10, *i );
        g->u.use_amount( i->typeId(), 1 );
    }
    target_bay.save();

    omt_ref = oter_id( "faction_hide_site_0" );

    overmap_buffer.reveal( point( omt_tgt.x, omt_tgt.y ), 3, 0 );
    return true;
}

// path and travel time
time_duration talk_function::companion_travel_time_calc( const tripoint &omt_pos,
        const tripoint &omt_tgt, time_duration work, int trips )
{
    std::vector<tripoint> journey = line_to( omt_pos, omt_tgt );
    return companion_travel_time_calc( journey, work, trips );
}

time_duration talk_function::companion_travel_time_calc( const std::vector<tripoint> &journey,
        time_duration work, int trips )
{
    int one_way = 0;
    for( auto &om : journey ) {
        oter_id &omt_ref = overmap_buffer.ter( om );
        std::string om_id = omt_ref.id().c_str();
        //Player walks 1 om is roughly 2.5 min
        if( om_id == "field" ) {
            one_way += 3;
        } else if( om_id == "forest" ) {
            one_way += 4;
        } else if( om_id == "forest_thick" ) {
            one_way += 5;
        } else if( om_id == "forest_water" ) {
            one_way += 6;
        } else if( is_river( omt_ref ) ) {
            one_way += 20;
        } else {
            one_way += 4;
        }
    }
    return work + one_way * trips * 1_minutes;
}

int talk_function::om_carry_weight_to_trips( units::mass mass, units::volume volume,
        units::mass carry_mass,
        units::volume carry_volume )
{
    int trips_m = 1 + mass / carry_mass;
    int trips_v = 1 + volume / carry_volume;
    // return the number of round trips
    return 2 * std::max( trips_m, trips_v );
}

int talk_function::om_carry_weight_to_trips( const std::vector<item *> &itms, npc_ptr comp )
{
    units::mass total_m = 0;
    units::volume total_v = 0;
    for( auto &i : itms ) {
        total_m += i->weight( true );
        total_v += i->volume( true );
    }
    units::mass max_m = comp ? comp->weight_capacity() - comp->weight_carried() : 30_kilogram;
    //Assume an additional pack will be carried in addition to normal gear
    units::volume sack_v = item( itype_id( "makeshift_sling" ) ).get_storage();
    units::volume max_v =  comp ? comp->volume_capacity() - comp->volume_carried() : sack_v;
    max_v += sack_v;
    return om_carry_weight_to_trips( total_m, total_v, max_m, max_v );
}

std::vector<tripoint> talk_function::om_companion_path( const tripoint &start, int range_start,
        bool bounce )
{
    std::vector<tripoint> scout_points;
    tripoint last = start;
    int range = range_start;
    int def_range = range_start;
    while( range > 3 ) {
        tripoint spt = om_target_tile( last, 0, range, {}, false, true, last, false );
        if( spt == tripoint( -999, -999, -999 ) ) {
            scout_points.clear();
            return scout_points;
        }
        if( last == spt ) {
            break;
        }
        std::vector<tripoint> note_pts = line_to( last, spt );
        scout_points.insert( scout_points.end(), note_pts.begin(), note_pts.end() );
        om_line_mark( last, spt );
        range -= rl_dist( spt.x, spt.y, last.x, last.y );
        last = spt;

        oter_id &omt_ref = overmap_buffer.ter( last );

        if( bounce && omt_ref.id() == "faction_hide_site_0" ) {
            range = def_range * .75;
            def_range = range;
        }
    }
    for( auto pt : scout_points ) {
        om_line_mark( pt, pt, false );
    }
    return scout_points;
}

// camp utility functions
void basecamp::validate_sort_points()
{
    if( sort_points.size() < COMPANION_SORT_POINTS ) {
        popup( _( "Sorting points have changed, forcing reset." ) );
        set_sort_points( true, true );
    }
}

bool basecamp::set_sort_points( bool reset_pts, bool choose_pts )
{
    std::vector<tripoint> new_pts;
    for( std::pair<const std::string, point> sort_pt : sort_point_data ) {
        tripoint default_pt = pos + sort_pt.second;
        new_pts.push_back( default_pt );
    }
    if( reset_pts ) {
        sort_points.clear();
        sort_points.insert( sort_points.end(), new_pts.begin(), new_pts.end() );
        if( !choose_pts ) {
            return true;
        }
    }
    if( choose_pts ) {
        for( size_t spi = 0; spi < new_pts.size(); spi++ ) {
            if( query_yn( string_format( _( "Reset point: %s?" ),
                                         sort_point_data[ spi ].first ) ) ) {
                const cata::optional<tripoint> where( g->look_around() );
                if( where && rl_dist( g->u.pos(), *where ) <= 20 ) {
                    new_pts[ spi ] = *where;
                }
            }
        }
    }
    std::string show_pts = _( "                    Items       New Point       Old Point\n \n" );
    for( size_t spi = 0; spi < new_pts.size(); spi++ ) {
        std::string display_new_pt = string_format( "( %d, %d, %d)", new_pts[ spi ].x,
                                     new_pts[ spi ].y, new_pts[ spi ].z );
        std::string display_old_pt;
        if( sort_points.size() == new_pts.size() ) {
            display_old_pt = string_format( "( %d, %d, %d)", sort_points[ spi ].x,
                                            sort_points[ spi ].y, sort_points[ spi ].y );
        }
        show_pts += string_format( "%25s %15s %15s    \n", sort_point_data[ spi ].first.c_str(),
                                   display_new_pt, display_old_pt );
    }
    show_pts += _( "\n \n             Save Points?" );
    if( query_yn( show_pts ) ) {
        sort_points.clear();
        sort_points.insert( sort_points.end(), new_pts.begin(), new_pts.end() );
        return true;
    }
    if( query_yn( _( "Revert to default points?" ) ) ) {
        return set_sort_points( true, false );
    }
    return false;
}

// camp analysis functions
std::vector<std::pair<std::string, tripoint>> talk_function::om_building_region( npc &p, int range,
        bool purge )
{
    const tripoint omt_pos = p.global_omt_location();
    std::vector<std::pair<std::string, tripoint>> om_camp_region;
    for( int x = -range; x <= range; x++ ) {
        for( int y = -range; y <= range; y++ ) {
            const tripoint omt_near_pos = omt_pos + point( x, y );
            oter_id &omt_rnear = overmap_buffer.ter( omt_near_pos );
            std::string om_rnear_id = omt_rnear.id().c_str();
            if( !purge || ( om_rnear_id.find( "faction_base_" ) != std::string::npos &&
                            om_rnear_id.find( "faction_base_camp" ) == std::string::npos ) ) {
                om_camp_region.push_back( std::make_pair( om_rnear_id, omt_near_pos ) );
            }
        }
    }
    return om_camp_region;
}

point talk_function::om_dir_to_offset( const std::string &dir )
{
    std::map<const std::string, point> dir2pt = { {
            { "[B]", point( 0, 0 ) },
            { "[N]", point( 0, -1 ) }, { "[S]", point( 0, 1 ) },
            { "[E]", point( 1, 0 ) }, { "[W]", point( -1, 0 ) },
            { "[NE]", point( 1, -1 ) }, { "[SE]", point( 1, 1 ) },
            { "[NW]", point( -1, -1 ) }, { "[SW]", point( -1, 1 ) }
        }
    };
    return dir2pt[ dir ];
}

std::string talk_function::om_simple_dir( const tripoint &omt_pos, const tripoint &omt_tar )
{
    std::string dir = "[";
    if( omt_tar.y < omt_pos.y ) {
        dir += "N";
    }
    if( omt_tar.y > omt_pos.y ) {
        dir += "S";
    }
    if( omt_tar.x < omt_pos.x ) {
        dir += "W";
    }
    if( omt_tar.x > omt_pos.x ) {
        dir += "E";
    }
    dir += "]";
    if( omt_tar.x == omt_pos.x && omt_tar.y == omt_pos.y ) {
        return "[B]";
    }
    return dir;
}

// mission descriptions
std::string talk_function::camp_trip_description( time_duration total_time,
        time_duration working_time, time_duration travel_time,
        int distance, int trips, int need_food )
{
    std::string entry = " \n";
    //A square is roughly 3 m
    int dist_m = distance * 24 * 3;
    if( dist_m > 1000 ) {
        entry += string_format( _( ">Distance:%15.2f (km)\n" ), dist_m / 1000.0 );
        entry += string_format( _( ">One Way: %15d (trips)\n" ), trips );
        entry += string_format( _( ">Covered: %15.2f (km)\n" ), dist_m / 1000.0 * trips );
    } else {
        entry += string_format( _( ">Distance:%15d (m)\n" ), dist_m );
        entry += string_format( _( ">One Way: %15d (trips)\n" ), trips );
        entry += string_format( _( ">Covered: %15d (m)\n" ), dist_m * trips );
    }
    entry += string_format( _( ">Travel:  %23s\n" ), to_string( travel_time ) );
    entry += string_format( _( ">Working: %23s\n" ), to_string( working_time ) );
    entry += "----                   ----\n";
    entry += string_format( _( "Total:    %23s\n" ), to_string( total_time ) );
    entry += string_format( _( "Food:     %15d (kcal)\n \n" ), 10 * need_food );
    return entry;
}

std::string talk_function::om_upgrade_description( const std::string &bldg )
{
    const recipe &making = recipe_id( bldg ).obj();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<std::string> component_print_buffer;
    int pane = FULL_SCREEN_WIDTH;
    auto tools = making.requirements().get_folded_tools_list( pane, c_white, total_inv, 1 );
    auto comps = making.requirements().get_folded_components_list( pane, c_white, total_inv, 1 );
    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp;
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    time_duration duration = time_duration::from_turns( making.time / 100 );
    comp = string_format( _( "Notes:\n%s\n \nSkill used: %s\n"
                             "Difficulty: %d\n%s \nRisk: None\nTime: %s\n" ),
                          making.description, making.skill_used.obj().name(),
                          making.difficulty, comp, to_string( duration ) );
    return comp;
}

std::string talk_function::om_craft_description( const std::string &itm )
{
    const recipe &making = recipe_id( itm ).obj();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<std::string> component_print_buffer;
    int pane = FULL_SCREEN_WIDTH;
    auto tools = making.requirements().get_folded_tools_list( pane, c_white, total_inv, 1 );
    auto comps = making.requirements().get_folded_components_list( pane, c_white, total_inv, 1 );

    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp;
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    time_duration duration = time_duration::from_turns( making.time / 100 );
    comp = string_format( _( "Skill used: %s\nDifficulty: %d\n%s\nTime: %s\n" ),
                          making.skill_used.obj().name(), making.difficulty, comp,
                          to_string( duration ) );
    return comp;
}

int basecamp::recruit_evaluation( int &sbase, int &sexpansions, int &sfaction, int &sbonus ) const
{
    auto e = expansions.find( "[B]" );
    if( e == expansions.end() ) {
        return 0;
    }
    sbase = e->second.cur_level * 5;
    sexpansions = expansions.size() * 2;

    //How could we ever starve?
    //More than 5 farms at recruiting base
    int farm = 0;
    for( const std::string &dir : directions ) {
        e = expansions.find( dir );
        if( e == expansions.end() ) {
            continue;
        }
        if( e->second.cur_level >= 0 && next_upgrade( dir ) == "null" ) {
            sexpansions += 2;
        }
        if( e->second.type == "farm" && e->second.cur_level > 0 ) {
            farm++;
        }
    }
    sfaction = std::min( talk_function::camp_food_supply() / 10000, 10 );
    sfaction += std::min( talk_function::camp_discipline() / 10, 5 );
    sfaction += std::min( talk_function::camp_morale() / 10, 5 );

    //Secret or Hidden Bonus
    //Please avoid openly discussing so that there is some mystery to the system
    sbonus = 0;
    if( farm >= 5 ) {
        sbonus += 10;
    }
    //More machine than man
    //Bionics count > 10, respect > 75
    if( g->u.my_bionics->size() > 10 && talk_function::camp_discipline() > 75 ) {
        sbonus += 10;
    }
    //Survival of the fittest
    if( g->get_npc_kill().size() > 10 ) {
        sbonus += 10;
    }
    return sbase + sexpansions + sfaction + sbonus;
}
int basecamp::recruit_evaluation() const
{
    int sbase;
    int sexpansions;
    int sfaction;
    int sbonus;
    return recruit_evaluation( sbase, sexpansions, sfaction, sbonus );
}

std::string basecamp::recruit_start( int npc_count )
{
    int sbase;
    int sexpansions;
    int sfaction;
    int sbonus;
    int total = recruit_evaluation( sbase, sexpansions, sfaction, sbonus );
    std::string desc = string_format( _( "Notes:\n"
                                         "Recruiting additional followers is very dangerous and "
                                         "expensive.  The outcome is heavily dependent on the "
                                         "skill of the "
                                         "companion you send and the appeal of your base.\n \n"
                                         "Skill used: speech\n"
                                         "Difficulty: 2 \n"
                                         "Base Score:                   +%3d%%\n"
                                         "> Expansion Bonus:            +%3d%%\n"
                                         "> Faction Bonus:              +%3d%%\n"
                                         "> Special Bonus:              +%3d%%\n \n"
                                         "Total: Skill                  +%3d%%\n \n"
                                         "Risk: High\n"
                                         "Time: 4 Days\n"
                                         "Positions: %d/1\n" ), sbase, sexpansions, sfaction,
                                      sbonus, total, npc_count );
    return desc;
}

std::string talk_function::om_gathering_description( npc &p, const std::string &bldg )
{
    std::string itemlist;
    if( item_group::group_is_defined( "gathering_" + bldg ) ) {
        itemlist = "gathering_" + bldg;
    } else {
        itemlist = "forest" ;
    }
    std::string output =
        _( "Notes: \nSend a companion to gather materials for the next camp upgrade.\n \n"
           "Skill used: survival\n"
           "Difficulty: N/A \n"
           "Gathering Possibilities:\n" );

    // Functions like the debug item group tester but only rolls 6 times so the player
    // doesn't have perfect knowledge
    std::map<std::string, int> itemnames;
    for( size_t a = 0; a < 6; a++ ) {
        const auto items = item_group::items_from( itemlist, calendar::turn );
        for( auto &it : items ) {
            itemnames[it.display_name()]++;
        }
    }
    // Invert the map to get sorting!
    std::multimap<int, std::string> itemnames2;
    for( const auto &e : itemnames ) {
        itemnames2.insert( std::pair<int, std::string>( e.second, e.first ) );
    }
    for( const auto &e : itemnames2 ) {
        output = output + "> " + e.second + "\n";
    }
    comp_list npc_list = companion_list( p, "_faction_camp_gathering" );
    output = output + _( " \nRisk: Very Low\n"
                         "Time: 3 Hours, Repeated\n"
                         "Positions: " ) + to_string( npc_list.size() ) + "/3\n";
    return output;
}

std::string talk_function::camp_farm_description( const tripoint &omt_pos, farm_ops op )
{
    std::vector<std::string> plant_names;
    int harvestable = 0;
    int plots_empty = 0;
    int plots_plow = 0;

    //bay_json is what the are should look like according to jsons
    tinymap bay_json;
    bay_json.generate( omt_pos.x * 2, omt_pos.y * 2, omt_pos.z, calendar::turn );
    //bay is what the area actually looks like
    tinymap bay;
    bay.load( omt_pos.x * 2, omt_pos.y * 2, omt_pos.z, false );
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            //Needs to be plowed to match json
            if( bay_json.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                ( bay.ter( x, y ) == ter_str_id( "t_dirt" ) ||
                  bay.ter( x, y ) == ter_str_id( "t_grass" ) ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_plow++;
            }
            if( bay.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_empty++;
            }
            if( bay.furn( x, y ) == furn_str_id( "f_plant_harvest" ) &&
                !bay.i_at( x, y ).empty() ) {
                const item &seed = bay.i_at( x, y )[0];
                if( seed.is_seed() ) {
                    harvestable++;
                    const islot_seed &seed_data = *seed.type->seed;
                    item tmp = item( seed_data.fruit_id, calendar::turn );
                    if( std::find( plant_names.begin(), plant_names.end(),
                                   tmp.type_name( 3 ) ) != plant_names.end() ) {
                        plant_names.push_back( tmp.type_name( 3 ) );
                    }
                }
            }
        }
    }

    std::string crops;
    int total_c = 0;
    for( const std::string &i : plant_names ) {
        if( total_c < 5 ) {
            crops += "\t" + i + " \n";
            total_c++;
        } else if( total_c == 5 ) {
            crops += "+ more \n";
            total_c++;
        }
    }
    std::string entry;
    if( op & farm_ops::harvest ) {
        entry += _( "Harvestable: " ) + to_string( harvestable ) + " \n" + crops;
    }
    if( op & farm_ops::plant ) {
        entry += _( "Ready for Planting: " ) + to_string( plots_empty ) + " \n";
    }
    if( op & farm_ops::plow ) {
        entry += _( "Needs Plowing: " ) + to_string( plots_plow ) + " \n";
    }
    return entry;
}

std::string talk_function::camp_car_description( vehicle *car )
{
    std::string entry = string_format( _( "Name:     %25s\n" ), car->name );
    entry += _( "----          Engines          ----\n" );
    for( const vpart_reference &vpr : car->get_any_parts( "ENGINE" ) ) {
        const vehicle_part &pt = vpr.part();
        const vpart_info &vp = pt.info();
        entry += string_format( _( "Engine:  %25s\n" ), vp.name() );
        entry += string_format( _( ">Status:  %24d%%\n" ),
                                static_cast<int>( 100 * pt.health_percent() ) );
        entry += string_format( _( ">Fuel:    %25s\n" ), vp.fuel_type );
    }
    std::map<itype_id, long> fuels = car->fuels_left();
    entry += _( "----  Fuel Storage & Battery   ----\n" );
    for( std::map<itype_id, long>::iterator it = fuels.begin(); it != fuels.end(); ++it ) {
        std::string fuel_entry = string_format( "%d/%d", car->fuel_left( it->first ),
                                                car->fuel_capacity( it->first ) );
        entry += string_format( ">%s:%*s\n", item( it->first ).tname(),
                                33 - item( it->first ).tname().length(), fuel_entry );
    }
    for( auto &pt : car->parts ) {
        if( pt.is_battery() ) {
            const vpart_info &vp = pt.info();
            entry += string_format( ">%s:%*d%%\n", vp.name(), 32 - vp.name().length(),
                                    int( 100.0 * pt.ammo_remaining() / pt.ammo_capacity() ) );
        }
    }
    entry += " \n";
    entry += _( "Estimated Chop Time:         5 Days\n" );
    return entry;
}

std::string talk_function::camp_direction( const std::string &line )
{
    return line.substr( line.find_last_of( '[' ),
                        line.find_last_of( ']' ) - line.find_last_of( '[' ) + 1 );
}

// food supply
int talk_function::camp_food_supply( int change, bool return_days )
{
    faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
    yours->food_supply += change;
    if( yours->food_supply < 0 ) {
        yours->likes_u += yours->food_supply / 500;
        yours->respects_u += yours->food_supply / 100;
        yours->food_supply = 0;
    }
    if( return_days ) {
        return 10 * yours->food_supply / 2600;
    }

    return yours->food_supply;
}

int talk_function::camp_food_supply( time_duration work )
{
    int nut_consumption = time_to_food( work );
    g->faction_manager_ptr->get( faction_id( "your_followers" ) )-> food_supply -= nut_consumption;
    return g->faction_manager_ptr->get( faction_id( "your_followers" ) )-> food_supply;
}

int talk_function::time_to_food( time_duration work )
{
    return 260 * to_hours<int>( work ) / 24;
}

// mission support
bool basecamp::distribute_food()
{
    validate_sort_points();
    tripoint p_food_stock = sort_points[ static_cast<size_t>( sort_pt_ids::cfood ) ];
    tripoint p_trash = sort_points[ static_cast<size_t>( sort_pt_ids::trash ) ];
    tripoint p_litter = pos + point( -7, 0 );
    tripoint p_tool = sort_points[ static_cast<size_t>( sort_pt_ids::tools ) ];

    if( g->m.i_at( p_food_stock ).empty() ) {
        popup( _( "No items are located at the drop point..." ) );
        return false;
    }
    double quick_rot = 0.6 + ( any_has_level( "kitchen", 4 ) ? 0.1 : 0 );
    double slow_rot = 0.8 + ( any_has_level( "kitchen", 4 ) ? 0.05 : 0 );
    int total = 0;
    std::vector<item> keep_me;
    auto initial_items = g->m.i_at( p_food_stock );
    for( auto &i : initial_items ) {
        bool track = p_trash == p_food_stock;
        if( i.is_container() && i.get_contained().is_food() ) {
            auto comest = i.get_contained();
            i.contents.clear();
            //NPCs are lazy bastards who leave empties all around the camp fire
            tripoint litter_spread = p_litter;
            litter_spread.x += rng( -3, 3 );
            litter_spread.y += rng( -3, 3 );
            i.on_contents_changed();
            g->m.add_item_or_charges( litter_spread, i, false );
            i = comest;
            track = false;
        }
        if( i.is_comestible() && ( i.rotten() || i.type->comestible->fun < -6 ) ) {
            if( track ) {
                keep_me.push_back( i );
            } else {
                g->m.add_item_or_charges( p_trash, i, false );
            }
        } else if( i.is_food() ) {
            double rot_multip;
            int rots_in = to_days<int>( time_duration::from_turns( i.spoilage_sort_order() ) );
            if( rots_in >= 5 ) {
                rot_multip = 1.00;
            } else if( rots_in >= 2 ) {
                rot_multip = slow_rot;
            } else {
                rot_multip = quick_rot;
            }
            if( i.count_by_charges() ) {
                total += i.type->comestible->nutr * i.charges * rot_multip;
            } else {
                total += i.type->comestible->nutr * rot_multip;
            }
        } else if( i.is_corpse() ) {
            if( track ) {
                keep_me.push_back( i );
            } else {
                g->m.add_item_or_charges( p_trash, i, false );
            }
        } else {
            if( p_tool == p_food_stock ) {
                keep_me.push_back( i );
            } else {
                g->m.add_item_or_charges( p_tool, i, false );
            }
        }
    }
    g->m.i_clear( p_food_stock );
    for( auto &i : keep_me ) {
        g->m.add_item_or_charges( p_food_stock, i, false );
    }

    popup( _( "You distribute %d kcal worth of food to your companions." ), total * 10 );
    talk_function::camp_food_supply( total );
    return true;
}

// morale
int talk_function::camp_discipline( int change )
{
    faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
    yours->respects_u += change;
    return yours->respects_u;
}

int talk_function::camp_morale( int change )
{
    faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
    yours->likes_u += change;
    return yours->likes_u;
}

// combat and danger
// this entire system is stupid
bool talk_function::survive_random_encounter( npc &comp, std::string &situation, int favor,
        int threat )
{
    popup( _( "While %s, a silent specter approaches %s..." ), situation, comp.name );
    int skill_1 = comp.get_skill_level( skill_survival );
    int skill_2 = comp.get_skill_level( skill_speech );
    if( skill_1 + favor > rng( 0, 10 ) ) {
        popup( _( "%s notices the antlered horror and slips away before it gets too close." ),
               comp.name );
        companion_skill_trainer( comp, "gathering", 10_minutes, 10 - favor );
    } else if( skill_2 + favor > rng( 0, 10 ) ) {
        popup( _( "Another survivor approaches %s asking for directions." ), comp.name );
        popup( _( "Fearful that he may be an agent of some hostile faction, %s doesn't mention the camp." ),
               comp.name );
        popup( _( "The two part on friendly terms and the survivor isn't seen again." ) );
        companion_skill_trainer( comp, "recruiting", 10_minutes, 10 - favor );
    } else {
        popup( _( "%s didn't detect the ambush until it was too late!" ), comp.name );
        int skill = comp.get_skill_level( skill_melee ) +
                    0.5 * comp.get_skill_level( skill_survival ) +
                    comp.get_skill_level( skill_bashing ) +
                    comp.get_skill_level( skill_cutting ) +
                    comp.get_skill_level( skill_stabbing ) +
                    comp.get_skill_level( skill_unarmed ) + comp.get_skill_level( skill_dodge );
        int monsters = rng( 0, threat );
        if( skill * rng( 8, 12 ) > ( monsters * rng( 8, 12 ) ) ) {
            if( one_in( 2 ) ) {
                popup( _( "The bull moose charged %s from the tree line..." ), comp.name );
                popup( _( "Despite being caught off guard %s was able to run away until the moose gave up pursuit." ),
                       comp.name );
            } else {
                popup( _( "The jabberwock grabbed %s by the arm from behind and began to scream." ),
                       comp.name );
                popup( _( "Terrified, %s spun around and delivered a massive kick to the creature's torso..." ),
                       comp.name );
                popup( _( "Collapsing into a pile of gore, %s walked away unscathed..." ), comp.name );
                popup( _( "(Sounds like bullshit, you wonder what really happened.)" ) );
            }
            companion_skill_trainer( comp, "combat", 10_minutes, 10 - favor );
        } else {
            if( one_in( 2 ) ) {
                popup( _( "%s turned to find the hideous black eyes of a giant wasp staring back from only a few feet away..." ),
                       comp.name );
                popup( _( "The screams were terrifying, there was nothing anyone could do." ) );
            } else {
                popup( _( "Pieces of %s were found strewn across a few bushes." ), comp.name );
                popup( _( "(You wonder if your companions are fit to work on their own...)" ) );
            }
            overmap_buffer.remove_npc( comp.getID() );
            return false;
        }
    }
    return true;
}
