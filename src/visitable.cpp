#include "visitable.h"

#include <algorithm>
#include <climits>
#include <iosfwd>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "active_item_cache.h"
#include "bionics.h"
#include "character.h"
#include "colony.h"
#include "debug.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "make_static.h"
#include "map.h"
#include "map_selector.h"
#include "memory_fast.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "pimpl.h"
#include "point.h"
#include "submap.h"
#include "temp_crafting_inventory.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"

static const bionic_id bio_ups( "bio_ups" );

static const itype_id itype_UPS( "UPS" );
static const itype_id itype_UPS_off( "UPS_off" );
static const itype_id itype_apparatus( "apparatus" );

static const quality_id qual_BUTCHER( "BUTCHER" );

/** @relates visitable */
item *read_only_visitable::find_parent( const item &it ) const
{
    item *res = nullptr;
    if( visit_items( [&]( item * node, item * parent ) {
    if( node == &it ) {
            res = parent;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } ) != VisitResponse::ABORT ) {
        debugmsg( "Tried to find item parent using an object that doesn't contain it" );
    }
    return res;
}

/** @relates visitable */
std::vector<item *> read_only_visitable::parents( const item &it ) const
{
    std::vector<item *> res;
    for( item *obj = find_parent( it ); obj; obj = find_parent( *obj ) ) {
        res.push_back( obj );
    }
    return res;
}

/** @relates visitable */
bool read_only_visitable::has_item( const item &it ) const
{
    return visit_items( [&it]( const item * node, item * ) {
        return node == &it ? VisitResponse::ABORT : VisitResponse::NEXT;
    } ) == VisitResponse::ABORT;
}

/** @relates visitable */
bool read_only_visitable::has_item_with( const std::function<bool( const item & )> &filter ) const
{
    return visit_items( [&filter]( const item * node, item * ) {
        return filter( *node ) ? VisitResponse::ABORT : VisitResponse::NEXT;
    } ) == VisitResponse::ABORT;
}

/** Sums the two terms, being careful to not trigger overflow.
 * Doesn't handle underflow.
 *
 * @param a The first addend.
 * @param b The second addend.
 * @return the sum of the addends, but truncated to std::numeric_limits<int>::max().
 */
template <typename T>
static T sum_no_wrap( T a, T b )
{
    if( a > std::numeric_limits<T>::max() - b ||
        b > std::numeric_limits<T>::max() - a ) {
        return std::numeric_limits<T>::max();
    }
    return a + b;
}

template <typename T>
static int has_quality_internal( const T &self, const quality_id &qual, int level, int limit )
{
    int qty = 0;

    self.visit_items( [&qual, level, &limit, &qty]( item * e, item * ) {
        if( e->get_quality( qual ) >= level ) {
            qty = sum_no_wrap( qty, static_cast<int>( e->count() ) );
            if( qty >= limit ) {
                // found sufficient items
                return VisitResponse::ABORT;
            }
        }
        return VisitResponse::NEXT;
    } );
    return std::min( qty, limit );
}

static int has_quality_from_vpart( const vehicle &veh, int part, const quality_id &qual, int level,
                                   int limit )
{
    int qty = 0;

    point pos = veh.part( part ).mount;
    for( const auto &n : veh.parts_at_relative( pos, true ) ) {

        // only unbroken parts can provide tool qualities
        if( !veh.part( n ).is_broken() ) {
            auto tq = veh.part_info( n ).qualities;
            auto iter = tq.find( qual );

            // does the part provide this quality?
            if( iter != tq.end() && iter->second >= level ) {
                qty = sum_no_wrap( qty, 1 );
            }
        }
    }
    return std::min( qty, limit );
}

bool read_only_visitable::has_quality( const quality_id &qual, int level, int qty ) const
{
    return has_quality_internal( *this, qual, level, qty ) == qty;
}

/** @relates visitable */
bool inventory::has_quality( const quality_id &qual, int level, int qty ) const
{
    int res = 0;
    for( const auto &stack : this->items ) {
        res += stack.size() * has_quality_internal( stack.front(), qual, level, qty );
        if( res >= qty ) {
            return true;
        }
    }
    return false;
}

/** @relates visitable */
bool vehicle_selector::has_quality( const quality_id &qual, int level, int qty ) const
{
    for( const auto &cursor : *this ) {
        if( cursor.ignore_vpart ) {
            continue;
        }

        qty -= has_quality_from_vpart( cursor.veh, cursor.part, qual, level, qty );
        if( qty <= 0 ) {
            return true;
        }
    }
    return has_quality_internal( *this, qual, level, qty ) == qty;
}

/** @relates visitable */
bool vehicle_cursor::has_quality( const quality_id &qual, int level, int qty ) const
{
    if( !ignore_vpart ) {
        qty -= has_quality_from_vpart( veh, part, qual, level, qty );
    }
    return qty <= 0 ? true : has_quality_internal( *this, qual, level, qty ) == qty;
}

/** @relates visitable */
bool Character::has_quality( const quality_id &qual, int level, int qty ) const
{
    for( const auto &bio : *this->my_bionics ) {
        if( bio.get_quality( qual ) >= level ) {
            if( qty <= 1 ) {
                return true;
            }

            qty--;
        }
    }

    return qty <= 0 ? true : has_quality_internal( *this, qual, level, qty ) == qty;
}

bool read_only_visitable::has_tools( const itype_id &it, int quantity,
                                     const std::function<bool( const item & )> &filter ) const
{
    return has_amount( it, quantity, true, filter );
}

bool read_only_visitable::has_components( const itype_id &it, int quantity,
        const std::function<bool( const item & )> &filter ) const
{
    return has_amount( it, quantity, false, filter );
}

bool read_only_visitable::has_charges( const itype_id &it, int quantity,
                                       const std::function<bool( const item & )> &filter ) const
{
    return ( charges_of( it, INT_MAX, filter ) >= quantity );
}

template <typename T>
static int max_quality_internal( const T &self, const quality_id &qual )
{
    int res = INT_MIN;
    self.visit_items( [&res, &qual]( item * e, item * ) {
        res = std::max( res, e->get_quality( qual ) );
        return VisitResponse::NEXT;
    } );
    return res;
}

static int max_quality_from_vpart( const vehicle &veh, int part, const quality_id &qual )
{
    int res = INT_MIN;

    point pos = veh.part( part ).mount;
    for( const auto &n : veh.parts_at_relative( pos, true ) ) {

        // only unbroken parts can provide tool qualities
        if( !veh.part( n ).is_broken() ) {
            auto tq = veh.part_info( n ).qualities;
            auto iter = tq.find( qual );

            // does the part provide this quality?
            if( iter != tq.end() ) {
                res = std::max( res, iter->second );
            }
        }
    }
    return res;
}

int read_only_visitable::max_quality( const quality_id &qual ) const
{
    return max_quality_internal( *this, qual );
}

/** @relates visitable */
int Character::max_quality( const quality_id &qual ) const
{
    int res = INT_MIN;

    for( const bionic &bio : *my_bionics ) {
        res = std::max( res, bio.get_quality( qual ) );
    }

    if( qual == qual_BUTCHER ) {
        for( const trait_id &mut : get_mutations() ) {
            res = std::max( res, mut->butchering_quality );
        }
    }

    return std::max( res, max_quality_internal( *this, qual ) );
}

int Character::max_quality( const quality_id &qual, int radius )
{
    int res = max_quality( qual );

    if( radius > 0 ) {
        res = std::max( res,
                        crafting_inventory( tripoint_zero, radius, true )
                        .max_quality( qual ) );
    }

    return res;
}

/** @relates visitable */
int vehicle_cursor::max_quality( const quality_id &qual ) const
{
    int vpart = ignore_vpart ? 0 : max_quality_from_vpart( veh, part, qual );
    return std::max( vpart, max_quality_internal( *this, qual ) );
}

/** @relates visitable */
int vehicle_selector::max_quality( const quality_id &qual ) const
{
    int res = INT_MIN;
    for( const auto &e : *this ) {
        res = std::max( res, e.max_quality( qual ) );
    }
    return res;
}

template<typename T, typename V>
static inline std::vector<T> items_with_internal( V &self, const std::function<bool( const item & )>
        &filter )
{
    std::vector<T> res;
    self.visit_items( [&res, &filter]( const item * node, item * ) {
        if( filter( *node ) ) {
            res.push_back( const_cast<T>( node ) );
        }
        return VisitResponse::NEXT;
    } );
    return res;
}

/** @relates visitable */
std::vector<const item *> read_only_visitable::items_with(
    const std::function<bool( const item & )> &filter ) const
{
    return items_with_internal<const item *>( *this, filter );
}
/** @relates visitable */
std::vector<item *> read_only_visitable::items_with(
    const std::function<bool( const item & )> &filter )
{
    return items_with_internal<item *>( *this, filter );
}

static VisitResponse visit_internal( const std::function<VisitResponse( item *, item * )> &func,
                                     const item *node, item *parent = nullptr )
{
    // hack to avoid repetition
    item *m_node = const_cast<item *>( node );

    switch( func( m_node, parent ) ) {
        case VisitResponse::ABORT:
            return VisitResponse::ABORT;

        case VisitResponse::NEXT:
            if( m_node->visit_contents( func, m_node ) == VisitResponse::ABORT ) {
                return VisitResponse::ABORT;
            }
        /* intentional fallthrough */

        case VisitResponse::SKIP:
            return VisitResponse::NEXT;
    }

    /* never reached but suppresses GCC warning */
    return VisitResponse::ABORT;
}

VisitResponse item::visit_contents( const std::function<VisitResponse( item *, item * )>
                                    &func, item *parent )
{
    return contents.visit_contents( func, parent );
}

VisitResponse item_contents::visit_contents( const std::function<VisitResponse( item *, item * )>
        &func, item *parent )
{
    for( item_pocket &pocket : contents ) {
        if( !pocket.is_type( item_pocket::pocket_type::CONTAINER ) ) {
            // anything that is not CONTAINER is accessible only via its specific accessor
            return VisitResponse::NEXT;
        }
        switch( pocket.visit_contents( func, parent ) ) {
            case VisitResponse::ABORT:
                return VisitResponse::ABORT;
            default:
                break;
        }
    }
    return VisitResponse::NEXT;
}

VisitResponse item_pocket::visit_contents( const std::function<VisitResponse( item *, item * )>
        &func, item *parent )
{
    for( item &e : contents ) {
        switch( visit_internal( func, &e, parent ) ) {
            case VisitResponse::ABORT:
                return VisitResponse::ABORT;
            default:
                break;
        }
    }
    return VisitResponse::NEXT;
}

/** @relates visitable */
VisitResponse item::visit_items(
    const std::function<VisitResponse( item *, item * )> &func ) const
{
    return visit_internal( func, this );
}

/** @relates visitable */
VisitResponse inventory::visit_items(
    const std::function<VisitResponse( item *, item * )> &func ) const
{
    for( const auto &stack : items ) {
        for( const auto &it : stack ) {
            if( visit_internal( func, &it ) == VisitResponse::ABORT ) {
                return VisitResponse::ABORT;
            }
        }
    }
    return VisitResponse::NEXT;
}

/** @relates visitable */
VisitResponse temp_crafting_inventory::visit_items(
    const std::function<VisitResponse( item *, item * )> &func ) const
{
    for( item *it : items ) {
        if( visit_internal( func, it ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

/** @relates visitable */
VisitResponse Character::visit_items( const std::function<VisitResponse( item *, item * )> &func )
const
{
    if( !weapon.is_null() &&
        visit_internal( func, &weapon ) == VisitResponse::ABORT ) {
        return VisitResponse::ABORT;
    }

    for( const auto &e : worn ) {
        if( visit_internal( func, &e ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }

    for( const item *e : get_pseudo_items() ) {
        if( visit_internal( func, e ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }

    return inv->visit_items( func );
}

/** @relates visitable */
VisitResponse map_cursor::visit_items(
    const std::function<VisitResponse( item *, item * )> &func ) const
{
    map &here = get_map();
    // skip inaccessible items
    if( here.has_flag( ter_furn_flag::TFLAG_SEALED, pos() ) &&
        !here.has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, pos() ) ) {
        return VisitResponse::NEXT;
    }

    for( item &e : here.i_at( pos() ) ) {
        if( visit_internal( func, &e ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

/** @relates visitable */
VisitResponse map_selector::visit_items(
    const std::function<VisitResponse( item *, item * )> &func ) const
{
    for( auto &cursor : * ( const_cast<map_selector *>( this ) ) ) {
        if( cursor.visit_items( func ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

/** @relates visitable */
VisitResponse vehicle_cursor::visit_items(
    const std::function<VisitResponse( item *, item * )> &func ) const
{
    int idx = veh.part_with_feature( part, "CARGO", true );
    if( idx >= 0 ) {
        for( auto &e : veh.get_items( idx ) ) {
            if( visit_internal( func, &e ) == VisitResponse::ABORT ) {
                return VisitResponse::ABORT;
            }
        }
    }
    return VisitResponse::NEXT;
}

/** @relates visitable */
VisitResponse vehicle_selector::visit_items(
    const std::function<VisitResponse( item *, item * )> &func ) const
{
    for( const auto &cursor :  *this ) {
        if( cursor.visit_items( func ) == VisitResponse::ABORT ) {
            return VisitResponse::ABORT;
        }
    }
    return VisitResponse::NEXT;
}

/** @relates visitable */
item visitable::remove_item( item &it )
{
    auto obj = remove_items_with( [&it]( const item & e ) {
        return &e == &it;
    }, 1 );
    if( !obj.empty() ) {
        return obj.front();

    } else {
        debugmsg( "Tried removing item from object which did not contain it" );
        return item();
    }
}

/** @relates visitable */
std::list<item> item::remove_items_with( const std::function<bool( const item &e )>
        &filter, int count )
{
    std::list<item> res;

    if( count <= 0 ) {
        // nothing to do
        return res;
    }

    contents.remove_internal( filter, count, res );
    return res;
}

/** @relates visitable */
std::list<item> inventory::remove_items_with( const
        std::function<bool( const item &e )> &filter, int count )
{
    std::list<item> res;

    if( count <= 0 ) {
        // nothing to do
        return res;
    }

    for( auto stack = items.begin(); stack != items.end() && count > 0; ) {
        std::list<item> &istack = *stack;
        const char original_invlet = istack.front().invlet;

        for( auto istack_iter = istack.begin(); istack_iter != istack.end() && count > 0; ) {
            if( filter( *istack_iter ) ) {
                count--;
                res.splice( res.end(), istack, istack_iter++ );
                // The non-first items of a stack may have different invlets, the code
                // in inventory only ever checks the invlet of the first item. This
                // ensures that the first item of a stack always has the same invlet, even
                // after the original first item was removed.
                if( istack_iter == istack.begin() && istack_iter != istack.end() ) {
                    istack_iter->invlet = original_invlet;
                }

            } else {
                istack_iter->remove_internal( filter, count, res );
                ++istack_iter;
            }
        }

        if( istack.empty() ) {
            stack = items.erase( stack );
        } else {
            ++stack;
        }
    }

    // Invalidate binning cache
    binned = false;

    return res;
}

/** @relates visitable */
std::list<item> Character::remove_items_with( const
        std::function<bool( const item &e )> &filter, int count )
{
    std::list<item> res;

    if( count <= 0 ) {
        // nothing to do
        return res;
    }

    // first try and remove items from the inventory
    res = inv->remove_items_with( filter, count );
    count -= res.size();
    if( count == 0 ) {
        return res;
    }

    // then try any worn items
    for( auto iter = worn.begin(); iter != worn.end(); ) {
        if( filter( *iter ) ) {
            iter->on_takeoff( *this );
            res.splice( res.end(), worn, iter++ );
            if( --count == 0 ) {
                return res;
            }
        } else {
            iter->remove_internal( filter, count, res );
            if( count == 0 ) {
                return res;
            }
            ++iter;
        }
    }

    // finally try the currently wielded item (if any)
    if( filter( weapon ) ) {
        res.push_back( remove_weapon() );
        count--;
    } else {
        weapon.remove_internal( filter, count, res );
    }

    return res;
}

/** @relates visitable */
std::list<item> map_cursor::remove_items_with( const
        std::function<bool( const item &e )> &filter, int count )
{
    std::list<item> res;

    if( count <= 0 ) {
        // nothing to do
        return res;
    }

    map &here = get_map();
    if( !here.inbounds( pos() ) ) {
        debugmsg( "cannot remove items from map: cursor out-of-bounds" );
        return res;
    }

    // fetch the appropriate item stack
    point offset;
    submap *sub = here.get_submap_at( pos(), offset );
    cata::colony<item> &stack = sub->get_items( offset );

    for( auto iter = stack.begin(); iter != stack.end(); ) {
        if( filter( *iter ) ) {
            // remove from the active items cache (if it isn't there does nothing)
            sub->active_items.remove( &*iter );

            // if necessary remove item from the luminosity map
            sub->update_lum_rem( offset, *iter );

            // finally remove the item
            res.push_back( *iter );
            iter = stack.erase( iter );

            if( --count == 0 ) {
                return res;
            }
        } else {
            iter->remove_internal( filter, count, res );
            if( count == 0 ) {
                return res;
            }
            ++iter;
        }
    }
    here.update_submap_active_item_status( pos() );
    return res;
}

/** @relates visitable */
std::list<item> map_selector::remove_items_with( const
        std::function<bool( const item &e )> &filter, int count )
{
    std::list<item> res;

    for( auto &cursor : *this ) {
        std::list<item> out = cursor.remove_items_with( filter, count );
        count -= out.size();
        res.splice( res.end(), out );
    }

    return res;
}

/** @relates visitable */
std::list<item> vehicle_cursor::remove_items_with( const
        std::function<bool( const item &e )> &filter, int count )
{
    std::list<item> res;

    if( count <= 0 ) {
        // nothing to do
        return res;
    }

    int idx = veh.part_with_feature( part, "CARGO", false );
    if( idx < 0 ) {
        return res;
    }

    vehicle_part &p = veh.part( idx );
    for( auto iter = p.items.begin(); iter != p.items.end(); ) {
        if( filter( *iter ) ) {
            // remove from the active items cache (if it isn't there does nothing)
            veh.active_items.remove( &*iter );

            res.push_back( *iter );
            iter = p.items.erase( iter );

            if( --count == 0 ) {
                return res;
            }
        } else {
            iter->remove_internal( filter, count, res );
            if( count == 0 ) {
                return res;
            }
            ++iter;
        }
    }

    if( !res.empty() ) {
        // if we removed any items then invalidate the cached mass
        veh.invalidate_mass();
    }

    return res;
}

/** @relates visitable */
std::list<item> vehicle_selector::remove_items_with( const
        std::function<bool( const item &e )> &filter, int count )
{
    std::list<item> res;

    for( auto &cursor : *this ) {
        std::list<item> out = cursor.remove_items_with( filter, count );
        count -= out.size();
        res.splice( res.end(), out );
    }

    return res;
}

template <typename T, typename M>
static int charges_of_internal( const T &self, const M &main, const itype_id &id, int limit,
                                const std::function<bool( const item & )> &filter,
                                const std::function<void( int )> &visitor )
{
    int qty = 0;

    bool found_tool_with_UPS = false;
    bool found_bionic_tool = false;
    self.visit_items( [&]( const item * e, item * ) {
        if( filter( *e ) && id == e->typeId() && !e->is_broken() ) {
            if( id != itype_UPS_off ) {
                if( e->count_by_charges() ) {
                    qty = sum_no_wrap( qty, e->charges );
                } else {
                    qty = sum_no_wrap( qty, e->ammo_remaining() );
                }
                if( e->has_flag( STATIC( flag_id( "USE_UPS" ) ) ) ) {
                    found_tool_with_UPS = true;
                } else if( e->has_flag( STATIC( flag_id( "USES_BIONIC_POWER" ) ) ) ) {
                    found_bionic_tool = true;
                }
            } else if( id == itype_UPS_off && e->has_flag( STATIC( flag_id( "IS_UPS" ) ) ) ) {
                qty = sum_no_wrap( qty, e->ammo_remaining() );
            }
        }
        if( qty >= limit ) {
            return VisitResponse::ABORT;
        }
        // recurse through nested containers if any
        return e->is_container() ? VisitResponse::NEXT : VisitResponse::SKIP;
    } );

    if( found_tool_with_UPS && qty < limit && get_player_character().has_active_bionic( bio_ups ) ) {
        qty = sum_no_wrap( qty, static_cast<int>( units::to_kilojoule(
                               get_player_character().get_power_level() ) ) );
    }

    if( found_bionic_tool ) {
        qty = sum_no_wrap( qty, static_cast<int>( units::to_kilojoule(
                               get_player_character().get_power_level() ) ) );
    }

    if( qty < limit && found_tool_with_UPS ) {
        int used_ups = main.charges_of( itype_UPS, limit - qty );
        qty += used_ups;
        if( visitor ) {
            visitor( used_ups );
        }
    }

    return std::min( qty, limit );
}

template <typename T>
static std::pair<int, int> kcal_range_of_internal( const T &self, const itype_id &id,
        const std::function<bool( const item & )> &filter, Character &player_character )
{
    std::pair<int, int> result( INT_MAX, INT_MIN );
    self.visit_items( [&result, &id, &filter, &player_character]( const item * e, item * ) {
        if( e->typeId() == id && filter( *e ) ) {
            int kcal = player_character.compute_effective_nutrients( *e ).kcal();
            if( kcal < result.first ) {
                result.first = kcal;
            }
            if( kcal > result.second ) {
                result.second = kcal;
            }
        }
        return VisitResponse::NEXT;
    } );
    return result;
}

std::pair<int, int> read_only_visitable::kcal_range( const itype_id &id,
        const std::function<bool( const item & )> &filter, Character &player_character )
{
    return kcal_range_of_internal( *this, id, filter, player_character );
}

std::pair<int, int> inventory::kcal_range( const itype_id &id,
        const std::function<bool( const item & )> &filter, Character &player_character )
{
    return kcal_range_of_internal( *this, id, filter, player_character );
}

std::pair<int, int> Character::kcal_range( const itype_id &id,
        const std::function<bool( const item & )> &filter, Character &player_character )
{
    return kcal_range_of_internal( *this, id, filter, player_character );
}

/** @relates visitable */
int read_only_visitable::charges_of( const itype_id &what, int limit,
                                     const std::function<bool( const item & )> &filter,
                                     const std::function<void( int )> &visitor ) const
{
    if( what == itype_UPS ) {
        int qty = 0;
        qty = sum_no_wrap( qty, charges_of( itype_UPS_off ) );
        return std::min( qty, limit );
    }

    return charges_of_internal( *this, *this, what, limit, filter, visitor );
}

/** @relates visitable */
int inventory::charges_of( const itype_id &what, int limit,
                           const std::function<bool( const item & )> &filter,
                           const std::function<void( int )> &visitor ) const
{
    if( what == itype_UPS ) {
        int qty = 0;
        qty = sum_no_wrap( qty, charges_of( itype_UPS_off ) );
        return std::min( qty, limit );
    }

    const itype_bin &binned = get_binned_items();
    const auto iter = binned.find( what );
    if( iter == binned.end() ) {
        return 0;
    }

    int res = 0;
    for( const item *it : iter->second ) {
        res = sum_no_wrap( res, charges_of_internal( *it, *this, what, limit, filter, visitor ) );
        if( res >= limit ) {
            break;
        }
    }
    return std::min( limit, res );
}

/** @relates visitable */
int Character::charges_of( const itype_id &what, int limit,
                           const std::function<bool( const item & )> &filter,
                           const std::function<void( int )> &visitor ) const
{
    if( what == itype_UPS ) {
        int ups_power = available_ups();
        if( has_active_bionic( bio_ups ) ) {
            ups_power -= units::to_kilojoule( get_power_level() );
        }
        return std::min( ups_power, limit );
    }
    return charges_of_internal( *this, *this, what, limit, filter, visitor );
}

template <typename T>
static int amount_of_internal( const T &self, const itype_id &id, bool pseudo, int limit,
                               const std::function<bool( const item & )> &filter )
{
    int qty = 0;
    self.visit_items( [&qty, &id, &pseudo, &limit, &filter]( const item * e, item * ) {
        if( !e->has_flag( STATIC( flag_id( "ITEM_BROKEN" ) ) ) &&
            ( id == STATIC( itype_id( "any" ) ) || e->typeId() == id ) && filter( *e ) &&
            ( pseudo || !e->has_flag( STATIC( flag_id( "PSEUDO" ) ) ) ) ) {
            qty = sum_no_wrap( qty, 1 );
        }
        return qty != limit ? VisitResponse::NEXT : VisitResponse::ABORT;
    } );
    return qty;
}

/** @relates visitable */
int read_only_visitable::amount_of( const itype_id &what, bool pseudo, int limit,
                                    const std::function<bool( const item & )> &filter ) const
{
    return amount_of_internal( *this, what, pseudo, limit, filter );
}

/** @relates visitable */
int inventory::amount_of( const itype_id &what, bool pseudo, int limit,
                          const std::function<bool( const item & )> &filter ) const
{
    const auto &binned = get_binned_items();
    const auto iter = binned.find( what );
    if( iter == binned.end() && what != STATIC( itype_id( "any" ) ) ) {
        return 0;
    }

    int res = 0;
    if( what.str() == "any" ) {
        for( const auto &kv : binned ) {
            for( const item *it : kv.second ) {
                res = sum_no_wrap( res, it->amount_of( what, pseudo, limit, filter ) );
            }
        }
    } else {
        for( const item *it : iter->second ) {
            res = sum_no_wrap( res, it->amount_of( what, pseudo, limit, filter ) );
        }
    }

    return std::min( limit, res );
}

/** @relates visitable */
int Character::amount_of( const itype_id &what, bool pseudo, int limit,
                          const std::function<bool( const item & )> &filter ) const
{
    if( pseudo ) {
        for( const item *pseudos : get_pseudo_items() ) {
            if( pseudos->typeId() == what ) {
                return 1;
            }
        }
    }

    if( what == itype_apparatus && pseudo ) {
        int qty = 0;
        visit_items( [&qty, &limit, &filter]( const item * e, item * ) {
            if( e->get_quality( quality_id( "SMOKE_PIPE" ) ) >= 1 && filter( *e ) ) {
                qty = sum_no_wrap( qty, 1 );
            }
            return qty < limit ? VisitResponse::SKIP : VisitResponse::ABORT;
        } );
        return std::min( qty, limit );
    }

    return amount_of_internal( *this, what, pseudo, limit, filter );
}

/** @relates visitable */
bool read_only_visitable::has_amount( const itype_id &what, int qty, bool pseudo,
                                      const std::function<bool( const item & )> &filter ) const
{
    return amount_of( what, pseudo, qty, filter ) == qty;
}
