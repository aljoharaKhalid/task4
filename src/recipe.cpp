#include "recipe.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "calendar.h"
#include "game_constants.h"
#include "item.h"
#include "itype.h"
#include "output.h"
#include "skill.h"
#include "uistate.h"
#include "string_formatter.h"
#include "assign.h"
#include "cata_utility.h"
#include "character.h"
#include "construction.h"
#include "json.h"
#include "mapgen_functions.h"
#include "optional.h"
#include "player.h"
#include "translations.h"
#include "type_id.h"
#include "string_id.h"
#include "flat_set.h"
#include "units.h"

recipe::recipe() : skill_used( skill_id::NULL_ID() ) {}

time_duration recipe::batch_duration( int batch, float multiplier, size_t assistants ) const
{
    return time_duration::from_turns( batch_time( batch, multiplier, assistants ) / 100 );
}

int recipe::batch_time( int batch, float multiplier, size_t assistants ) const
{
    // 1.0f is full speed
    // 0.33f is 1/3 speed
    if( multiplier == 0.0f ) {
        // If an item isn't craftable in the dark, show the time to complete as if you could craft it
        multiplier = 1.0f;
    }

    const float local_time = static_cast<float>( time ) / multiplier;

    // if recipe does not benefit from batching and we have no assistants, don't do unnecessary additional calculations
    if( batch_rscale == 0.0 && assistants == 0 ) {
        return static_cast<int>( local_time ) * batch;
    }

    float total_time = 0.0;
    // if recipe does not benefit from batching but we do have assistants, skip calculating the batching scale factor
    if( batch_rscale == 0.0 ) {
        total_time = local_time * batch;
    } else {
        // recipe benefits from batching, so batching scale factor needs to be calculated
        // At batch_rsize, incremental time increase is 99.5% of batch_rscale
        const double scale = batch_rsize / 6.0;
        for( int x = 0; x < batch; x++ ) {
            // scaled logistic function output
            const double logf = ( 2.0 / ( 1.0 + exp( -( x / scale ) ) ) ) - 1.0;
            total_time += local_time * ( 1.0 - ( batch_rscale * logf ) );
        }
    }

    //Assistants can decrease the time for production but never less than that of one unit
    if( assistants == 1 ) {
        total_time = total_time * .75;
    } else if( assistants >= 2 ) {
        total_time = total_time * .60;
    }
    if( total_time < local_time ) {
        total_time = local_time;
    }

    return static_cast<int>( total_time );
}

bool recipe::has_flag( const std::string &flag_name ) const
{
    return flags.count( flag_name );
}

void recipe::load( const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    abstract = jo.has_string( "abstract" );

    if( abstract ) {
        ident_ = recipe_id( jo.get_string( "abstract" ) );
    } else {
        result_ = jo.get_string( "result" );
        ident_ = recipe_id( result_ );
    }

    if( jo.has_bool( "obsolete" ) ) {
        assign( jo, "obsolete", obsolete );
    }

    if( jo.has_int( "time" ) ) {
        time = jo.get_int( "time" );
    } else if( jo.has_string( "time" ) ) {
        time = to_moves<int>( read_from_json_string<time_duration>( *jo.get_raw( "time" ),
                              time_duration::units ) );
    }
    assign( jo, "difficulty", difficulty, strict, 0, MAX_SKILL );
    assign( jo, "flags", flags );

    // automatically set contained if we specify as container
    assign( jo, "contained", contained, strict );
    contained |= assign( jo, "container", container, strict );

    if( jo.has_array( "batch_time_factors" ) ) {
        auto batch = jo.get_array( "batch_time_factors" );
        batch_rscale = batch.get_int( 0 ) / 100.0;
        batch_rsize  = batch.get_int( 1 );
    }

    assign( jo, "charges", charges );
    assign( jo, "result_mult", result_mult );

    assign( jo, "skill_used", skill_used, strict );

    if( jo.has_member( "skills_required" ) ) {
        auto sk = jo.get_array( "skills_required" );
        required_skills.clear();

        if( sk.empty() ) {
            // clear all requirements

        } else if( sk.has_array( 0 ) ) {
            // multiple requirements
            for( JsonArray arr : sk ) {
                required_skills[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }

        } else {
            // single requirement
            required_skills[skill_id( sk.get_string( 0 ) )] = sk.get_int( 1 );
        }
    }

    // simplified autolearn sets requirements equal to required skills at finalization
    if( jo.has_bool( "autolearn" ) ) {
        assign( jo, "autolearn", autolearn );

    } else if( jo.has_array( "autolearn" ) ) {
        autolearn = true;
        for( JsonArray arr : jo.get_array( "autolearn" ) ) {
            autolearn_requirements[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
        }
    }

    // Never let the player have a debug or NPC recipe
    if( jo.has_bool( "never_learn" ) ) {
        assign( jo, "never_learn", never_learn );
    }

    if( jo.has_member( "decomp_learn" ) ) {
        learn_by_disassembly.clear();

        if( jo.has_int( "decomp_learn" ) ) {
            if( !skill_used ) {
                jo.throw_error( "decomp_learn specified with no skill_used" );
            }
            assign( jo, "decomp_learn", learn_by_disassembly[skill_used] );

        } else if( jo.has_array( "decomp_learn" ) ) {
            for( JsonArray arr : jo.get_array( "decomp_learn" ) ) {
                learn_by_disassembly[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }
        }
    }

    if( jo.has_member( "book_learn" ) ) {
        booksets.clear();
        for( JsonArray arr : jo.get_array( "book_learn" ) ) {
            booksets.emplace( arr.get_string( 0 ), arr.size() > 1 ? arr.get_int( 1 ) : -1 );
        }
    }

    if( jo.has_member( "delete_flags" ) ) {
        flags_to_delete = jo.get_tags( "delete_flags" );
    }

    // recipes not specifying any external requirements inherit from their parent recipe (if any)
    if( jo.has_string( "using" ) ) {
        reqs_external = { { requirement_id( jo.get_string( "using" ) ), 1 } };

    } else if( jo.has_array( "using" ) ) {
        reqs_external.clear();
        for( JsonArray cur : jo.get_array( "using" ) ) {
            reqs_external.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    }

    const std::string type = jo.get_string( "type" );

    if( type == "recipe" ) {
        if( jo.has_string( "id_suffix" ) ) {
            if( abstract ) {
                jo.throw_error( "abstract recipe cannot specify id_suffix", "id_suffix" );
            }
            ident_ = recipe_id( ident_.str() + "_" + jo.get_string( "id_suffix" ) );
        }

        assign( jo, "category", category, strict );
        assign( jo, "subcategory", subcategory, strict );
        assign( jo, "description", description, strict );
        assign( jo, "reversible", reversible, strict );

        if( jo.has_member( "byproducts" ) ) {
            if( this->reversible ) {
                jo.throw_error( "Recipe cannot be reversible and have byproducts" );
            }
            byproducts.clear();
            for( JsonArray arr : jo.get_array( "byproducts" ) ) {
                byproducts[ arr.get_string( 0 ) ] += arr.size() == 2 ? arr.get_int( 1 ) : 1;
            }
        }
        assign( jo, "construction_blueprint", blueprint );
        if( !blueprint.empty() ) {
            assign( jo, "blueprint_name", bp_name );
            bp_resources.clear();
            for( const std::string &resource : jo.get_array( "blueprint_resources" ) ) {
                bp_resources.emplace_back( resource );
            }
            for( JsonObject provide : jo.get_array( "blueprint_provides" ) ) {
                bp_provides.emplace_back( std::make_pair( provide.get_string( "id" ),
                                          provide.get_int( "amount", 1 ) ) );
            }
            // all blueprints provide themselves with needing it written in JSON
            bp_provides.emplace_back( std::make_pair( result_, 1 ) );
            for( JsonObject require : jo.get_array( "blueprint_requires" ) ) {
                bp_requires.emplace_back( std::make_pair( require.get_string( "id" ),
                                          require.get_int( "amount", 1 ) ) );
            }
            // all blueprints exclude themselves with needing it written in JSON
            bp_excludes.emplace_back( std::make_pair( result_, 1 ) );
            for( JsonObject exclude : jo.get_array( "blueprint_excludes" ) ) {
                bp_excludes.emplace_back( std::make_pair( exclude.get_string( "id" ),
                                          exclude.get_int( "amount", 1 ) ) );
            }
            assign( jo, "blueprint_autocalc", bp_autocalc );
        }
    } else if( type == "uncraft" ) {
        reversible = true;
    } else {
        jo.throw_error( "unknown recipe type", "type" );
    }

    // inline requirements are always replaced (cannot be inherited)
    const requirement_id req_id( string_format( "inline_%s_%s", type.c_str(), ident_.c_str() ) );
    requirement_data::load_requirement( jo, req_id );
    reqs_internal = { { req_id, 1 } };
}

void recipe::finalize()
{
    if( bp_autocalc ) {
        add_bp_autocalc_requirements();
    }
    // concatenate both external and inline requirements
    add_requirements( reqs_external );
    add_requirements( reqs_internal );

    reqs_external.clear();
    reqs_internal.clear();

    if( bp_autocalc ) {
        requirements_.consolidate();
    }

    if( contained && container == "null" ) {
        container = item::find_type( result_ )->default_container.value_or( "null" );
    }

    if( autolearn && autolearn_requirements.empty() ) {
        autolearn_requirements = required_skills;
        if( skill_used ) {
            autolearn_requirements[ skill_used ] = difficulty;
        }
    }
}

void recipe::add_requirements( const std::vector<std::pair<requirement_id, int>> &reqs )
{
    requirements_ = std::accumulate( reqs.begin(), reqs.end(), requirements_,
    []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
        return lhs + ( *rhs.first * rhs.second );
    } );
}

std::string recipe::get_consistency_error() const
{
    if( category == "CC_BUILDING" ) {
        if( is_blueprint() || oter_str_id( result_.c_str() ).is_valid() ) {
            return std::string();
        }
        return "defines invalid result";
    }

    if( !item::type_is_defined( result_ ) ) {
        return "defines invalid result";
    }

    if( charges && !item::count_by_charges( result_ ) ) {
        return "specifies charges but result is not counted by charges";
    }

    const auto is_invalid_bp = []( const std::pair<itype_id, int> &elem ) {
        return !item::type_is_defined( elem.first );
    };

    if( std::any_of( byproducts.begin(), byproducts.end(), is_invalid_bp ) ) {
        return "defines invalid byproducts";
    }

    if( !contained && container != "null" ) {
        return "defines container but not contained";
    }

    if( !item::type_is_defined( container ) ) {
        return "specifies unknown container";
    }

    const auto is_invalid_skill = []( const std::pair<skill_id, int> &elem ) {
        return !elem.first.is_valid();
    };

    if( ( skill_used && !skill_used.is_valid() ) ||
        std::any_of( required_skills.begin(), required_skills.end(), is_invalid_skill ) ) {
        return "uses invalid skill";
    }

    const auto is_invalid_book = []( const std::pair<itype_id, int> &elem ) {
        return !item::find_type( elem.first )->book;
    };

    if( std::any_of( booksets.begin(), booksets.end(), is_invalid_book ) ) {
        return "defines invalid book";
    }

    return std::string();
}

item recipe::create_result() const
{
    item newit( result_, calendar::turn, item::default_charges_tag{} );
    if( charges ) {
        newit.charges = *charges;
    }

    if( !newit.craft_has_charges() ) {
        newit.charges = 0;
    } else if( result_mult != 1 ) {
        // TODO: Make it work for charge-less items
        newit.charges *= result_mult;
    }

    if( contained ) {
        newit = newit.in_container( container );
    }

    return newit;
}

std::vector<item> recipe::create_results( int batch ) const
{
    std::vector<item> items;

    const bool by_charges = item::count_by_charges( result_ );
    if( contained || !by_charges ) {
        // by_charges items get their charges multiplied in create_result
        const int num_results = by_charges ? batch : batch * result_mult;
        for( int i = 0; i < num_results; i++ ) {
            item newit = create_result();
            items.push_back( newit );
        }
    } else {
        item newit = create_result();
        newit.charges *= batch;
        items.push_back( newit );
    }

    return items;
}

std::vector<item> recipe::create_byproducts( int batch ) const
{
    std::vector<item> bps;
    for( const auto &e : byproducts ) {
        item obj( e.first, calendar::turn, item::default_charges_tag{} );
        if( obj.has_flag( "VARSIZE" ) ) {
            obj.item_tags.insert( "FIT" );
        }

        if( obj.count_by_charges() ) {
            obj.charges *= e.second * batch;
            bps.push_back( obj );

        } else {
            if( !obj.craft_has_charges() ) {
                obj.charges = 0;
            }
            for( int i = 0; i < e.second * batch; ++i ) {
                bps.push_back( obj );
            }
        }
    }
    return bps;
}

bool recipe::has_byproducts() const
{
    return !byproducts.empty();
}

std::string recipe::primary_skill_string( const Character *c, bool print_skill_level ) const
{
    std::list< std::pair<skill_id, int> > skillList;

    if( !skill_used.is_null() ) {
        skillList.push_back( std::pair<skill_id, int>( skill_used, difficulty ) );
    }

    return required_skills_string( skillList.begin(), skillList.end(), c, print_skill_level );
}

std::string recipe::required_skills_string( const Character *c, bool include_primary_skill,
        bool print_skill_level ) const
{
    // There is no primary skill used or it shouldn't be included then we can just use the required_skills directly.
    if( skill_used.is_null() || !include_primary_skill ) {
        return required_skills_string( required_skills.begin(), required_skills.end(), c,
                                       print_skill_level );
    }

    std::list< std::pair<skill_id, int> > skillList;
    skillList.push_back( std::pair<skill_id, int>( skill_used, difficulty ) );
    for( const std::pair<skill_id, int> &skill : required_skills ) {
        skillList.push_back( skill );
    }

    return required_skills_string( skillList.begin(), skillList.end(), c, print_skill_level );
}

std::string recipe::required_all_skills_string() const
{
    // There is no primary skill used, we can just use the required_skills directly.
    if( skill_used == skill_id( "none" ) ) {
        return required_skills_string( required_skills.begin(), required_skills.end() );
    }

    std::list< std::pair<skill_id, int> > skillList;
    skillList.push_back( std::pair<skill_id, int>( skill_used, difficulty ) );
    for( const std::pair<skill_id, int> &skill : required_skills ) {
        skillList.push_back( skill );
    }

    return required_skills_string( skillList.begin(), skillList.end() );
}

std::string recipe::batch_savings_string() const
{
    return ( batch_rsize != 0 ) ?
           string_format( _( "%s%% at >%s units" ), static_cast<int>( batch_rscale * 100 ), batch_rsize )
           : _( "none" );
}

std::string recipe::result_name() const
{
    std::string name = item::nname( result_ );
    if( uistate.favorite_recipes.find( this->ident() ) != uistate.favorite_recipes.end() ) {
        name = "* " + name;
    }

    return name;
}

bool recipe::will_be_blacklisted() const
{
    if( requirements_.is_blacklisted() ) {
        return true;
    }

    auto any_is_blacklisted = []( const std::vector<std::pair<requirement_id, int>> &reqs ) {
        auto req_is_blacklisted = []( const std::pair<requirement_id, int> &req ) {
            return req.first->is_blacklisted();
        };

        return std::any_of( reqs.begin(), reqs.end(), req_is_blacklisted );
    };

    return any_is_blacklisted( reqs_internal ) || any_is_blacklisted( reqs_external );
}

std::function<bool( const item & )> recipe::get_component_filter(
    const recipe_filter_flags flags ) const
{
    const item result = create_result();

    // Disallow crafting of non-perishables with rotten components
    // Make an exception for items with the ALLOW_ROTTEN flag such as seeds
    const bool recipe_forbids_rotten =
        result.is_food() && !result.goes_bad() && !has_flag( "ALLOW_ROTTEN" );
    const bool flags_forbid_rotten =
        static_cast<bool>( flags & recipe_filter_flags::no_rotten ) && result.goes_bad();
    std::function<bool( const item & )> rotten_filter = return_true<item>;
    if( recipe_forbids_rotten || flags_forbid_rotten ) {
        rotten_filter = []( const item & component ) {
            return !component.rotten();
        };
    }

    // If the result is made hot, we can allow frozen components.
    // EDIBLE_FROZEN components ( e.g. flour, chocolate ) are allowed as well
    // Otherwise forbid them
    std::function<bool( const item & )> frozen_filter = return_true<item>;
    if( result.is_food() && !hot_result() ) {
        frozen_filter = []( const item & component ) {
            return !component.has_flag( "FROZEN" ) || component.has_flag( "EDIBLE_FROZEN" );
        };
    }

    // Disallow usage of non-full magazines as components
    // This is primarily used to require a fully charged battery, but works for any magazine.
    std::function<bool( const item & )> magazine_filter = return_true<item>;
    if( has_flag( "FULL_MAGAZINE" ) ) {
        magazine_filter = []( const item & component ) {
            return !component.is_magazine() || ( component.ammo_remaining() >= component.ammo_capacity() );
        };
    }

    return [ rotten_filter, frozen_filter, magazine_filter ]( const item & component ) {
        return is_crafting_component( component ) &&
               rotten_filter( component ) &&
               frozen_filter( component ) &&
               magazine_filter( component );
    };
}

bool recipe::is_blueprint() const
{
    return !blueprint.empty();
}

const std::string &recipe::get_blueprint() const
{
    return blueprint;
}

const translation &recipe::blueprint_name() const
{
    return bp_name;
}

const std::vector<itype_id> &recipe::blueprint_resources() const
{
    return bp_resources;
}

const std::vector<std::pair<std::string, int>> &recipe::blueprint_provides() const
{
    return bp_provides;
}

const std::vector<std::pair<std::string, int>>  &recipe::blueprint_requires() const
{
    return bp_requires;
}

const std::vector<std::pair<std::string, int>>  &recipe::blueprint_excludes() const
{
    return bp_excludes;
}

void recipe::add_bp_autocalc_requirements()
{
    build_reqs total_reqs;
    get_build_reqs_for_furn_ter_ids( get_changed_ids_from_update( blueprint ), total_reqs );
    time = total_reqs.time;
    for( const auto &skill_data : total_reqs.skills ) {
        if( required_skills.find( skill_data.first ) == required_skills.end() ) {
            required_skills[skill_data.first] = skill_data.second;
        } else {
            required_skills[skill_data.first] = std::max( skill_data.second,
                                                required_skills[skill_data.first] );
        }
    }
    for( const auto &req : total_reqs.reqs ) {
        reqs_internal.emplace_back( std::make_pair( req.first, req.second ) );
    }
}

bool recipe::hot_result() const
{
    // Check if the recipe tools make this food item hot upon making it.
    // We don't actually know which specific tool the player used/will use here, but
    // we're checking for a class of tools; because of the way requirements
    // processing works, the "surface_heat" id gets nuked into an actual
    // list of tools, see data/json/recipes/cooking_tools.json.
    //
    // Currently it's only checking for a hotplate because that's a
    // suitable item in both the "surface_heat" and "water_boiling_heat"
    // tools, and it's usually the first item in a list of tools so if this
    // does get heated we'll find it right away.
    //
    // TODO: Make this less of a hack
    if( create_result().is_food() ) {
        const requirement_data::alter_tool_comp_vector &tool_lists = requirements().get_tools();
        for( const std::vector<tool_comp> &tools : tool_lists ) {
            for( const tool_comp &t : tools ) {
                if( t.type == "hotplate" ) {
                    return true;
                }
            }
        }
    }
    return false;
}
