#ifndef _MAPDATA_H_
#define _MAPDATA_H_

#include <vector>
#include <string>
#include "color.h"
#include "item.h"
#include "trap.h"
#include "monster.h"
#include "enums.h"
#include "computer.h"
#include "vehicle.h"
#include "graffiti.h"
#include "basecamp.h"
#include "iexamine.h"
#include "field.h"
#include <iosfwd>

class game;
class monster;

//More importantly: SEEX defines the size of a nonant, or grid. Same with SEEY.
#ifndef SEEX 	// SEEX is how far the player can see in the X direction (at
#define SEEX 12	// least, without scrolling).  All map segments will need to be
#endif		// at least this wide. The map therefore needs to be 3x as wide.

#ifndef SEEY	// Same as SEEX
#define SEEY 12 // Requires 2*SEEY+1= 25 vertical squares
#endif	        // Nuts to 80x24 terms. Mostly exists in graphical clients, and
	        // those fatcats can resize.

// mfb(t_flag) converts a flag to a bit for insertion into a bitfield
#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

/*
Enum: t_flag
The terrain flag bitfield defines all of the metadata for a single tile of terrain.
Use in conjunction with the mfb macro to test for existence.
*/
enum t_flag {
 transparent = 0,// Player & monsters can see through/past it
                 // Should be both on ter and furn to be transparent
 flat,    // Player can build and move furniture on
 bashable,     // Player & monsters can bash this & make it the next in the list
 container,    // Items on this square are hidden until looted by the player
 place_item,   // Valid terrain for place_item() to put items on
 door,         // Can be opened--used for NPC pathfinding.
 flammable,    // May be lit on fire
 l_flammable,  // Harder to light on fire, but still possible
 explodes,     // Explodes when on fire
 diggable,     // Digging monsters, seeding monsters, digging w/ shovel, etc.
 liquid,       // Blocks movement but isn't a wall, e.g. lava or water
 swimmable,    // You (and monsters) swim here
 sharp,	       // May do minor damage to players/monsters passing it
 painful,      // May cause a small amount of pain
 rough,        // May hurt the player's feet
 sealed,       // Can't 'e' to retrieve items here
 noitem,       // Items "fall off" this space
 destroy_item,  // Items are destroyed by this tile
 goes_down,    // Can '>' to go down a level
 goes_up,      // Can '<' to go up a level
 console,      // Used as a computer
 alarmed,      // Sets off an alarm if smashed
 supports_roof,// Used as a boundary for roof construction
               // can also knock down adjacent supports_roof and collapses tiles when destroyed
 indoors,      // Has a roof ovr it, therefore blocks rain, sunlight, etc.
 thin_obstacle,// passable by player and monsters, but not by vehicles
 collapses,    // Tiles that have a roof over them (which can collapse)
 flammable2,   // Burn to ash rather than rubble.
 deconstruct,  // Can be deconstructed
 reduce_scent, // Reduces the scent even more, only works if object is bashable as well
 fire_container,// Contains fire like brazier or wood stove.
 suppress_smoke, // Prevents smoke from being generated by fires. Used for ventilated tiles such as wood stoves.
 num_t_flags   // MUST be last
};

/*
Struct ter_t:
Short for terrain type. This struct defines all of the metadata for a given terrain id (an enum below).

*/
struct ter_t {
 std::string name; //The plaintext name of the terrain type the user would see (IE: dirt)

 /*
 The symbol drawn on the screen for the terrain. Please note that there are extensive rules as to which
 possible object/field/entity in a single square gets drawn and that some symbols are "reserved" such as * and % to do programmatic behavior.
 */
 long sym;

 nc_color color;//The color the sym will draw in on the GUI.
 unsigned char movecost; //The amount of movement points required to pass this terrain by default.
 trap_id trap; //The id of the trap located at this terrain. Limit one trap per tile currently.
 unsigned long flags;// : num_t_flags; This refers to enum t_flag defined above.
 void (iexamine::*examine)(game *, player *, map *m, int examx, int examy); //What should be examined
};

/*
enum: ter_id
Terrain id refers to a position in the terlist[] area describing, in the order of the enum, the terrain in question
through the use of a ter_t struct.
*/
enum ter_id {
t_null = 0,
t_hole,	// Real nothingness; makes you fall a z-level
// Ground
t_dirt, t_sand, t_dirtmound, t_pit_shallow, t_pit,
t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered,
t_rock_floor, t_rubble, t_ash, t_metal, t_wreckage,
t_grass,
t_metal_floor,
t_pavement, t_pavement_y, t_sidewalk,
t_floor,
t_dirtfloor,//Dirt floor(Has roof)
t_grate,
t_slime,
t_bridge,
// Lighting related
t_skylight, t_emergency_light_flicker, t_emergency_light,
// Walls
t_wall_log_half, t_wall_log, t_wall_log_chipped, t_wall_log_broken, t_palisade, t_palisade_gate, t_palisade_gate_o,
t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
t_wall_v, t_wall_h, t_concrete_v, t_concrete_h,
t_wall_metal_v, t_wall_metal_h,
t_wall_glass_v, t_wall_glass_h,
t_wall_glass_v_alarm, t_wall_glass_h_alarm,
t_reinforced_glass_v, t_reinforced_glass_h,
t_bars,
t_door_c, t_door_b, t_door_o, t_door_locked_interior, t_door_locked, t_door_locked_alarm, t_door_frame,
t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o, t_door_boarded,
t_door_metal_c, t_door_metal_o, t_door_metal_locked,
t_door_bar_c, t_door_bar_o, t_door_bar_locked,
t_door_glass_c, t_door_glass_o,
t_portcullis,
t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open, t_curtains,
t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
t_window_stained_green, t_window_stained_red, t_window_stained_blue,
t_rock, t_fault,
t_paper,
// Tree
t_tree, t_tree_young, t_tree_apple, t_underbrush, t_shrub, t_shrub_blueberry, t_shrub_strawberry, t_trunk,
t_root_wall,
t_wax, t_floor_wax,
t_fence_v, t_fence_h, t_chainfence_v, t_chainfence_h, t_chainfence_posts,
t_fence_post, t_fence_wire, t_fence_barbed, t_fence_rope,
t_railing_v, t_railing_h,
// Nether
t_marloss, t_fungus, t_tree_fungal,
// Water, lava, etc.
t_water_sh, t_water_dp, t_water_pool, t_sewage,
t_lava,
// More embellishments than you can shake a stick at.
t_sandbox, t_slide, t_monkey_bars, t_backboard,
t_gas_pump, t_gas_pump_smashed,
t_generator_broken,
t_missile, t_missile_exploded,
t_radio_tower, t_radio_controls,
t_console_broken, t_console, t_gates_mech_control, t_gates_control_concrete, t_barndoor, t_palisade_pulley,
t_sewage_pipe, t_sewage_pump,
t_centrifuge,
t_column,
t_vat,
// Staircases etc.
t_stairs_down, t_stairs_up, t_manhole, t_ladder_up, t_ladder_down, t_slope_down,
 t_slope_up, t_rope_up,
t_manhole_cover,
// Special
t_card_science, t_card_military, t_card_reader_broken, t_slot_machine,
 t_elevator_control, t_elevator_control_off, t_elevator, t_pedestal_wyrm,
 t_pedestal_temple,
// Temple tiles
t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue,
 t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even,
t_tombstone,
num_terrain_types
};

/*
The terrain list contains the master list of  information and metadata for a given type of terrain.
This is a good candidate for moving into JSON.
*/
const ter_t terlist[num_terrain_types] = {  // MUST match enum ter_id above!
{"nothing",             ' ', c_white,   2, tr_null,
    mfb(transparent)|mfb(diggable), &iexamine::none},
{"empty space",         ' ', c_black,   2, tr_ledge,
    mfb(transparent), &iexamine::none},
{"dirt",	            '.', c_brown,   2, tr_null,
    mfb(transparent)|mfb(diggable)|mfb(flat), &iexamine::none},
{"sand",	            '.', c_yellow,  2, tr_null,
    mfb(transparent)|mfb(diggable)|mfb(flat), &iexamine::none},
{"mound of dirt",       '#', c_brown,   3, tr_null,
    mfb(transparent)|mfb(diggable), &iexamine::none},
{"shallow pit",	        '0', c_yellow,  8, tr_null,
    mfb(transparent)|mfb(diggable), &iexamine::none},
{"pit",                 '0', c_brown,   10, tr_pit,
    mfb(transparent)|mfb(diggable), &iexamine::pit},
{"corpse filled pit",   '#', c_green,   5, tr_null,
    mfb(transparent)|mfb(diggable), &iexamine::none},
{"covered pit",         '#', c_ltred,   2, tr_null,
    mfb(transparent), &iexamine::pit_covered},
{"spiked pit",          '0', c_ltred,  10, tr_spike_pit,
    mfb(transparent)|mfb(diggable), &iexamine::pit},
{"covered spiked pit",  '#', c_ltred,   2, tr_null,
    mfb(transparent), &iexamine::pit_covered},
{"rock floor",          '.', c_ltgray,  2, tr_null,
    mfb(transparent)|mfb(indoors)|mfb(flat), &iexamine::none},
{"pile of rubble",      '^', c_ltgray,  4, tr_null,
    mfb(transparent)|mfb(rough)|mfb(diggable), &iexamine::rubble},
{"pile of ash",         '#', c_ltgray,  2, tr_null,
    mfb(transparent)|mfb(diggable), &iexamine::rubble},
{"twisted metal",       '#', c_cyan,    5, tr_null,
    mfb(transparent)|mfb(rough)|mfb(sharp)|mfb(place_item), &iexamine::wreckage},
{"metal wreckage",      '#', c_cyan,    5, tr_null,
    mfb(transparent)|mfb(rough)|mfb(sharp)|mfb(place_item), &iexamine::wreckage},
{"grass",	            '.', c_green,   2, tr_null,
    mfb(transparent)|mfb(diggable)|mfb(flat), &iexamine::none},
{"metal floor",         '.', c_ltcyan,  2, tr_null,
    mfb(transparent)|mfb(indoors)|mfb(flat), &iexamine::none},
{"pavement",	        '.', c_dkgray,  2, tr_null,
    mfb(transparent)|mfb(flat), &iexamine::none},
{"yellow pavement",     '.', c_yellow,  2, tr_null,
    mfb(transparent)|mfb(flat), &iexamine::none},
{"sidewalk",            '.', c_ltgray,  2, tr_null,
    mfb(transparent)|mfb(flat), &iexamine::none},
{"floor",	            '.', c_cyan,    2, tr_null,
    mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)|mfb(indoors)|mfb(flat),
    &iexamine::none},
{"dirt floor",	        '.', c_brown,   2, tr_null,  //Dirt Floor, must have roofs!
    mfb(transparent)|mfb(diggable)|mfb(supports_roof)|mfb(collapses)|mfb(indoors)|mfb(flat),
    &iexamine::none},
{"metal grate",         '#', c_dkgray,  2, tr_null,
    mfb(transparent), &iexamine::none},
{"slime",               '~', c_green,   6, tr_null,
    mfb(transparent)|mfb(container)|mfb(flammable2)|mfb(place_item), &iexamine::none},
{"walkway",             '#', c_yellow,  2, tr_null,
    mfb(transparent), &iexamine::none},
{"floor",	            '.', c_white,   2, tr_null, // Skylight
    mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)|mfb(flat), &iexamine::none},
{"floor",	            '.', c_white,   2, tr_null,
    mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)|mfb(indoors)|mfb(flat),
    &iexamine::none}, // Emergency Light
{"floor",	            '.', c_white,   2, tr_null,
    mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)|mfb(indoors)|mfb(flat),
    &iexamine::none}, // Regular Light
{"half-built wall",     '#', c_brown,   4, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem), &iexamine::none},
{"log wall",            '#', c_brown,   0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"chipped log wall",    '#', c_brown,   0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"broken log wall",     '&', c_brown,   0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)|
    mfb(supports_roof), &iexamine::none},
{"palisade wall",       '#', c_brown,   0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)|mfb(transparent),
    &iexamine::none},
{"palisade gate",       '+', c_ltred,   0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)|mfb(door)|mfb(transparent),
    &iexamine::none},
{"dirt",                '.', c_brown,   2, tr_null,
    mfb(transparent)|mfb(flammable)|mfb(supports_roof)|mfb(flat), &iexamine::none}, // Open palisade gate
{"half-built wall",     '#', c_ltred,   4, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem), &iexamine::none},
{"wooden wall",         '#', c_ltred,   0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"chipped wood wall",   '#', c_ltred,   0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"broken wood wall",    '&', c_ltred,   0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)|
    mfb(supports_roof), &iexamine::none},
{"wall",          LINE_XOXO, c_ltgray,  0, tr_null,
    mfb(flammable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"wall",          LINE_OXOX, c_ltgray,  0, tr_null,
    mfb(flammable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"concrete wall", LINE_XOXO, c_dkgray,  0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"concrete wall", LINE_OXOX, c_dkgray,  0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"metal wall",    LINE_XOXO, c_cyan,    0, tr_null,
    mfb(noitem)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"metal wall",    LINE_OXOX, c_cyan,    0, tr_null,
    mfb(noitem)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"glass wall",    LINE_XOXO, c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"glass wall",    LINE_OXOX, c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"glass wall",    LINE_XOXO, c_ltcyan,  0, tr_null, // Alarmed
    mfb(transparent)|mfb(bashable)|mfb(alarmed)|mfb(noitem)|
    mfb(supports_roof), &iexamine::none},
{"glass wall",    LINE_OXOX, c_ltcyan,  0, tr_null, // Alarmed
    mfb(transparent)|mfb(bashable)|mfb(alarmed)|mfb(noitem)|
    mfb(supports_roof), &iexamine::none},
{"reinforced glass", LINE_XOXO, c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"reinforced glass", LINE_OXOX, c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"metal bars",          '"', c_ltgray,  0, tr_null,
    mfb(transparent)|mfb(noitem), &iexamine::none},
{"closed wood door",    '+', c_brown,   0, tr_null,
    mfb(bashable)|mfb(flammable2)|mfb(door)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"damaged wood door",   '&', c_brown,   0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)|
    mfb(supports_roof), &iexamine::none},
{"open wood door",      '\'', c_brown,  2, tr_null,
    mfb(flammable2)|mfb(transparent)|mfb(supports_roof)|mfb(flat), &iexamine::none},
{"closed wood door",    '+', c_brown,   0, tr_null,	// Locked from all directions
    mfb(bashable)|mfb(flammable2)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"closed wood door",    '+', c_brown,   0, tr_null,	// Actually locked
    mfb(bashable)|mfb(flammable2)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"closed wood door",    '+', c_brown,   0, tr_null, // Locked and alarmed
    mfb(bashable)|mfb(flammable2)|mfb(alarmed)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"empty door frame",    '.', c_brown,   2, tr_null,
    mfb(transparent)|mfb(supports_roof)|mfb(flat), &iexamine::none},
{"locked wire gate",    '+', c_cyan,    0, tr_null,
    mfb(transparent)|mfb(supports_roof), &iexamine::none},
{"closed wooden gate",  '+', c_brown,   3, tr_null,
    mfb(transparent)|mfb(supports_roof) |mfb(bashable)|mfb(flammable2), &iexamine::none},
{"open wooden gate",    '.', c_brown,   2, tr_null,
    mfb(transparent)|mfb(supports_roof) |mfb(bashable)|mfb(flammable2)|mfb(flat), &iexamine::none},
{"closed wire gate",    '+', c_cyan,    0, tr_null,
    mfb(transparent)|mfb(supports_roof), &iexamine::none},
{"open wire gate",      '.', c_cyan,    2, tr_null,
    mfb(transparent)|mfb(supports_roof)|mfb(flat), &iexamine::none},
{"boarded up door",     '#', c_brown,   0, tr_null,
    mfb(bashable)|mfb(flammable2)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"closed metal door",   '+', c_cyan,    0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"open metal door",     '\'', c_cyan,   2, tr_null,
    mfb(transparent)|mfb(supports_roof)|mfb(flat), &iexamine::none},
{"closed metal door",   '+', c_cyan,    0, tr_null, // Actually locked
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"closed bar door",     '+', c_cyan,    0, tr_null,// unlocked, only created at map gen
    mfb(transparent)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"open bar door",       '\'', c_cyan,   2, tr_null,
    mfb(transparent)|mfb(supports_roof)|mfb(flat), &iexamine::none},
{"closed bar door",     '+', c_cyan,    0, tr_null, // locked
    mfb(transparent)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"closed glass door",   '+', c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(door)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"open glass door",     '\'', c_ltcyan, 2, tr_null,
    mfb(transparent)|mfb(supports_roof)|mfb(flat), &iexamine::none},
{"makeshift portcullis", '&', c_cyan,   0, tr_null,
    mfb(noitem), &iexamine::none},
{"steel compactor",     '&', c_green,   0, tr_null,
    mfb(transparent), &iexamine::recycler},
{"window",	            '"', c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable)|mfb(noitem)|
    mfb(supports_roof)|mfb(deconstruct), &iexamine::none}, // Plain Ol' window
{"taped window",        '"', c_dkgray,  0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)| mfb(reduce_scent),
    &iexamine::none}, // Regular window
{"window",	            '"', c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable)|mfb(noitem)|
    mfb(supports_roof)|mfb(deconstruct), &iexamine::none}, //has curtains
{"taped window",        '"', c_dkgray,  0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)| mfb(reduce_scent),
    &iexamine::none}, // Curtain window
{"open window",         '\'', c_ltcyan, 4, tr_null,
    mfb(transparent)|mfb(flammable)|mfb(noitem)| mfb(supports_roof), &iexamine::none},
{"closed curtains",     '"', c_dkgray,  0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof), &iexamine::none},
{"window",	            '"', c_ltcyan,  0, tr_null, // Actually alarmed
    mfb(transparent)|mfb(bashable)|mfb(flammable)|mfb(alarmed)|mfb(noitem)|
    mfb(supports_roof), &iexamine::none},
{"taped window",        '"', c_dkgray,  0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)|mfb(alarmed)|
    mfb(reduce_scent), &iexamine::none}, //Alarmed, duh.
{"empty window",        '0', c_yellow,  8, tr_null,
    mfb(transparent)|mfb(flammable)|mfb(supports_roof), &iexamine::none},
{"window frame",        '0', c_ltcyan,  8, tr_null,
    mfb(transparent)|mfb(sharp)|mfb(flammable)|mfb(noitem)|
    mfb(supports_roof), &iexamine::none},
{"boarded up window",   '#', c_brown,   0, tr_null,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"high stained glass window", '"', c_ltgreen,  0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"high stained glass window", '"', c_ltred,  0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"high stained glass window", '"', c_ltblue,  0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"solid rock",          '#', c_white,   0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"odd fault",           '#', c_magenta, 0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::fault},
{"paper wall",          '#', c_white,   0, tr_null,
    mfb(bashable)|mfb(flammable2)|mfb(noitem), &iexamine::none},
{"tree",	            '7', c_green,   0, tr_null,
    mfb(flammable2)|mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"young tree",          '1', c_green,   4, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem), &iexamine::none},
{"apple tree",          '7', c_ltgreen, 0, tr_null,
    mfb(flammable2)|mfb(noitem)|mfb(supports_roof), &iexamine::tree_apple},
{"underbrush",          '#', c_ltgreen, 6, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(diggable)|mfb(container)|
    mfb(flammable2)|mfb(thin_obstacle)|mfb(place_item), &iexamine::shrub_wildveggies},
{"shrub",               '#', c_green,   8, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(container)|mfb(flammable2)|
    mfb(thin_obstacle)|mfb(place_item), &iexamine::none},
{"blueberry bush",      '#', c_ltgreen, 8, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(container)|mfb(flammable2)|mfb(thin_obstacle),
    &iexamine::shrub_blueberry},
{"strawberry bush",     '#', c_ltgreen, 8, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(container)|mfb(flammable2)|mfb(thin_obstacle),
    &iexamine::shrub_strawberry},
{"tree trunk",          '1', c_brown,   4, tr_null,
	   mfb(transparent)|mfb(flammable2)|mfb(diggable), &iexamine::none},
{"root wall",           '#', c_brown,   0, tr_null,
    mfb(noitem)|mfb(supports_roof), &iexamine::none},
{"wax wall",            '#', c_yellow,  0, tr_null,
    mfb(flammable2)|mfb(noitem)|mfb(supports_roof)|mfb(place_item), &iexamine::none},
{"wax floor",           '.', c_yellow,  2, tr_null,
    mfb(transparent)|mfb(l_flammable)|mfb(indoors)|mfb(flat), &iexamine::none},
{"picket fence",        '|', c_brown,   3, tr_null,
    mfb(bashable)|mfb(transparent)|mfb(diggable)|mfb(flammable2)|mfb(noitem)|mfb(thin_obstacle),
    &iexamine::none},
{"picket fence",        '-', c_brown,   3, tr_null,
    mfb(bashable)|mfb(transparent)|mfb(diggable)|mfb(flammable2)|mfb(noitem)|mfb(thin_obstacle),
    &iexamine::none},
{"chain link fence",    '|', c_cyan,    0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(thin_obstacle)|mfb(supports_roof), &iexamine::chainfence},
{"chain link fence",    '-', c_cyan,    0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(thin_obstacle)|mfb(supports_roof), &iexamine::chainfence},
{"metal post",          '#', c_cyan,    2, tr_null,
    mfb(transparent)|mfb(thin_obstacle), &iexamine::none},
{"fence post",          '#', c_brown,   2, tr_null,
    mfb(transparent)|mfb(thin_obstacle), &iexamine::fence_post},
{"wire fence",          '$', c_blue,    4, tr_null,
    mfb(transparent)|mfb(thin_obstacle), &iexamine::remove_fence_wire},
{"barbed wire fence",   '$', c_blue,    4, tr_null,
    mfb(transparent)|mfb(sharp)|mfb(thin_obstacle), &iexamine::remove_fence_barbed},
{"rope fence",          '$', c_brown,   3, tr_null,
    mfb(transparent)|mfb(thin_obstacle), &iexamine::remove_fence_rope},
{"railing",             '|', c_yellow,  3, tr_null,
   	mfb(transparent)|mfb(noitem)|mfb(thin_obstacle), &iexamine::none},
{"railing",             '-', c_yellow,  3, tr_null,
   	mfb(transparent)|mfb(noitem)|mfb(thin_obstacle), &iexamine::none},
{"marloss bush",        '1', c_dkgray,  0, tr_null,
	   mfb(transparent)|mfb(bashable)|mfb(flammable2), &iexamine::none},
{"fungal bed",          '#', c_dkgray,  3, tr_null,
	   mfb(transparent)|mfb(flammable2)|mfb(diggable), &iexamine::none},
{"fungal tree",         '7', c_dkgray,  0, tr_null,
	   mfb(flammable2)|mfb(noitem), &iexamine::none},
{"shallow water",       '~', c_ltblue,  5, tr_null,
	   mfb(transparent)|mfb(liquid)|mfb(swimmable), &iexamine::water_source},
{"deep water",          '~', c_blue,    0, tr_null,
	   mfb(transparent)|mfb(liquid)|mfb(swimmable), &iexamine::water_source},
{"pool water",          '~', c_ltblue,  5, tr_null,
    mfb(transparent)|mfb(liquid)|mfb(swimmable)|mfb(indoors),
    &iexamine::water_source},
{"sewage",              '~', c_ltgreen, 6, tr_null,
	   mfb(transparent)|mfb(swimmable), &iexamine::water_source},
{"lava",                '~', c_red,     4, tr_lava,
        mfb(transparent)|mfb(liquid)|mfb(destroy_item), &iexamine::none},
{"sandbox",             '#', c_yellow,  3, tr_null,
   	mfb(transparent)|mfb(deconstruct), &iexamine::none},
{"slide",               '#', c_ltcyan,  4, tr_null,
   	mfb(transparent)|mfb(deconstruct), &iexamine::none},
{"monkey bars",         '#', c_cyan,    4, tr_null,
   	mfb(transparent)|mfb(deconstruct), &iexamine::none},
{"backboard",           '7', c_red,     0, tr_null,
	   mfb(transparent)|mfb(deconstruct), &iexamine::none},
{"gasoline pump",       '&', c_red,     0, tr_null,
	   mfb(transparent)|mfb(explodes)|mfb(noitem)|mfb(sealed)|mfb(container), &iexamine::gaspump},
{"smashed gas pump",    '&', c_ltred,   0, tr_null,
	   mfb(transparent)|mfb(noitem), &iexamine::none},
{"broken generator",    '&', c_ltgray,  0, tr_null,
	   mfb(transparent)|mfb(noitem), &iexamine::none},
{"missile",             '#', c_ltblue,  0, tr_null,
	   mfb(explodes)|mfb(noitem), &iexamine::none},
{"blown-out missile",   '#', c_ltgray,  0, tr_null,
	   mfb(noitem), &iexamine::none},
{"radio tower",         '&', c_ltgray,  0, tr_null,
	   mfb(noitem), &iexamine::none},
{"radio controls",      '6', c_green,   0, tr_null,
	   mfb(transparent)|mfb(bashable)|mfb(noitem), &iexamine::none},
{"broken console",      '6', c_ltgray,  0, tr_null,
	   mfb(transparent)|mfb(noitem)|mfb(collapses)|mfb(indoors), &iexamine::none},
{"computer console",    '6', c_blue,    0, tr_null,
	   mfb(transparent)|mfb(console)|mfb(noitem)|mfb(collapses)|mfb(indoors), &iexamine::none},
{"mechanical winch",    '6', c_cyan_red, 0, tr_null,
    mfb(transparent)|mfb(noitem)|mfb(collapses), &iexamine::controls_gate},
{"mechanical winch",    '6', c_cyan_red, 0, tr_null,
    mfb(transparent)|mfb(noitem)|mfb(collapses), &iexamine::controls_gate},
{"rope and pulley",     '|', c_brown,   0, tr_null,
    mfb(transparent)|mfb(noitem)|mfb(collapses), &iexamine::controls_gate},
{"rope and pulley",     '|', c_brown,   0, tr_null,
    mfb(transparent)|mfb(noitem)|mfb(collapses), &iexamine::controls_gate},
{"sewage pipe",         '1', c_ltgray,  0, tr_null,
   	mfb(transparent), &iexamine::none},
{"sewage pump",         '&', c_ltgray,  0, tr_null,
   	mfb(noitem), &iexamine::none},
{"centrifuge",          '{', c_magenta, 0, tr_null,
   	mfb(transparent), &iexamine::none},
{"column",              '1', c_ltgray,  0, tr_null,
	   mfb(flammable), &iexamine::none},
{"cloning vat",         '0', c_ltcyan,  0, tr_null,
    mfb(transparent)|mfb(bashable)|mfb(sealed)|mfb(place_item), &iexamine::none},
{"stairs down",         '>', c_yellow,  2, tr_null,
    mfb(transparent)|mfb(goes_down)|mfb(place_item), &iexamine::none},
{"stairs up",           '<', c_yellow,  2, tr_null,
    mfb(transparent)|mfb(goes_up)|mfb(place_item), &iexamine::none},
{"manhole",             '>', c_dkgray,  2, tr_null,
    mfb(transparent)|mfb(goes_down)|mfb(place_item), &iexamine::none},
{"ladder",              '<', c_dkgray,  2, tr_null,
    mfb(transparent)|mfb(goes_up)|mfb(place_item), &iexamine::none},
{"ladder",              '>', c_dkgray,  2, tr_null,
    mfb(transparent)|mfb(goes_down)|mfb(place_item), &iexamine::none},
{"downward slope",      '>', c_brown,   2, tr_null,
    mfb(transparent)|mfb(goes_down)|mfb(place_item), &iexamine::none},
{"upward slope",        '<', c_brown,   2, tr_null,
    mfb(transparent)|mfb(goes_up)|mfb(place_item), &iexamine::none},
{"rope leading up",     '<', c_white,   2, tr_null,
   	mfb(transparent)|mfb(goes_up), &iexamine::none},
{"manhole cover",       '0', c_dkgray,  2, tr_null,
	   mfb(transparent), &iexamine::none},
{"card reader",	        '6', c_pink,    0, tr_null,	// Science
	   mfb(noitem), &iexamine::cardreader},
{"card reader",	        '6', c_pink,    0, tr_null,	// Military
	   mfb(noitem), &iexamine::cardreader},
{"broken card reader",  '6',c_ltgray,   0, tr_null,
	   mfb(noitem), &iexamine::none},
{"slot machine",        '6', c_green,   0, tr_null,
	   mfb(bashable)|mfb(noitem)|mfb(indoors), &iexamine::slot_machine},
{"elevator controls",   '6', c_ltblue,  0, tr_null,
    mfb(noitem)|mfb(indoors), &iexamine::elevator},
{"powerless controls",  '6',c_ltgray,   0, tr_null,
    mfb(noitem)|mfb(indoors), &iexamine::none},
{"elevator",            '.', c_magenta, 2, tr_null,
    mfb(indoors)|mfb(transparent), &iexamine::none},
{"dark pedestal",       '&', c_dkgray,  0, tr_null,
	   mfb(transparent), &iexamine::pedestal_wyrm},
{"light pedestal",      '&', c_white,   0, tr_null,
	   mfb(transparent), &iexamine::pedestal_temple},
{"red stone",           '#', c_red,     0, tr_null,
	   0, &iexamine::none},
{"green stone",         '#', c_green,   0, tr_null,
	   0, &iexamine::none},
{"blue stone",          '#', c_blue,    0, tr_null,
	   0, &iexamine::none},
{"red floor",           '.', c_red,     2, tr_null,
	   mfb(transparent)|mfb(flat), &iexamine::none},
{"green floor",         '.', c_green,   2, tr_null,
	   mfb(transparent)|mfb(flat), &iexamine::none},
{"blue floor",          '.', c_blue,    2, tr_null,
	   mfb(transparent)|mfb(flat), &iexamine::none},
{"yellow switch",       '6', c_yellow,  0, tr_null,
	   mfb(transparent), &iexamine::fswitch},
{"cyan switch",         '6', c_cyan,    0, tr_null,
	   mfb(transparent), &iexamine::fswitch},
{"purple switch",       '6', c_magenta, 0, tr_null,
	   mfb(transparent), &iexamine::fswitch},
{"checkered switch",    '6', c_white,   0, tr_null,
	   mfb(transparent), &iexamine::fswitch},
{"tombstone",           'T', c_white,    2, tr_null,
    mfb(transparent)|mfb(thin_obstacle), &iexamine::none},
};

struct furn_t {
 std::string name;
 long sym;
 nc_color color;
 char movecost; // Penalty to terrain
 int move_str_req; //The amount of strength requried to move through this terrain easily.
 unsigned long flags;// : num_t_flags;
 void (iexamine::*examine)(game *, player *, map *m, int examx, int examy);
};

enum furn_id {
f_null,
f_hay,
f_bulletin,
f_indoor_plant,
f_bed, f_toilet, f_makeshift_bed,
f_sink, f_oven, f_woodstove, f_fireplace, f_bathtub,
f_chair, f_armchair, f_sofa, f_cupboard, f_trashcan, f_desk, f_exercise,
f_bench, f_table, f_pool_table,
f_counter,
f_fridge, f_glass_fridge, f_dresser, f_locker,
f_rack, f_bookcase,
f_washer, f_dryer,
f_dumpster,
f_crate_c, f_crate_o,
f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet, f_fema_groundsheet,
f_skin_wall, f_skin_door, f_skin_door_o,  f_skin_groundsheet,
f_mutpoppy,

num_furniture_types
};

const furn_t furnlist[num_furniture_types] = { // MUST match enum furn_id above!
{"nothing",             ' ', c_white,   0, -1,
    mfb(transparent), &iexamine::none},
{"hay",                 '#', i_brown,   3, 6,
    mfb(transparent)|mfb(container)|mfb(flammable2), &iexamine::none},
{"bulletin board",      '6', c_blue,    -10, -1,
    mfb(bashable)|mfb(flammable)|mfb(noitem)|
    mfb(deconstruct), &iexamine::bulletin_board},
{"indoor plant",        '^', c_green,   2, 5,
    mfb(container)|mfb(bashable)|mfb(place_item), &iexamine::none},
{"bed",                 '#', c_magenta, 3, -1,
    mfb(transparent)|mfb(container)|mfb(flammable2)|
    mfb(deconstruct)|mfb(place_item), &iexamine::none},
{"toilet",              '&', c_white,   2, 18,
    mfb(transparent)|mfb(bashable)|mfb(l_flammable), &iexamine::water_source},
{"makeshift bed",       '#', c_magenta, 3, 12,
    mfb(transparent)|mfb(bashable)|mfb(flammable2)|
    mfb(deconstruct), &iexamine::none},
{"sink",                '&', c_white,   2, 18,
    mfb(transparent)|mfb(bashable)|mfb(l_flammable)|
    mfb(container)|mfb(place_item), &iexamine::none},
{"oven",                '#', c_dkgray,  2, 10,
    mfb(transparent)|mfb(bashable)|mfb(l_flammable)|
    mfb(container)|mfb(place_item), &iexamine::none},
{"wood stove",          '#', i_red,     2, 10,
    mfb(transparent)|mfb(container)|mfb(fire_container)|
    mfb(suppress_smoke)|mfb(place_item), &iexamine::none},
{"fireplace",           '#', i_white,   2, -1,
    mfb(transparent)|mfb(container)|mfb(fire_container)|
    mfb(suppress_smoke)|mfb(place_item), &iexamine::none},
{"bathtub",             '~', c_white,   2, 10,
    mfb(transparent)|mfb(bashable)|mfb(l_flammable)|
    mfb(container)|mfb(place_item), &iexamine::none},
{"chair",               '#', c_brown,   1, 6,
    mfb(transparent)|mfb(flammable2)|mfb(deconstruct), &iexamine::none},
{"arm chair",           'H', c_green,   1, 7,
    mfb(transparent)|mfb(flammable2)|mfb(deconstruct), &iexamine::none},
{"sofa",                'H', i_red,     1, 10,
    mfb(transparent)|mfb(flammable2)|mfb(deconstruct), &iexamine::none},
{"cupboard",            '#', c_blue,    1, 8,
    mfb(transparent)|mfb(flammable2)|mfb(deconstruct)|
    mfb(container)|mfb(place_item), &iexamine::none},
{"trash can",           '&', c_ltcyan,  1, 5,
    mfb(transparent)|mfb(flammable2)|mfb(container)|
    mfb(place_item), &iexamine::none},
{"desk",                '#', c_ltred,   1, 8,
    mfb(transparent)|mfb(flammable2)|mfb(deconstruct)|
    mfb(container)|mfb(place_item), &iexamine::none},
{"exercise machine",    'T', c_dkgray,  1, 8,
    mfb(transparent)|mfb(deconstruct), &iexamine::none},
{"bench",               '#', c_brown,   1, 8,
    mfb(transparent)|mfb(flammable2)|mfb(deconstruct), &iexamine::none},
{"table",               '#', c_red,     2, 9,
    mfb(transparent)|mfb(flammable)|mfb(deconstruct), &iexamine::none},
{"pool table",          '#', c_green,   2, -1,
    mfb(transparent)|mfb(flammable)|mfb(deconstruct), &iexamine::none},
{"counter",	            '#', c_blue,    2, 10,
    mfb(transparent)|mfb(flammable)|mfb(deconstruct), &iexamine::none},
{"refrigerator",        '{', c_ltcyan,  -10, 10,
    mfb(container)|mfb(bashable)|mfb(deconstruct)|mfb(place_item), 
    &iexamine::none},
{"glass door fridge",   '{', c_ltcyan,  -10, 10,
    mfb(container)|mfb(bashable)|mfb(deconstruct)|mfb(place_item), 
    &iexamine::none},
{"dresser",             '{', c_brown,   -10, 8,
    mfb(transparent)|mfb(container)|mfb(flammable)|mfb(bashable)|
    mfb(deconstruct)|mfb(place_item), &iexamine::none},
{"locker",              '{', c_ltgray,  -10, 9,
    mfb(container)|mfb(bashable)|mfb(place_item)|
    mfb(deconstruct), &iexamine::none},
{"display rack",        '{', c_ltgray,  -10, 8,
    mfb(transparent)|mfb(l_flammable)|mfb(bashable)|mfb(deconstruct)|
    mfb(place_item)|mfb(deconstruct), &iexamine::none},
{"book case",           '{', c_brown,   -10, 9,
    mfb(flammable)|mfb(bashable)|mfb(deconstruct)|
    mfb(place_item), &iexamine::none},
{"washing machine",     '{', i_white,   -10, 16,
    mfb(container)|mfb(bashable)|mfb(place_item), &iexamine::none},
{"dryer",               '{', i_white,   -10, 16,
    mfb(container)|mfb(bashable)|mfb(place_item), &iexamine::none},
{"dumpster",	        '{', c_green,   -10, 16,
    mfb(container)|mfb(bashable)|mfb(place_item), &iexamine::none},
{"crate",               'X', i_brown,   -10, 14,
    mfb(transparent)|mfb(bashable)|mfb(container)|mfb(sealed)|
    mfb(flammable)|mfb(deconstruct)|mfb(place_item), &iexamine::none},
{"open crate",          'O', i_brown,   -10, 12,
    mfb(transparent)|mfb(bashable)|mfb(container)|
    mfb(flammable)|mfb(place_item), &iexamine::none},
{"canvas wall",         '#', c_blue,    -10, -1,
    mfb(l_flammable)|mfb(bashable)|mfb(noitem), &iexamine::none},
{"canvas flap",         '+', c_blue,    -10, -1,
    mfb(l_flammable)|mfb(bashable)|mfb(noitem), &iexamine::none},
{"open canvas flap",    '.', c_blue,    0, -1,
    mfb(transparent), &iexamine::none},
{"groundsheet",         ';', c_green,   0, -1,
    mfb(transparent)|mfb(indoors), &iexamine::tent},
{"groundsheet",         ';', c_green,   0, -1,
    mfb(transparent)|mfb(indoors), &iexamine::none},
{"animalskin wall",     '#', c_brown,   -10, -1,
    mfb(l_flammable)|mfb(bashable)|mfb(noitem), &iexamine::none},
{"animalskin flap",     '+', c_white,   -10, -1,
    mfb(l_flammable)|mfb(bashable)|mfb(noitem), &iexamine::none},
{"open animalskin flap", '.', c_white,  0, -1,
    mfb(transparent), &iexamine::none},
{"animalskin floor",    ';', c_brown,   0, -1,
    mfb(transparent)|mfb(indoors), &iexamine::shelter},
{"mutated poppy flower", 'f', c_red,    1, -1,
    mfb(transparent), &iexamine::flower_poppy}
};

/*
enum: map_extra
Map Extras are overmap specific flags that tell a submap "hey, put something extra here ontop of whats normally here".
*/
enum map_extra {
 mx_null = 0,
 mx_helicopter,
 mx_military,
 mx_science,
 mx_stash,
 mx_drugdeal,
 mx_supplydrop,
 mx_portal,
 mx_minefield,
 mx_wolfpack,
 mx_cougar,
 mx_puddle,
 mx_crater,
 mx_fumarole,
 mx_portal_in,
 mx_anomaly,
 num_map_extras
};

//Classic Extras is for when you have special zombies turned off.
const int classic_extras =  mfb(mx_helicopter) | mfb(mx_military) |
  mfb(mx_stash) | mfb(mx_drugdeal) | mfb(mx_supplydrop) | mfb(mx_minefield) |
  mfb(mx_wolfpack) | mfb(mx_cougar) | mfb(mx_puddle) | mfb(mx_crater);

// Chances are relative to eachother; e.g. a 200 chance is twice as likely
// as a 100 chance to appear.
const int map_extra_chance[num_map_extras + 1] = {
  0,	// Null - 0 chance
 40,	// Helicopter
 50,	// Military
120,	// Science
200,	// Stash
 20,	// Drug deal
 10, // Supply drop
  5,	// Portal
 70,	// Minefield
 30,	// Wolf pack
 40, // Cougar
250,	// Puddle
 10,	// Crater
  8,	// Fumarole
  7,	// One-way portal into this world
 10,	// Anomaly
  0	 // Just a cap value; leave this as the last one
};

struct map_extras {
 unsigned int chance;
 int chances[num_map_extras + 1];
 map_extras(unsigned int embellished, int helicopter = 0, int mili = 0,
            int sci = 0, int stash = 0, int drug = 0, int supply = 0,
            int portal = 0, int minefield = 0, int wolves = 0, int cougar = 0, int puddle = 0,
            int crater = 0, int lava = 0, int marloss = 0, int anomaly = 0)
            : chance(embellished)
 {
  chances[ 0] = 0;
  chances[ 1] = helicopter;
  chances[ 2] = mili;
  chances[ 3] = sci;
  chances[ 4] = stash;
  chances[ 5] = drug;
  chances[ 6] = supply;
  chances[ 7] = portal;
  chances[ 8] = minefield;
  chances[ 9] = wolves;
  chances[10] = cougar;
  chances[11] = puddle;
  chances[12] = crater;
  chances[13] = lava;
  chances[14] = marloss;
  chances[15] = anomaly;
  chances[16] = 0;
 }
};

struct spawn_point {
 int posx, posy;
 int count;
 mon_id type;
 int faction_id;
 int mission_id;
 bool friendly;
 std::string name;
 spawn_point(mon_id T = mon_null, int C = 0, int X = -1, int Y = -1,
             int FAC = -1, int MIS = -1, bool F = false,
             std::string N = "NONE") :
             posx (X), posy (Y), count (C), type (T), faction_id (FAC),
             mission_id (MIS), friendly (F), name (N) {}
};

struct submap {
 ter_id			ter[SEEX][SEEY]; // Terrain on each square
 std::vector<item>	itm[SEEX][SEEY]; // Items on each square
 furn_id        frn[SEEX][SEEY]; // Furniture on each square
 trap_id		trp[SEEX][SEEY]; // Trap on each square
 field			fld[SEEX][SEEY]; // Field on each square
 int			rad[SEEX][SEEY]; // Irradiation of each square
 graffiti graf[SEEX][SEEY]; // Graffiti on each square
 int active_item_count;
 int field_count;
 int turn_last_touched;
 std::vector<spawn_point> spawns;
 std::vector<vehicle*> vehicles;
 computer comp;
 basecamp camp;  // only allowing one basecamp per submap
};

std::ostream & operator<<(std::ostream &, const submap *);
std::ostream & operator<<(std::ostream &, const submap &);

#endif
