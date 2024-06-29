#pragma once
#ifndef CATA_SRC_MAPDATA_H
#define CATA_SRC_MAPDATA_H

#include <array>
#include <bitset>
#include <cstddef>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "clone_ptr.h"
#include "color.h"
#include "enum_bitset.h"
#include "game_constants.h"
#include "iexamine.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

struct ter_t;

using ter_str_id = string_id<ter_t>;

class JsonObject;
class Character;
struct iexamine_actor;
struct furn_t;
struct itype;
struct tripoint;

// size of connect groups bitset; increase if needed
const int NUM_TERCONN = 256;
connect_group get_connect_group( const std::string &name );

template <typename E> struct enum_traits;

struct map_common_bash_info { //TODO: Half of this shouldn't be common
        int str_min;            // min str(*) required to bash
        int str_max;            // max str required: bash succeeds if str >= random # between str_min & str_max
        int str_min_blocked;    // same as above; alternate values for has_adjacent_furniture(...) == true
        int str_max_blocked;
        int str_min_supported;  // Alternative values for floor supported by something from below
        int str_max_supported;
        int explosive;          // Explosion on destruction
        int sound_vol;          // sound volume of breaking terrain/furniture
        int sound_fail_vol;     // sound volume on fail
        int collapse_radius;    // Radius of the tent supported by this tile
        bool destroy_only;      // Only used for destroying, not normally bashable
        bool bash_below;        // This terrain is the roof of the tile below it, try to destroy that too
        item_group_id drop_group; // item group of items that are dropped when the object is bashed
        translation sound;      // sound made on success ('You hear a "smash!"')
        translation sound_fail; // sound  made on fail
        std::vector<furn_str_id> tent_centers;
        virtual void load( const JsonObject &jo, const bool was_loaded, const std::string &context );
        virtual void check( const std::string &id ) const;
    public:
        virtual ~map_common_bash_info() = default;
};
struct map_ter_bash_info : map_common_bash_info {
    ter_str_id ter_set;    // terrain to set
    ter_str_id ter_set_bashed_from_above; // terrain to set if bashed from above (defaults to ter_set)
    map_ter_bash_info() {};
    void load( const JsonObject &jo, const bool was_loaded, const std::string &context ) override;
    void check( const std::string &id ) const override;
};
struct map_furn_bash_info : map_common_bash_info {
    furn_str_id furn_set;   // furniture to set (only used by furniture, not terrain)
    map_furn_bash_info() {};
    void load( const JsonObject &jo, const bool was_loaded, const std::string &context ) override;
    void check( const std::string &id ) const override;
};
struct map_fd_bash_info : map_common_bash_info {
    int fd_bash_move_cost; // cost to bash a field
    translation field_bash_msg_success; // message upon successfully bashing a field
    map_fd_bash_info() {};
    void load( const JsonObject &jo, const bool was_loaded, const std::string &context ) override;
    void check( const std::string &id ) const override;
};
struct map_deconstruct_skill {
    skill_id id; // Id of skill to increase on successful deconstruction
    int min; // Minimum level to recieve xp
    int max; // Level cap after which no xp is recieved but practise still occurs delaying rust
    double multiplier; // Multiplier of the base xp given that's calced using the mean of the min and max
};
struct map_common_deconstruct_info {
        // This terrain provided a roof, we need to tear it down now
        bool deconstruct_above = false;
        // items you get when deconstructing.
        item_group_id drop_group;
        std::optional<map_deconstruct_skill> skill;
        virtual void load( const JsonObject &jo, const bool was_loaded, const std::string &context );
        virtual void check( const std::string &id ) const;
    public:
        virtual ~map_common_deconstruct_info() = default;
};
struct map_ter_deconstruct_info : map_common_deconstruct_info {
    ter_str_id ter_set = ter_str_id::NULL_ID();
    void load( const JsonObject &jo, const bool was_loaded, const std::string &context ) override;
    void check( const std::string &id ) const override;
    map_ter_deconstruct_info() {};
};
struct map_furn_deconstruct_info : map_common_deconstruct_info {
    furn_str_id furn_set = furn_str_id::NULL_ID();
    void load( const JsonObject &jo, const bool was_loaded, const std::string &context ) override;
    void check( const std::string &id ) const override;
    map_furn_deconstruct_info() {};
};
struct map_shoot_info {
    // Base chance to hit the object at all (defaults to 100%)
    int chance_to_hit = 0;
    // Minimum damage reduction to apply to shot when hit
    int reduce_dmg_min = 0;
    // Maximum damage reduction to apply to shot when hit
    int reduce_dmg_max = 0;
    // Minimum damage reduction to apply to laser shots when hit
    int reduce_dmg_min_laser = 0;
    // Maximum damage reduction to apply to laser shots when hit
    int reduce_dmg_max_laser = 0;
    // Damage required to have a chance to destroy
    int destroy_dmg_min = 0;
    // Damage required to guarantee destruction
    int destroy_dmg_max = 0;
    // Are lasers incapable of destroying the object (defaults to false)
    bool no_laser_destroy = false;
    bool load( const JsonObject &jsobj, std::string_view member, bool was_loaded );
};
struct furn_workbench_info {
    // Base multiplier applied for crafting here
    float multiplier;
    // Mass/volume allowed before a crafting speed penalty is applied
    units::mass allowed_mass;
    units::volume allowed_volume;
    furn_workbench_info();
    bool load( const JsonObject &jsobj, std::string_view member );
};
struct plant_data {
    // What the furniture turns into when it grows or you plant seeds in it
    furn_str_id transform;
    // What the 'base' furniture of the plant is, before you plant in it, and what it turns into when eaten
    furn_str_id base;
    // At what percent speed of a normal plant this plant furniture grows at
    float growth_multiplier;
    // What percent of the normal harvest this crop gives
    float harvest_multiplier;
    plant_data();
    bool load( const JsonObject &jsobj, std::string_view member );
};

/*
 * Note; All flags are defined as strings dynamically in data/json/terrain.json and furniture.json. The list above
 * represent the common builtins. The enum below is an alternative means of fast-access, for those flags that are checked
 * so much that strings produce a significant performance penalty. The following are equivalent:
 *  m->has_flag("FLAMMABLE");
 *  m->has_flag(ter_furn_flag::TFLAG_FLAMMABLE); // ~ 20 x faster than the above, ( 2.5 x faster if the above uses static const std::string str_flammable("FLAMMABLE");
 * To add a new ter_bitflag, add below and in mapdata.cpp
 * Order does not matter.
 * For descriptions see /docs/JSON_FLAGS.md
 */
enum class ter_furn_flag : int {
    TFLAG_TRANSPARENT,
    TFLAG_FLAMMABLE,
    TFLAG_REDUCE_SCENT,
    TFLAG_SWIMMABLE,
    TFLAG_SUPPORTS_ROOF,
    TFLAG_MINEABLE,
    TFLAG_NOITEM,
    TFLAG_NO_SIGHT,
    TFLAG_NO_SCENT,
    TFLAG_SEALED,
    TFLAG_ALLOW_FIELD_EFFECT,
    TFLAG_LIQUID,
    TFLAG_COLLAPSES,
    TFLAG_FLAMMABLE_ASH,
    TFLAG_DESTROY_ITEM,
    TFLAG_INDOORS,
    TFLAG_LIQUIDCONT,
    TFLAG_FIRE_CONTAINER,
    TFLAG_FLAMMABLE_HARD,
    TFLAG_SUPPRESS_SMOKE,
    TFLAG_SHARP,
    TFLAG_DIGGABLE,
    TFLAG_ROUGH,
    TFLAG_UNSTABLE,
    TFLAG_WALL,
    TFLAG_DEEP_WATER,
    TFLAG_SHALLOW_WATER,
    TFLAG_WATER_CUBE,
    TFLAG_CURRENT,
    TFLAG_HARVESTED,
    TFLAG_PERMEABLE,
    TFLAG_AUTO_WALL_SYMBOL,
    TFLAG_CONNECT_WITH_WALL,
    TFLAG_CLIMBABLE,
    TFLAG_GOES_DOWN,
    TFLAG_GOES_UP,
    TFLAG_NO_FLOOR,
    TFLAG_ALLOW_ON_OPEN_AIR,
    TFLAG_SEEN_FROM_ABOVE,
    TFLAG_RAMP_DOWN,
    TFLAG_RAMP_UP,
    TFLAG_RAMP,
    TFLAG_HIDE_PLACE,
    TFLAG_BLOCK_WIND,
    TFLAG_FLAT,
    TFLAG_RAIL,
    TFLAG_THIN_OBSTACLE,
    TFLAG_SMALL_PASSAGE,
    TFLAG_Z_TRANSPARENT,
    TFLAG_SUN_ROOF_ABOVE,
    TFLAG_FUNGUS,
    TFLAG_LOCKED,
    TFLAG_PICKABLE,
    TFLAG_WINDOW,
    TFLAG_DOOR,
    TFLAG_SHRUB,
    TFLAG_YOUNG,
    TFLAG_PLANT,
    TFLAG_FISHABLE,
    TFLAG_GRAZABLE,
    TFLAG_GRAZER_INEDIBLE,
    TFLAG_BROWSABLE,
    TFLAG_TREE,
    TFLAG_PLOWABLE,
    TFLAG_ORGANIC,
    TFLAG_CONSOLE,
    TFLAG_PLANTABLE,
    TFLAG_GROWTH_HARVEST,
    TFLAG_GROWTH_OVERGROWN,
    TFLAG_MOUNTABLE,
    TFLAG_RAMP_END,
    TFLAG_FLOWER,
    TFLAG_CAN_SIT,
    TFLAG_FLAT_SURF,
    TFLAG_BUTCHER_EQ,
    TFLAG_GROWTH_SEEDLING,
    TFLAG_GROWTH_MATURE,
    TFLAG_WORKOUT_ARMS,
    TFLAG_WORKOUT_LEGS,
    TFLAG_TRANSLOCATOR,
    TFLAG_AUTODOC,
    TFLAG_AUTODOC_COUCH,
    TFLAG_OPENCLOSE_INSIDE,
    TFLAG_SALT_WATER,
    TFLAG_PLACE_ITEM,
    TFLAG_BARRICADABLE_WINDOW_CURTAINS,
    TFLAG_CLIMB_SIMPLE,
    TFLAG_NANOFAB_TABLE,
    TFLAG_ROAD,
    TFLAG_TINY,
    TFLAG_SHORT,
    TFLAG_NOCOLLIDE,
    TFLAG_BARRICADABLE_DOOR,
    TFLAG_BARRICADABLE_DOOR_DAMAGED,
    TFLAG_BARRICADABLE_DOOR_REINFORCED,
    TFLAG_USABLE_FIRE,
    TFLAG_CONTAINER,
    TFLAG_NO_PICKUP_ON_EXAMINE,
    TFLAG_RUBBLE,
    TFLAG_DIGGABLE_CAN_DEEPEN,
    TFLAG_PIT_FILLABLE,
    TFLAG_DIFFICULT_Z,
    TFLAG_ALIGN_WORKBENCH,
    TFLAG_NO_SPOIL,
    TFLAG_EASY_DECONSTRUCT,
    TFLAG_LADDER,
    TFLAG_ALARMED,
    TFLAG_CHOCOLATE,
    TFLAG_SIGN,
    TFLAG_SIGN_ALWAYS,
    TFLAG_DONT_REMOVE_ROTTEN,
    TFLAG_BLOCKSDOOR,
    TFLAG_NO_SELF_CONNECT,
    TFLAG_BURROWABLE,
    TFLAG_MURKY,
    TFLAG_AMMOTYPE_RELOAD,
    TFLAG_TRANSPARENT_FLOOR,
    TFLAG_TOILET_WATER,
    TFLAG_ELEVATOR,
    TFLAG_ACTIVE_GENERATOR,
    TFLAG_SMALL_HIDE,
    TFLAG_NO_FLOOR_WATER,
    TFLAG_SINGLE_SUPPORT,
    TFLAG_CLIMB_ADJACENT,
    TFLAG_FLOATS_IN_AIR,
    TFLAG_HARVEST_REQ_CUT1,

    NUM_TFLAG_FLAGS
};

template<>
struct enum_traits<ter_furn_flag> {
    static constexpr ter_furn_flag last = ter_furn_flag::NUM_TFLAG_FLAGS;
};

struct connect_group {
    public:
        connect_group_id id;
        int index;

        static void load( const JsonObject &jo );
        static void reset();
};

struct activity_byproduct {
    itype_id item;
    int count      = 0;
    int random_min = 0;
    int random_max = 0;

    int roll() const;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

struct pry_data {
    bool prying_nails = false;

    int difficulty = 0;
    int prying_level = 0;

    bool noisy = false;
    bool alarm = false;
    bool breakable = false;

    translation failure;

    bool was_loaded = false;
    void load( const JsonObject &jo );
    void deserialize( const JsonObject &jo );
};

class activity_data_common
{
    public:
        activity_data_common() = default;

        bool valid() const {
            return valid_;
        }

        const time_duration &duration() const {
            return duration_;
        }

        const translation &message() const {
            return message_;
        }

        const translation &sound() const {
            return sound_;
        }

        const pry_data &prying_data() const {
            return prying_data_;
        }

        const std::vector<activity_byproduct> &byproducts() const {
            return byproducts_;
        }

        bool was_loaded = false;
        void load( const JsonObject &jo );

    protected:
        bool valid_ = false;
        time_duration duration_;
        translation message_;
        translation sound_;
        struct pry_data prying_data_;
        std::vector<activity_byproduct> byproducts_;
};

class activity_data_ter : public activity_data_common
{
    public:
        activity_data_ter() = default;

        const ter_str_id &result() const {
            return result_;
        }

        void load( const JsonObject &jo );

    private:
        ter_str_id result_;
};

class activity_data_furn : public activity_data_common
{
    public:
        activity_data_furn() = default;

        const furn_str_id &result() const {
            return result_;
        }

        void load( const JsonObject &jo );

    private:
        furn_str_id result_;
};

void init_mapdata();

struct map_data_common_t {
        std::set<emit_id> emissions;
        translation lockpick_message; // Lockpick action: message when successfully lockpicked
        cata::value_ptr<map_shoot_info> shoot;

    public:
        virtual ~map_data_common_t() = default;

    protected:
        friend furn_t null_furniture_t();
        friend ter_t null_terrain_t();
        // The (untranslated) plaintext name of the terrain type the user would see (i.e. dirt)
        translation name_;

        // Hardcoded examination function
        iexamine_functions examine_func; // What happens when the terrain/furniture is examined

        // Data-driven examine actor
        cata::clone_ptr<iexamine_actor> examine_actor;

    private:
        std::set<std::string> flags;    // string flags which possibly refer to what's documented above.
        enum_bitset<ter_furn_flag> bitflags; // bitfield of -certain- string flags which are heavily checked

    public:
        ter_str_id curtain_transform;

        bool has_curtains() const {
            return !( curtain_transform.is_empty() || curtain_transform.is_null() );
        }

        std::string name() const;

        /*
        * The symbol drawn on the screen for the terrain. Please note that
        * there are extensive rules as to which possible object/field/entity in
        * a single square gets drawn and that some symbols are "reserved" such
        * as * and % to do programmatic behavior.
        */
        std::array<int, NUM_SEASONS> symbol_;

        // TODO: Get rid of untyped overload.
        bool can_examine( const tripoint &examp ) const;
        bool can_examine( const tripoint_bub_ms &examp ) const;
        bool has_examine( iexamine_examine_function func ) const;
        bool has_examine( const std::string &action ) const;
        void set_examine( iexamine_functions func );
        // TODO: Get rid of untyped overload.
        void examine( Character &, const tripoint & ) const;
        void examine( Character &, const tripoint_bub_ms & ) const;

        int light_emitted = 0;
        // The amount of movement points required to pass this terrain by default.
        int movecost = 0;
        int heat_radiation = 0;
        // The coverage percentage of a furniture piece of terrain. <30 won't cover from sight.
        int coverage = 0;
        // Warmth provided by the terrain (for sleeping, etc.)
        units::temperature_delta floor_bedding_warmth = 0_C_delta;
        int comfort = 0;
        // Maximal volume of items that can be stored in/on this furniture
        units::volume max_volume = DEFAULT_TILE_VOLUME;

        translation description;

        // The color the sym will draw in on the GUI.
        std::array<nc_color, NUM_SEASONS> color_;
        void load_symbol( const JsonObject &jo, const std::string &context );

        std::string looks_like;

        /**
         * When will this terrain/furniture get harvested and what will drop?
         * Note: This excludes items that take extra tools to harvest.
         */
        std::array<harvest_id, NUM_SEASONS> harvest_by_season = {{
                harvest_id::NULL_ID(), harvest_id::NULL_ID(), harvest_id::NULL_ID(), harvest_id::NULL_ID()
            }
        };

        bool transparent = false;

        const std::set<std::string> &get_flags() const {
            return flags;
        }

        bool has_flag( const std::string &flag ) const {
            return flags.count( flag ) > 0;
        }

        bool has_flag( const ter_furn_flag flag ) const {
            return bitflags[flag];
        }

        void set_flag( const std::string &flag );

        void set_flag( ter_furn_flag flag );

        void unset_flag( const std::string &flag );

        void unset_flags();

        // Terrain groups of this type, for others to connect or rotate to; not symmetric, passive part
        std::bitset<NUM_TERCONN> connect_groups;
        // Terrain groups to connect to; not symmetric, target of active part
        std::bitset<NUM_TERCONN> connect_to_groups;
        // Terrain groups rotate towards; not symmetric, target of active part
        std::bitset<NUM_TERCONN> rotate_to_groups;

        // Set to be member of a connection target group
        void set_connect_groups( const std::vector<std::string> &connect_groups_vec );
        // Set target connection group
        void set_connects_to( const std::vector<std::string> &connect_groups_vec );
        // Set target group to rotate towards
        void set_rotates_to( const std::vector<std::string> &connect_groups_vec );

        // Set groups helper function
        void set_groups( std::bitset<NUM_TERCONN> &bits,
                         const std::vector<std::string> &connect_groups_vec );

        bool in_connect_groups( const std::bitset<NUM_TERCONN> &test_connect_group ) const {
            return ( connect_groups & test_connect_group ).any();
        }

        int symbol() const;
        nc_color color() const;

        const harvest_id &get_harvest() const;
        /**
         * Returns a set of names of the items that would be dropped.
         * Used for NPC whitelist checking.
         */
        const std::set<std::string> &get_harvest_names() const;

        std::string extended_description() const;

        bool was_loaded = false;

        bool is_flammable() const {
            return has_flag( ter_furn_flag::TFLAG_FLAMMABLE ) ||
                   has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH ) ||
                   has_flag( ter_furn_flag::TFLAG_FLAMMABLE_HARD );
        }

        virtual void load( const JsonObject &jo, const std::string & );
        virtual void check() const {};
};

/*
* Struct ter_t:
* Short for terrain type. This struct defines all of the metadata for a given terrain id (an enum below).
*/
struct ter_t : map_data_common_t {

    std::vector<std::pair<ter_str_id, mod_id>> src;

    ter_str_id id;    // The terrain's ID. Must be set, must be unique.
    ter_str_id open;  // Open action: transform into terrain with matching id
    ter_str_id close; // Close action: transform into terrain with matching id

    map_ter_bash_info bash;
    std::optional<map_ter_deconstruct_info> deconstruct;

    ter_str_id lockpick_result; // Lockpick action: transform when successfully lockpicked

    cata::value_ptr<activity_data_ter> boltcut; // Bolt cutting action data
    cata::value_ptr<activity_data_ter> hacksaw; // Hacksaw action data
    cata::value_ptr<activity_data_ter> oxytorch; // Oxytorch action data
    cata::value_ptr<activity_data_ter> prying;  // Prying action data

    std::string trap_id_str;     // String storing the id string of the trap.
    ter_str_id transforms_into; // Transform into what terrain?
    ter_str_id roof;            // What will be the floor above this terrain

    trap_id trap; // The id of the trap located at this terrain. Limit one trap per tile currently.

    std::set<itype_id> allowed_template_id;

    ter_t();

    static size_t count();

    bool is_null() const;

    void load( const JsonObject &jo, const std::string &src ) override;
    void check() const override;
};

void set_ter_ids();
void set_furn_ids();
void reset_furn_ter();

/*
 * The terrain list contains the master list of  information and metadata for a given type of terrain.
 */

struct furn_t : map_data_common_t {

    std::vector<std::pair<furn_str_id, mod_id>> src;

    furn_str_id id;
    furn_str_id open;  // Open action: transform into furniture with matching id
    furn_str_id close; // Close action: transform into furniture with matching id
    furn_str_id lockpick_result; // Lockpick action: transform when successfully lockpicked
    map_furn_bash_info bash;
    std::optional<map_furn_deconstruct_info> deconstruct;
    itype_id crafting_pseudo_item;
    units::volume keg_capacity = 0_ml;

    units::temperature_delta bonus_fire_warmth_feet = 0.6_C_delta;
    itype_id deployed_item; // item id string used to create furniture

    int move_str_req = 0; //The amount of strength required to move through this furniture easily.

    cata::value_ptr<activity_data_furn> boltcut; // Bolt cutting action data
    cata::value_ptr<activity_data_furn> hacksaw; // Hacksaw action data
    cata::value_ptr<activity_data_furn> oxytorch; // Oxytorch action data
    cata::value_ptr<activity_data_furn> prying;  // Prying action data

    cata::value_ptr<furn_workbench_info> workbench;

    cata::value_ptr<plant_data> plant;

    cata::value_ptr<float> surgery_skill_multiplier;

    // May return NULL
    const itype *crafting_pseudo_item_type() const;
    // May return an empty container if no valid ammotype
    std::vector<const itype *> crafting_ammo_item_types() const;

    furn_t();

    static size_t count();

    bool is_movable() const;

    void load( const JsonObject &jo, const std::string &src ) override;
    void check() const override;
};

void load_furniture( const JsonObject &jo, const std::string &src );
void load_terrain( const JsonObject &jo, const std::string &src );

void verify_furniture();
void verify_terrain();

class ter_furn_migrations
{
    public:
        /** Handler for loading "ter_furn_migration" type of json object */
        static void load( const JsonObject &jo );

        /** Clears migration list */
        static void reset();

        /** Checks migrations */
        static void check();
};

class field_type_migrations
{
    public:
        /** Handler for loading "field_type_migration" type of json object */
        static void load( const JsonObject &jo );

        /** Clears migration list */
        static void reset();

        /** Checks migrations */
        static void check();
};

/*
runtime index: ter_id
ter_id refers to a position in the terlist[] where the ter_t struct is stored. These global
ints are a drop-in replacement to the old enum, however they are -not- required (save for areas in
the code that can use the performance boost and refer to core terrain types), and they are -not-
provided for terrains added by mods. A string equivalent is always present, i.e.;
t_basalt
"t_basalt"
*/
// NOLINTNEXTLINE(cata-static-int_id-constants)
extern ter_id t_null;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// These are on their way OUT and only used in certain switch statements until they are rewritten.

// consistency checking of terlist & furnlist.
void check_furniture_and_terrain();

void finalize_furniture_and_terrain();

#endif // CATA_SRC_MAPDATA_H
