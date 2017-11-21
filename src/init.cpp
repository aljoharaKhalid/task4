#include "init.h"

#include "json.h"
#include "filesystem.h"

// can load from json
#include "activity_type.h"
#include "flag.h"
#include "effect.h"
#include "emit.h"
#include "vitamin.h"
#include "fault.h"
#include "material.h"
#include "bionics.h"
#include "profession.h"
#include "skill.h"
#include "mutation.h"
#include "text_snippets.h"
#include "item_factory.h"
#include "vehicle_group.h"
#include "string_formatter.h"
#include "crafting.h"
#include "crafting_gui.h"
#include "mapdata.h"
#include "color.h"
#include "trap.h"
#include "mission.h"
#include "monstergenerator.h"
#include "inventory.h"
#include "tutorial.h"
#include "overmap.h"
#include "overmap_connection.h"
#include "artifact.h"
#include "overmap_location.h"
#include "mapgen.h"
#include "speech.h"
#include "construction.h"
#include "ammo.h"
#include "debug.h"
#include "path_info.h"
#include "requirements.h"
#include "start_location.h"
#include "scenario.h"
#include "omdata.h"
#include "options.h"
#include "game.h"
#include "faction.h"
#include "npc.h"
#include "item_action.h"
#include "dialogue.h"
#include "mongroup.h"
#include "monfaction.h"
#include "martialarts.h"
#include "veh_type.h"
#include "clzones.h"
#include "sounds.h"
#include "gates.h"
#include "overlay_ordering.h"
#include "worldfactory.h"
#include "weather_gen.h"
#include "npc_class.h"
#include "recipe_dictionary.h"
#include "rotatable_symbols.h"
#include "harvest.h"
#include "morale_types.h"
#include "anatomy.h"
#include "loading_ui.h"

#include <assert.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors
#include <locale> // for loading names

DynamicDataLoader::DynamicDataLoader()
{
    initialize();
}

DynamicDataLoader::~DynamicDataLoader() = default;

DynamicDataLoader &DynamicDataLoader::get_instance()
{
    static DynamicDataLoader theDynamicDataLoader;
    return theDynamicDataLoader;
}

void DynamicDataLoader::load_object( JsonObject &jo, const std::string &src )
{
    std::string type = jo.get_string("type");
    t_type_function_map::iterator it = type_function_map.find(type);
    if (it == type_function_map.end()) {
        jo.throw_error( "unrecognized JSON object", "type" );
    }
    it->second( jo, src );
}

void DynamicDataLoader::load_deferred( deferred_json& data )
{
    while( !data.empty() ) {
        const size_t n = data.size();
        auto it = data.begin();
        for( size_t idx = 0; idx != n; ++idx ) {
            try {
                std::istringstream str( it->first );
                JsonIn jsin( str );
                JsonObject jo = jsin.get_object();
                load_object( jo, it->second );
            } catch( const std::exception &err ) {
                debugmsg( "Error loading data from json: %s", err.what() );
            }
            ++it;
        }
        data.erase( data.begin(), it );
        if( data.size() == n ) {
            std::ostringstream discarded;
            for( const auto &elem : data ) {
                discarded << elem.first;
            }
            debugmsg( "JSON contains circular dependency. Discarded %i objects:\n%s",
                      data.size(), discarded.str().c_str() );
            data.clear();
            return; // made no progress on this cycle so abort
        }
    }
}

void load_ignored_type(JsonObject &jo)
{
    // This does nothing!
    // This function is used for types that are to be ignored
    // (for example for testing or for unimplemented types)
    (void) jo;
}

void DynamicDataLoader::add( const std::string &type, std::function<void(JsonObject &, const std::string &)> f )
{
    const auto pair = type_function_map.emplace( type, f );
    if( !pair.second ) {
        debugmsg( "tried to insert a second handler for type %s into the DynamicDataLoader", type.c_str() );
    }
}

void DynamicDataLoader::add( const std::string &type, std::function<void(JsonObject&)> f )
{
    const auto pair = type_function_map.emplace( type, [f]( JsonObject &obj, const std::string & ) { f( obj ); } );
    if( !pair.second ) {
        debugmsg( "tried to insert a second handler for type %s into the DynamicDataLoader", type.c_str() );
    }
}

void DynamicDataLoader::initialize()
{
    // all of the applicable types that can be loaded, along with their loading functions
    // Add to this as needed with new StaticFunctionAccessors or new ClassFunctionAccessors for new applicable types
    // Static Function Access
    add( "json_flag", &json_flag::load );
    add( "fault", &fault::load_fault );
    add( "emit", &emit::load_emit );
    add( "activity_type", &activity_type::load );
    add( "vitamin", &vitamin::load_vitamin );
    add( "material", &materials::load );
    add( "bionic", &load_bionic );
    add( "profession", &profession::load_profession );
    add( "profession_item_substitutions", &profession::load_item_substitutions );
    add( "skill", &Skill::load_skill );
    add( "dream", &load_dream );
    add( "mutation_category", &load_mutation_category );
    add( "mutation", &mutation_branch::load );
    add( "furniture", &load_furniture );
    add( "terrain", &load_terrain );
    add( "monstergroup", &MonsterGroupManager::LoadMonsterGroup );
    add( "MONSTER_BLACKLIST", &MonsterGroupManager::LoadMonsterBlacklist );
    add( "MONSTER_WHITELIST", &MonsterGroupManager::LoadMonsterWhitelist );
    add( "speech", &load_speech );
    add( "ammunition_type", &ammunition_type::load_ammunition_type );
    add( "scenario", &scenario::load_scenario );
    add( "start_location", &start_location::load_location );

    // json/colors.json would be listed here, but it's loaded before the others (see start_color())
    // Non Static Function Access
    add( "snippet", []( JsonObject &jo ) { SNIPPET.load_snippet( jo ); } );
    add( "item_group", []( JsonObject &jo ) { item_controller->load_item_group( jo ); } );
    add( "item_action", []( JsonObject &jo ) { item_action_generator::generator().load_item_action( jo ); } );

    add( "vehicle_part",  &vpart_info::load );
    add( "vehicle",  &vehicle_prototype::load );
    add( "vehicle_group",  &VehicleGroup::load );
    add( "vehicle_placement",  &VehiclePlacement::load );
    add( "vehicle_spawn",  &VehicleSpawn::load );

    add( "requirement", []( JsonObject &jo ) { requirement_data::load_requirement( jo ); } );
    add( "trap", &trap::load_trap );

    add( "AMMO", []( JsonObject &jo, const std::string &src ) { item_controller->load_ammo( jo, src ); } );
    add( "GUN", []( JsonObject &jo, const std::string &src ) { item_controller->load_gun( jo, src ); } );
    add( "ARMOR", []( JsonObject &jo, const std::string &src ) { item_controller->load_armor( jo, src ); } );
    add( "TOOL", []( JsonObject &jo, const std::string &src ) { item_controller->load_tool( jo, src ); } );
    add( "TOOLMOD", []( JsonObject &jo, const std::string &src ) { item_controller->load_toolmod( jo, src ); } );
    add( "TOOL_ARMOR", []( JsonObject &jo, const std::string &src ) { item_controller->load_tool_armor( jo, src ); } );
    add( "BOOK", []( JsonObject &jo, const std::string &src ) { item_controller->load_book( jo, src ); } );
    add( "COMESTIBLE", []( JsonObject &jo, const std::string &src ) { item_controller->load_comestible( jo, src ); } );
    add( "CONTAINER", []( JsonObject &jo, const std::string &src ) { item_controller->load_container( jo, src ); } );
    add( "ENGINE", []( JsonObject &jo, const std::string &src ) { item_controller->load_engine( jo, src ); } );
    add( "WHEEL", []( JsonObject &jo, const std::string &src ) { item_controller->load_wheel( jo, src ); } );
    add( "FUEL", []( JsonObject &jo, const std::string &src ) { item_controller->load_fuel( jo, src ); } );
    add( "GUNMOD", []( JsonObject &jo, const std::string &src ) { item_controller->load_gunmod( jo, src ); } );
    add( "MAGAZINE", []( JsonObject &jo, const std::string &src ) { item_controller->load_magazine( jo, src ); } );
    add( "GENERIC", []( JsonObject &jo, const std::string &src ) { item_controller->load_generic( jo, src ); } );
    add( "BIONIC_ITEM", []( JsonObject &jo, const std::string &src ) { item_controller->load_bionic( jo, src ); } );

    add( "ITEM_CATEGORY", []( JsonObject &jo ) { item_controller->load_item_category( jo ); } );
    add( "MIGRATION", []( JsonObject &jo ) { item_controller->load_migration( jo ); } );

    add( "MONSTER", []( JsonObject &jo, const std::string &src ) { MonsterGenerator::generator().load_monster( jo, src ); } );
    add( "SPECIES", []( JsonObject &jo, const std::string &src ) { MonsterGenerator::generator().load_species( jo, src ); } );

    add( "recipe_category", &load_recipe_category );
    add( "recipe",  &recipe_dictionary::load_recipe );
    add( "uncraft", &recipe_dictionary::load_uncraft );

    add( "tool_quality", &quality::load_static );
    add( "technique", &load_technique );
    add( "martial_art", &load_martial_art );
    add( "effect_type", &load_effect_type );
    add( "tutorial_messages", &load_tutorial_messages );
    add( "overmap_terrain", &overmap_terrains::load );
    add( "construction", &load_construction );
    add( "mapgen", &load_mapgen );
    add( "overmap_connection", &overmap_connections::load );
    add( "overmap_location", &overmap_locations::load );
    add( "overmap_special", &overmap_specials::load );
    add( "city_building", &city_buildings::load );

    add( "region_settings", &load_region_settings );
    add( "region_overlay", &load_region_overlay );
    add( "ITEM_BLACKLIST", []( JsonObject &jo ) { item_controller->load_item_blacklist( jo ); } );
    add( "WORLD_OPTION", &load_world_option );

    // loaded earlier.
    add( "colordef", &load_ignored_type );
    // mod information, ignored, handled by the mod manager
    add( "MOD_INFO", &load_ignored_type );

    add( "faction", &faction::load_faction );
    add( "npc", &npc_template::load );
    add( "npc_class", &npc_class::load_npc_class );
    add( "talk_topic", &load_talk_topic );
    add( "epilogue", &epilogue::load_epilogue );

    add( "MONSTER_FACTION", &monfactions::load_monster_faction );

    add( "sound_effect", &sfx::load_sound_effects );
    add( "playlist", &sfx::load_playlist );

    add( "gate", &gates::load );
    add( "overlay_order", &load_overlay_ordering );
    add( "mission_definition", []( JsonObject &jo, const std::string &src ) { mission_type::load_mission_type( jo, src ); } );
    add( "harvest", []( JsonObject &jo, const std::string &src ) { harvest_list::load( jo, src ); } );

    add( "monster_attack", []( JsonObject &jo, const std::string &src ) { MonsterGenerator::generator().load_monster_attack( jo, src ); } );
    add( "palette", mapgen_palette::load );
    add( "rotatable_symbol", &rotatable_symbols::load );
    add( "body_part", &body_part_struct::load_bp );
    add( "anatomy", &anatomy::load_anatomy );
    add( "morale_type", &morale_type_data::load_type );
}

void DynamicDataLoader::load_data_from_path( const std::string &path, const std::string &src, loading_ui &ui )
{
    assert( !finalized && "Can't load additional data after finalization. Must be unloaded first." );
    // We assume that each folder is consistent in itself,
    // and all the previously loaded folders.
    // E.g. the core might provide a vpart "frame-x"
    // the first loaded mode might provide a vehicle that uses that frame
    // But not the other way round.

    // get a list of all files in the directory
    str_vec files = get_files_from_path(".json", path, true, true);
    if (files.empty()) {
        std::ifstream tmp(path.c_str(), std::ios::in);
        if (tmp) {
            // path is actually a file, don't checking the extension,
            // assume we want to load this file anyway
            files.push_back(path);
        }
    }
    // iterate over each file
    for( auto &files_i : files ) {
        const std::string &file = files_i;
        // open the file as a stream
        std::ifstream infile(file.c_str(), std::ifstream::in | std::ifstream::binary);
        // and stuff it into ram
        std::istringstream iss(
            std::string(
                (std::istreambuf_iterator<char>(infile)),
                std::istreambuf_iterator<char>()
            )
        );
        try {
            // parse it
            JsonIn jsin(iss);
            load_all_from_json( jsin, src, ui );
        } catch( const JsonError &err ) {
            throw std::runtime_error( file + ": " + err.what() );
        }
    }
}

void DynamicDataLoader::load_all_from_json( JsonIn &jsin, const std::string &src, loading_ui & )
{
    if( jsin.test_object() ) {
        // find type and dispatch single object
        JsonObject jo = jsin.get_object();
        load_object( jo, src );
        jo.finish();
        // if there's anything else in the file, it's an error.
        jsin.eat_whitespace();
        if (jsin.good()) {
            jsin.error( string_format( "expected single-object file but found '%c'", jsin.peek() ) );
        }
    } else if( jsin.test_array() ) {
        jsin.start_array();
        // find type and dispatch each object until array close
        while (!jsin.end_array()) {
            JsonObject jo = jsin.get_object();
            load_object( jo, src );
            jo.finish();
        }
    } else {
        // not an object or an array?
        jsin.error( "expected object or array" );
    }
}

void DynamicDataLoader::unload_data()
{
    finalized = false;

    json_flag::reset();
    requirement_data::reset();
    vitamin::reset();
    emit::reset();
    activity_type::reset();
    fault::reset();
    materials::reset();
    profession::reset();
    Skill::reset();
    dreams.clear();
    // clear techniques, martial arts, and ma buffs
    clear_techniques_and_martial_arts();
    // Mission types are not loaded from json, but they depend on
    // the overmap terrain + items and that gets loaded from json.
    mission_type::reset();
    item_controller->reset();
    mutations_category.clear();
    mutation_category_trait::reset();
    mutation_branch::reset_all();
    reset_bionics();
    clear_tutorial_messages();
    reset_furn_ter();
    MonsterGroupManager::ClearMonsterGroups();
    SNIPPET.clear_snippets();
    vehicle_prototype::reset();
    vpart_info::reset();
    MonsterGenerator::generator().reset();
    reset_recipe_categories();
    recipe_dictionary::reset();
    quality::reset();
    trap::reset();
    reset_constructions();
    overmap_terrains::reset();
    reset_region_settings();
    reset_mapgens();
    reset_effect_types();
    reset_speech();
    overmap_connections::reset();
    overmap_locations::reset();
    overmap_specials::reset();
    ammunition_type::reset();
    unload_talk_topics();
    start_location::reset();
    scenario::reset();
    gates::reset();
    reset_overlay_ordering();
    npc_class::reset_npc_classes();
    rotatable_symbols::reset();
    body_part_struct::reset();
    npc_template::reset();
    anatomy::reset();

    // TODO:
    //    Name::clear();
}

extern void calculate_mapgen_weights();

void DynamicDataLoader::finalize_loaded_data()
{
    // Create a dummy that will not display anything
    // @todo Make it print to stdout?
    loading_ui ui( false );
    finalize_loaded_data( ui );
}

void DynamicDataLoader::finalize_loaded_data( loading_ui &ui )
{
    assert( !finalized && "Can't finalize the data twice." );
    ui.new_context( _( "Finalizing" ) );

    using named_entry = std::pair<std::string, std::function<void()>>;
    const std::vector<named_entry> entries = {{
        { _( "Body parts" ), &body_part_struct::finalize_all },
        { _( "Items" ), []() { item_controller->finalize(); } },
        { _( "Crafting requirements" ), []() { requirement_data::finalize(); } },
        { _( "Vehicle parts" ), &vpart_info::finalize },
        { _( "Traps" ), &trap::finalize },
        { _( "Terrain" ), &set_ter_ids },
        { _( "Furniture" ), &set_furn_ids },
        { _( "Overmap terrain" ), &overmap_terrains::finalize },
        { _( "Overmap connections" ), &overmap_connections::finalize },
        { _( "Overmap specials" ), &overmap_specials::finalize },
        { _( "Vehicle prototypes" ), &vehicle_prototype::finalize },
        { _( "Mapgen weights" ), &calculate_mapgen_weights },
        { _( "Monster types" ), []() { MonsterGenerator::generator().finalize_mtypes(); } },
        { _( "Monster groups" ), &MonsterGroupManager::FinalizeMonsterGroups },
        { _( "Monster factions" ), &monfactions::finalize },
        { _( "Crafting recipes" ), &recipe_dictionary::finalize },
        { _( "Martial arts" ), &finialize_martial_arts },
        { _( "Constructions" ), &finalize_constructions },
        { _( "NPC classes" ), &npc_class::finalize_all },
        { _( "Harvest lists" ), &harvest_list::finalize_all },
        { _( "Anatomies" ), &anatomy::finalize_all }
    }};

    for( const named_entry &e : entries ) {
        ui.add_entry( e.first );
    }

    ui.show();
    for( const named_entry &e : entries ) {
        e.second();
        ui.proceed();
    }

    check_consistency( ui );
    finalized = true;
}

void DynamicDataLoader::check_consistency( loading_ui &ui )
{
    ui.new_context( _( "Verifying" ) );

    using named_entry = std::pair<std::string, std::function<void()>>;
    const std::vector<named_entry> entries = {{
        { _( "Flags" ), &json_flag::check_consistency },
        { _( "Crafting requirements" ), []() { requirement_data::check_consistency(); } },
        { _( "Vitamins" ), &vitamin::check_consistency },
        { _( "Emissions" ), &emit::check_consistency },
        { _( "Activities" ), &activity_type::check_consistency },
        { _( "Items" ), []() { item_controller->check_definitions(); } },
        { _( "Materials" ), &materials::check },
        { _( "Engine faults" ), &fault::check_consistency },
        { _( "Vehicle parts" ), &vpart_info::check },
        { _( "Monster types" ), []() { MonsterGenerator::generator().check_monster_definitions(); } },
        { _( "Monster groups" ), &MonsterGroupManager::check_group_definitions },
        { _( "Furniture and terrain" ), &check_furniture_and_terrain },
        { _( "Constructions" ), &check_constructions },
        { _( "Professions" ), &profession::check_definitions },
        { _( "Scenarios" ), &scenario::check_definitions },
        { _( "Martial arts" ), &check_martialarts },
        { _( "Mutations" ), &mutation_branch::check_consistency },
        { _( "Mutation Categories" ), &mutation_category_trait::check_consistency },
        { _( "Overmap connections" ), &overmap_connections::check_consistency },
        { _( "Overmap terrain" ), &overmap_terrains::check_consistency },
        { _( "Overmap locations" ), &overmap_locations::check_consistency },
        { _( "Overmap specials" ), &overmap_specials::check_consistency },
        { _( "Ammunition types" ), &ammunition_type::check_consistency },
        { _( "Traps" ), &trap::check_consistency },
        { _( "Bionics" ), &check_bionics },
        { _( "Gates" ), &gates::check },
        { _( "NPC classes" ), &npc_class::check_consistency },
        { _( "Mission types" ), &mission_type::check_consistency },
        { _( "Item actions" ), []() { item_action_generator::generator().check_consistency(); } },
        { _( "Harvest lists" ), &harvest_list::check_consistency },
        { _( "NPC templates" ), &npc_template::check_consistency },
        { _( "Body parts" ), &body_part_struct::check_consistency },
        { _( "Anatomies" ), &anatomy::check_consistency }
    }};

    for( const named_entry &e : entries ) {
        ui.add_entry( e.first );
    }

    ui.show();
    for( const named_entry &e : entries ) {
        e.second();
        ui.proceed();
    }
}
