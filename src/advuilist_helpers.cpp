#include "advuilist_helpers.h"

#include <algorithm>     // for __niter_base, max
#include <functional>    // for equal_to, function
#include <iterator>      // for operator!=, operator==, make_move...
#include <memory>        // for _Destroy, allocator_traits<>::val...
#include <set>           // for set
#include <string>        // for allocator, basic_string, operator==
#include <tuple>         // for pair::pair<_T1, _T2>
#include <unordered_map> // for unordered_map, unordered_map<>::i...
#include <utility>       // for move, get, pair
#include <vector>        // for vector

#include "advuilist.h"         // for advuilist
#include "advuilist_const.h"   // for ACTION_EXAMINE
#include "advuilist_sourced.h" // for advuilist_sourced
#include "character.h"         // for get_player_character, Character
#include "color.h"             // for c_white, color_manager
#include "item.h"              // for item, iteminfo
#include "item_category.h"     // for item_category
#include "item_location.h"     // for item_location
#include "item_search.h"       // for item_filter_from_string
#include "map.h"               // for get_map, map_stack, map
#include "map_selector.h"      // for map_cursor, map_selector
#include "output.h"            // for format_volume, right_print, item_...
#include "point.h"             // for tripoint
#include "string_formatter.h"  // for string_format
#include "translations.h"      // for _
#include "type_id.h"           // for itype_id, hash
#include "units.h"             // for quantity, operator""_kilogram
#include "units_utility.h"     // for convert_weight, volume_units_abbr

namespace
{
// FIXME: this string is duplicated from draw_item_filter_rules() because that function doesn't fit
// anywhere in the current implementation of advuilist
std::string const desc = string_format(
    "%s\n\n%s\n %s\n\n%s\n %s\n\n%s\n %s", _( "Type part of an item's name to filter it." ),
    _( "Separate multiple items with [<color_yellow>,</color>]." ),
    _( "Example: back,flash,aid, ,band" ),
    _( "To exclude items, place [<color_yellow>-</color>] in front." ),
    _( "Example: -pipe,-chunk,-steel" ),
    _( "Search [<color_yellow>c</color>]ategory, [<color_yellow>m</color>]aterial, "
       "[<color_yellow>q</color>]uality, [<color_yellow>n</color>]otes or "
       "[<color_yellow>d</color>]isassembled components." ),
    _( "Examples: c:food,m:iron,q:hammering,n:toolshelf,d:pipe" ) );
} // namespace

namespace advuilist_helpers
{
iloc_stack_t get_stacks( map_cursor &cursor )
{
    iloc_stack_t stacks;
    map_stack items = get_map().i_at( tripoint( cursor ) );
    std::unordered_map<itype_id, std::set<int>> cache;
    for( item &elem : items ) {
        const auto id = elem.typeId();
        auto iter = cache.find( id );
        bool got_stacked = false;
        if( iter != cache.end() ) {
            for( auto const &idx : iter->second ) {
                got_stacked = stacks[idx].stack.front()->display_stacked_with( elem );
                if( got_stacked ) {
                    stacks[idx].stack.emplace_back( cursor, &elem );
                    break;
                }
            }
        }
        if( !got_stacked ) {
            cache[id].insert( stacks.size() );
            stacks.emplace_back( iloc_entry{ { item_location{ cursor, &elem } } } );
        }
    }

    return stacks;
}

std::size_t iloc_entry_counter( iloc_entry const &it )
{
    return it.stack[0]->count_by_charges() ? it.stack[0]->charges : it.stack.size();
}

void collect_stats( aim_stats_t *stats, iloc_entry const &it )
{
    for( auto const &v : it.stack ) {
        stats->first += v->weight();
        stats->second += v->volume();
    }
}

template <class Container>
void setup_for_aim( advuilist<Container, iloc_entry> *myadvuilist, aim_stats_t *stats )
{
    using myuilist_t = advuilist<Container, iloc_entry>;
    using col_t = typename myuilist_t::col_t;
    using sorter_t = typename myuilist_t::sorter_t;
    using grouper_t = typename myuilist_t::grouper_t;
    using filter_t = typename myuilist_t::filter_t;

    myadvuilist->setColumns( std::vector<col_t>{
        { "item", []( iloc_entry const &it ) { return it.stack[0]->display_name(); }, 8.F },
        { "count",
          []( iloc_entry const &it ) {
              std::size_t const itsize = iloc_entry_counter( it );
              return std::to_string( itsize );
          },
          2.F } } );
    myadvuilist->setcountingf( iloc_entry_counter );
    myadvuilist->addSorter( sorter_t{ "count", []( iloc_entry const &l, iloc_entry const &r ) {
                                         std::size_t const lsize = iloc_entry_counter( l );
                                         std::size_t const rsize = iloc_entry_counter( r );
                                         return lsize < rsize;
                                     } } );
    // FIXME: this might be better in the ctxt handler of the top transaction_ui so we can show the
    // info on the opposite pane
    myadvuilist->setexaminef( [myadvuilist]( advuilist_helpers::iloc_entry const &it ) {
        // FIXME: apparently inventory examine needs special handling
        item const &_it = *it.stack.front();
        std::vector<iteminfo> vThisItem;
        std::vector<iteminfo> vDummy;
        _it.info( true, vThisItem );

        item_info_data data( _it.tname(), _it.type_name(), vThisItem, vDummy );
        data.handle_scrolling = true;

        draw_item_info( *myadvuilist->get_window(), data ).get_first_input();
    } );
    myadvuilist->addGrouper( grouper_t{
        "category",
        []( iloc_entry const &it ) { return it.stack[0]->get_category_shallow().sort_rank(); },
        []( iloc_entry const &it ) { return it.stack[0]->get_category_shallow().name(); } } );
    myadvuilist->get_ctxt()->register_action( ACTION_EXAMINE );
    myadvuilist->setfilterf( filter_t{ desc, []( iloc_entry const &it, std::string const &filter ) {
                                          // FIXME: salvage filter caching from old AIM code
                                          auto const filterf = item_filter_from_string( filter );
                                          return filterf( *it.stack[0] );
                                      } } );
    myadvuilist->on_rebuild( [stats]( bool first, advuilist_helpers::iloc_entry const &it ) {
        if( first ) {
            stats->first = 0_kilogram;
            stats->second = 0_liter;
        }
        collect_stats( stats, it );
    } );
    myadvuilist->on_redraw( [stats]( catacurses::window *w ) {
        right_print( *w, 1, 2, c_white,
                     string_format( "%3.1f %s  %s %s", convert_weight( stats->first ),
                                    weight_units(), format_volume( stats->second ),
                                    volume_units_abbr() ) );
    } );
}

template <class Container>
Container source_all( int radius )
{
    Container itemlist;
    for( map_cursor &cursor : map_selector( get_player_character().pos(), radius ) ) {
        Container const &stacks = get_stacks( cursor );
        itemlist.insert( itemlist.end(), std::make_move_iterator( stacks.begin() ),
                         std::make_move_iterator( stacks.end() ) );
    }
    return itemlist;
}

template <class Container>
void add_aim_sources( advuilist_sourced<Container, iloc_entry> *myadvuilist )
{
    using myuilist_t = advuilist_sourced<Container, iloc_entry>;
    using fsource_t = typename myuilist_t::fsource_t;

    fsource_t _source_all = []() { return source_all<Container>(); };
    fsource_t source_dummy = []() { return Container(); };
    myadvuilist->addSource(
        { _( SOURCE_NW ), SOURCE_NW_i, { 3, 0 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_N ), SOURCE_N_i, { 4, 0 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_NE ), SOURCE_NE_i, { 5, 0 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_W ), SOURCE_W_i, { 3, 1 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_CENTER ), SOURCE_CENTER_i, { 4, 1 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_E ), SOURCE_E_i, { 5, 1 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_SW ), SOURCE_SW_i, { 3, 2 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_S ), SOURCE_S_i, { 4, 2 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_SE ), SOURCE_SE_i, { 5, 2 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_CONT ), SOURCE_CONT_i, { 0, 0 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_DRAGGED ), SOURCE_DRAGGED_i, { 1, 0 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_INV ), SOURCE_INV_i, { 1, 1 }, source_dummy, []() { return false; } } );
    myadvuilist->addSource(
        { _( SOURCE_ALL ), SOURCE_ALL_i, { 0, 2 }, _source_all, []() { return true; } } );
    myadvuilist->addSource(
        { _( SOURCE_WORN ), SOURCE_WORN_i, { 1, 2 }, source_dummy, []() { return false; } } );
}

template void setup_for_aim( aim_advuilist_t *myadvuilist, aim_stats_t *stats );
template aim_container_t source_all( int radius );
template void add_aim_sources( aim_advuilist_sourced_t *myadvuilist );

} // namespace advuilist_helpers
