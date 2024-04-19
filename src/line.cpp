#include "line.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <tuple>
#include <utility>

#include "cata_assert.h"
#include "enums.h"
#include "math_defines.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "units_fwd.h"

bool trigdist;

double iso_tangent( double distance, const units::angle &vertex )
{
    return tan( vertex / 2 )  * distance * 2;
}

void bresenham( const point &start, const point &target, int offset,
                const std::function<bool( const point & )> &interact )
{
    // Relative distances and directions to the target
    const point delta = target - start;
    // Single steps towards the target
    const point s( sgn( delta.x ), sgn( delta.y ) );
    // Absolute distances to the target. Scaled by 2 since we can't divide odd numbers in half
    const point a = delta.abs() * 2;

    const int major = std::max( a.x, a.y );
    const int minor = std::min( a.x, a.y );

    point cur = start;

    if( minor == 0 ) {
        // One axis
        if( major == a.x ) {
            while( cur.x != target.x ) {
                cur.x += s.x;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else {
            while( cur.y != target.y ) {
                cur.y += s.y;
                if( !interact( cur ) ) {
                    break;
                }
            }
        }
    } else {
        // Two axes
        if( major == minor ) {
            // Diagonal
            while( cur.x != target.x ) {
                cur.x += s.x;
                cur.y += s.y;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else {
            // Sloped
            if( major == a.x ) {
                const int threshold = a.x / 2 - a.y;
                while( cur.x != target.x ) {
                    if( offset > threshold ) {
                        cur.y += s.y;
                        offset -= a.x;
                    }
                    cur.x += s.x;
                    offset += a.y;
                    if( !interact( cur ) ) {
                        break;
                    }
                }
            } else {
                const int threshold = a.y / 2 - a.x;
                while( cur.y != target.y ) {
                    if( offset > threshold ) {
                        cur.x += s.x;
                        offset -= a.y;
                    }
                    cur.y += s.y;
                    offset += a.x;
                    if( !interact( cur ) ) {
                        break;
                    }
                }
            }
        }
    }
}

void bresenham( const tripoint &start, const tripoint &target, int offset_middle, int offset_minor,
                const std::function<bool( const tripoint & )> &interact )
{
    // Relative distances and directions to the target
    const tripoint delta = target - start;
    // Single steps towards the target
    const tripoint s( sgn( delta.x ), sgn( delta.y ), sgn( delta.z ) );
    // Absolute distances to the target. Scaled by 2 since we can't divide odd numbers in half
    const tripoint a = delta.abs() * 2;

    const int major = std::max( std::max( a.x, a.y ), a.z );
    const int middle = a.x > a.y ? ( a.y > a.z ? a.y : ( a.x > a.z ? a.z : a.x ) ) : ( a.x > a.z ? a.x : ( a.y > a.z ? a.z : a.y ) );
    const int minor = std::min( std::min( a.x, a.y ), a.z );

    tripoint cur = start;

    if( minor == 0 ) {
        // Two or fewer axes
        if( middle == 0 ) {
            // One axis
            if( major == a.x ) {
                while( cur.x != target.x ) {
                    cur.x += s.x;
                    if( !interact( cur ) ) {
                        break;
                    }
                }
            } else if( major == a.y ) {
                while( cur.y != target.y ) {
                    cur.y += s.y;
                    if( !interact( cur ) ) {
                        break;
                    }
                }
            } else {
                while( cur.z != target.z ) {
                    cur.z += s.z;
                    if( !interact( cur ) ) {
                        break;
                    }
                }
            }
        } else {
            // Two axes
            if( major == middle ) {
                // Diagonal on the one plane
                if( minor == a.x ) {
                    while( cur.y != target.y ) {
                        cur.y += s.y;
                        cur.z += s.z;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                } else if( minor == a.y ) {
                    while( cur.z != target.z ) {
                        cur.z += s.z;
                        cur.x += s.x;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                } else {
                    while( cur.x != target.x ) {
                        cur.x += s.x;
                        cur.y += s.y;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                }
            } else {
                // Sloped on the one plane
                if( minor == a.x ) {
                    if( major == a.y ) {
                        const int threshold = a.y / 2 - a.z;
                        while( cur.y != target.y ) {
                            if( offset_middle > threshold ) {
                                cur.z += s.z;
                                offset_middle -= a.y;
                            }
                            cur.y += s.y;
                            offset_middle += a.z;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    } else {
                        const int threshold = a.z / 2 - a.y;
                        while( cur.z != target.z ) {
                            if( offset_middle > threshold ) {
                                cur.y += s.y;
                                offset_middle -= a.z;
                            }
                            cur.z += s.z;
                            offset_middle += a.y;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    }
                } else if( minor == a.y ) {
                    if( major == a.z ) {
                        const int threshold = a.z / 2 - a.x;
                        while( cur.z != target.z ) {
                            if( offset_middle > threshold ) {
                                cur.x += s.x;
                                offset_middle -= a.z;
                            }
                            cur.z += s.z;
                            offset_middle += a.x;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    } else {
                        const int threshold = a.x / 2 - a.z;
                        while( cur.x != target.x ) {
                            if( offset_middle > threshold ) {
                                cur.z += s.z;
                                offset_middle -= a.x;
                            }
                            cur.x += s.x;
                            offset_middle += a.z;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    }
                } else {
                    if( major == a.x ) {
                        const int threshold = a.x / 2 - a.y;
                        while( cur.x != target.x ) {
                            if( offset_middle > threshold ) {
                                cur.y += s.y;
                                offset_middle -= a.x;
                            }
                            cur.x += s.x;
                            offset_middle += a.y;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    } else {
                        const int threshold = a.y / 2 - a.x;
                        while( cur.y != target.y ) {
                            if( offset_middle > threshold ) {
                                cur.x += s.x;
                                offset_middle -= a.y;
                            }
                            cur.y += s.y;
                            offset_middle += a.x;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Three axes
        if( major == middle ) {
            // Diagonal on at least the minor-orthogonal plane
            if( middle == minor ) {
                // Diagonal on all three planes
                while( cur.x != target.x ) {
                    cur.x += s.x;
                    cur.y += s.y;
                    cur.z += s.z;
                    if( !interact( cur ) ) {
                        break;
                    }
                }
            } else {
                // Diagonal on the minor-orthogonal plane and sloped on the other two
                if( minor == a.x ) {
                    const int threshold = a.y / 2 - a.x;
                    while( cur.y != target.y ) {
                        if( offset_minor > threshold ) {
                            cur.x += s.x;
                            offset_minor -= a.y;
                        }
                        cur.y += s.y;
                        cur.z += s.z;
                        offset_minor += a.x;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                } else if( minor == a.y ) {
                    const int threshold = a.z / 2 - a.y;
                    while( cur.z != target.z ) {
                        if( offset_minor > threshold ) {
                            cur.y += s.y;
                            offset_minor -= a.z;
                        }
                        cur.z += s.z;
                        cur.x += s.x;
                        offset_minor += a.y;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                } else {
                    const int threshold = a.x / 2 - a.z;
                    while( cur.x != target.x ) {
                        if( offset_minor > threshold ) {
                            cur.z += s.z;
                            offset_minor -= a.x;
                        }
                        cur.x += s.x;
                        cur.y += s.y;
                        offset_minor += a.z;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                }
            }
        } else {
            // Sloped on at least the minor-orthogonal plane
            if( middle == minor ) {
                // Diagonal on the major-orthogonal plane and sloped on the other two
                // These are equivalent to the sloped on all planes ones but I'm including them for completeness
                if( major == a.x ) {
                    const int threshold_middle = a.x / 2 - a.y;
                    const int threshold_minor = a.x / 2 - a.z;
                    while( cur.x != target.x ) {
                        if( offset_middle > threshold_middle ) {
                            cur.y += s.y;
                            offset_middle -= a.x;
                        }
                        if( offset_minor > threshold_minor ) {
                            cur.z += s.z;
                            offset_minor -= a.x;
                        }
                        cur.x += s.x;
                        offset_middle += a.y;
                        offset_minor += a.z;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                } else if( major == a.y ) {
                    const int threshold_middle = a.y / 2 - a.z;
                    const int threshold_minor = a.y / 2 - a.x;
                    while( cur.y != target.y ) {
                        if( offset_middle > threshold_middle ) {
                            cur.z += s.z;
                            offset_middle -= a.y;
                        }
                        if( offset_minor > threshold_minor ) {
                            cur.x += s.x;
                            offset_minor -= a.y;
                        }
                        cur.y += s.y;
                        offset_middle += a.z;
                        offset_minor += a.x;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                } else {
                    const int threshold_middle = a.z / 2 - a.x;
                    const int threshold_minor = a.z / 2 - a.y;
                    while( cur.z != target.z ) {
                        if( offset_middle > threshold_middle ) {
                            cur.x += s.x;
                            offset_middle -= a.z;
                        }
                        if( offset_minor > threshold_minor ) {
                            cur.y += s.y;
                            offset_minor -= a.z;
                        }
                        cur.z += s.z;
                        offset_middle += a.x;
                        offset_minor += a.y;
                        if( !interact( cur ) ) {
                            break;
                        }
                    }
                }
            } else {
                // Sloped on all three planes
                if( major == a.x ) {
                    if( middle == a.y ) {
                        const int threshold_middle = a.x / 2 - a.y;
                        const int threshold_minor = a.x / 2 - a.z;
                        while( cur.x != target.x ) {
                            if( offset_middle > threshold_middle ) {
                                cur.y += s.y;
                                offset_middle -= a.x;
                            }
                            if( offset_minor > threshold_minor ) {
                                cur.z += s.z;
                                offset_minor -= a.x;
                            }
                            cur.x += s.x;
                            offset_middle += a.y;
                            offset_minor += a.z;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    } else {
                        const int threshold_middle = a.x / 2 - a.z;
                        const int threshold_minor = a.x / 2 - a.y;
                        while( cur.x != target.x ) {
                            if( offset_middle > threshold_middle ) {
                                cur.z += s.z;
                                offset_middle -= a.x;
                            }
                            if( offset_minor > threshold_minor ) {
                                cur.y += s.y;
                                offset_minor -= a.x;
                            }
                            cur.x += s.x;
                            offset_middle += a.z;
                            offset_minor += a.y;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    }
                } else if( major == a.y ) {
                    if( middle == a.z ) {
                        const int threshold_middle = a.y / 2 - a.z;
                        const int threshold_minor = a.y / 2 - a.x;
                        while( cur.y != target.y ) {
                            if( offset_middle > threshold_middle ) {
                                cur.z += s.z;
                                offset_middle -= a.y;
                            }
                            if( offset_minor > threshold_minor ) {
                                cur.x += s.x;
                                offset_minor -= a.y;
                            }
                            cur.y += s.y;
                            offset_middle += a.z;
                            offset_minor += a.x;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    } else {
                        const int threshold_middle = a.y / 2 - a.x;
                        const int threshold_minor = a.y / 2 - a.z;
                        while( cur.y != target.y ) {
                            if( offset_middle > threshold_middle ) {
                                cur.x += s.x;
                                offset_middle -= a.y;
                            }
                            if( offset_minor > threshold_minor ) {
                                cur.z += s.z;
                                offset_minor -= a.y;
                            }
                            cur.y += s.y;
                            offset_middle += a.x;
                            offset_minor += a.z;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    }
                } else {
                    if( middle == a.x ) {
                        const int threshold_middle = a.z / 2 - a.x;
                        const int threshold_minor = a.z / 2 - a.y;
                        while( cur.z != target.z ) {
                            if( offset_middle > threshold_middle ) {
                                cur.x += s.x;
                                offset_middle -= a.z;
                            }
                            if( offset_minor > threshold_minor ) {
                                cur.y += s.y;
                                offset_minor -= a.z;
                            }
                            cur.z += s.z;
                            offset_middle += a.x;
                            offset_minor += a.y;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    } else {
                        const int threshold_middle = a.z / 2 - a.y;
                        const int threshold_minor = a.z / 2 - a.x;
                        while( cur.z != target.z ) {
                            if( offset_middle > threshold_middle ) {
                                cur.y += s.y;
                                offset_middle -= a.z;
                            }
                            if( offset_minor > threshold_minor ) {
                                cur.x += s.x;
                                offset_minor -= a.z;
                            }
                            cur.z += s.z;
                            offset_middle += a.y;
                            offset_minor += a.x;
                            if( !interact( cur ) ) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

//Trying to pull points out of a tripoint vector is messy and
//probably slow, so leaving two full functions for now
std::vector<point> line_to( const point &p1, const point &p2, int t )
{
    std::vector<point> line;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int numCells = square_dist( p1, p2 );
    if( numCells == 0 ) {
        line.push_back( p1 );
    } else {
        line.reserve( numCells );
        bresenham( p1, p2, t, [&line]( const point & new_point ) {
            line.push_back( new_point );
            return true;
        } );
    }
    return line;
}

std::vector<tripoint> line_to( const tripoint &loc1, const tripoint &loc2, int t, int t2 )
{
    std::vector<tripoint> line;
    // Preallocate the number of tiles we need instead of allocating them piecewise.
    const int max_tiles = square_dist( loc1, loc2 );
    if( max_tiles == 0 ) {
        line.push_back( loc1 );
    } else {
        line.reserve( max_tiles );
        bresenham( loc1, loc2, t, t2, [&line]( const tripoint & new_point ) {
            line.push_back( new_point );
            return true;
        } );
    }
    return line;
}

// better line_to
// Returns a line from start to target that includes the tile the interact fails on.
std::vector<point> line_to_2( const point &start, const point &target,
                              const std::function<bool( std::vector<point> & )> &interact, const int offset )
{
    std::vector<point> new_line;
    // Preallocate the max number of tiles we might need instead of allocating them piecewise.
    const int max_tiles = square_dist( start, target );
    if( max_tiles != 0 ) {
        new_line.reserve( max_tiles );
        bresenham( start, target, offset, [&new_line, &interact]( const point & new_point ) {
            new_line.push_back( new_point );
            return interact( new_line );
        } );
    }
    return new_line;
}

std::vector<tripoint> line_to_2( const tripoint &start, const tripoint &target,
                                 const std::function<bool( std::vector<tripoint> & )> &interact,
                                 const int offset_middle, const int offset_minor )
{
    std::vector<tripoint> new_line;
    // Preallocate the max number of tiles we might need instead of allocating them piecewise.
    const int max_tiles = square_dist( start, target );
    if( max_tiles != 0 ) {
        new_line.reserve( max_tiles );
        bresenham( start, target, offset_middle, offset_minor,
        [&new_line, &interact]( const tripoint & new_point ) {
            new_line.push_back( new_point );
            return interact( new_line );
        } );
    }
    return new_line;
}

// Returns a line from start to target that includes the start and the tile the interact fails on.
std::vector<point> line_through_2( const point &start, const point &target,
                                   const std::function<bool( std::vector<point> & )> &interact, const int offset )
{
    std::vector<point> new_line;
    // Preallocate the max number of tiles we might need instead of allocating them piecewise.
    new_line.reserve( square_dist( start, target ) + 1 );
    new_line.push_back( start );
    if( !interact( new_line ) ) {
        return new_line;
    }
    bresenham( start, target, offset, [&new_line, &interact]( const point & new_point ) {
        new_line.push_back( new_point );
        return interact( new_line );
    } );
    return new_line;
}

std::vector<tripoint> line_through_2( const tripoint &start, const tripoint &target,
                                      const std::function<bool( std::vector<tripoint> & )> &interact,
                                      const int offset_middle, const int offset_minor )
{
    std::vector<tripoint> new_line;
    // Preallocate the max number of tiles we might need instead of allocating them piecewise.
    new_line.reserve( square_dist( start, target ) + 1 );
    new_line.push_back( start );
    if( !interact( new_line ) ) {
        return new_line;
    }
    bresenham( start, target, offset_middle, offset_minor,
    [&new_line, &interact]( const tripoint & new_point ) {
        new_line.push_back( new_point );
        return interact( new_line );
    } );
    return new_line;
}

// Tries every line offset between target and start and returns the centermost one.
// If there is no complete line, returns the closest one.
std::vector<tripoint> find_line_to_2( const tripoint &start, const tripoint &target,
                                      const std::function<bool( std::vector<tripoint> & )> &interact )
{
    const tripoint a = ( target - start ).abs();
    const int major = std::max( a.x, a.y ) * 2;
    const int minor = std::min( a.x, a.y ) * 2;
    const int gcd = std::gcd( major, minor );
    const int maximum = major / 2;

    std::vector<tripoint> line;
    std::vector<tripoint> closest_line;
    int line_dist;
    int closest_line_dist = rl_dist( start, target );

    // Iterate over each relevant offset without going out of bounds and into the next tile.
    // If the major axis is odd we start on the exact center,
    // but if it's even we start on the positive side of the two centers.
    for( int offset = maximum % 2 == 0 ; offset < maximum; offset += gcd ) {
        // Try this offset on the positive side...
        line = line_to_2( start, target, interact, offset );
        if( line.back() == target ) {
            return line;
        } else {
            line_dist = rl_dist( line.back(), target );
            if( line_dist < closest_line_dist ) {
                closest_line_dist = line_dist;
                closest_line = line;
            }
            // ...and then on the negitve side. Unless we are at the odd center which has no negative side.
            if( offset != 0 ) {
                line = line_to_2( start, target, interact, -offset );
                if( line.back() == target ) {
                    return line;
                } else {
                    line_dist = rl_dist( line.back(), target );
                    if( line_dist < closest_line_dist ) {
                        closest_line_dist = line_dist;
                        closest_line = line;
                    }
                }
            }
        }
    }
    // If we couldn't find a clear line, return the one that got us the closest.
    return closest_line;
}

float rl_dist_exact( const tripoint &loc1, const tripoint &loc2 )
{
    if( trigdist ) {
        return trig_dist( loc1, loc2 );
    }
    return square_dist( loc1, loc2 );
}

int manhattan_dist( const point &loc1, const point &loc2 )
{
    const point d = ( loc1 - loc2 ).abs();
    return d.x + d.y;
}

int octile_dist( const point &loc1, const point &loc2, int multiplier )
{
    const point d = ( loc1 - loc2 ).abs();
    const int mind = std::min( d.x, d.y );
    // sqrt(2) is approximately 99 / 70
    return ( d.x + d.y - 2 * mind ) * multiplier + mind * multiplier * 99 / 70;
}

float octile_dist_exact( const point &loc1, const point &loc2 )
{
    const point d = ( loc1 - loc2 ).abs();
    const int mind = std::min( d.x, d.y );
    return d.x + d.y - 2 * mind + mind * M_SQRT2;
}

units::angle atan2( const point &p )
{
    return units::atan2( p.y, p.x );
}

units::angle atan2( const rl_vec2d &p )
{
    return units::atan2( p.y, p.x );
}

// This more general version of this function gives correct values for larger values.
unsigned make_xyz( const tripoint &p )
{
    static constexpr double sixteenth_arc = M_PI / 8;
    int vertical_position = ( ( p.z > 0 ) ? 2u : ( p.z < 0 ) ? 1u : 0u ) * 9u;
    if( p.xy() == point_zero ) {
        return vertical_position;
    }
    // Get the arctan of the angle and divide by approximately 22.5 deg to get the octant.
    // the angle is in, then truncate it and map to the right direction.
    // You can read 'octant' as being "number of 22.5 degree sections away from due south".
    // FIXME: atan2 normally takes arguments in ( y, x ) order.  This is
    // passing ( x, y ).
    int octant = atan2( p.x, p.y ) / sixteenth_arc;
    switch( octant ) {
        case 0:
            return direction::SOUTH + vertical_position;
        case 1:
        case 2:
            return direction::SOUTHEAST + vertical_position;
        case 3:
        case 4:
            return direction::EAST + vertical_position;
        case 5:
        case 6:
            return direction::NORTHEAST + vertical_position;
        case -1:
        case -2:
            return direction::SOUTHWEST + vertical_position;
        case -3:
        case -4:
            return direction::WEST + vertical_position;
        case -5:
        case -6:
            return direction::NORTHWEST + vertical_position;
        case 7:
        case 8:
        case -7:
        case -8:
        default:
            return direction::NORTH + vertical_position;
    }
}

// returns the normalized dx, dy, dz for the current line vector.
static std::tuple<double, double, double> slope_of( const std::vector<tripoint> &line )
{
    cata_assert( !line.empty() && line.front() != line.back() );
    const double len = trig_dist( line.front(), line.back() );
    double normDx = ( line.back().x - line.front().x ) / len;
    double normDy = ( line.back().y - line.front().y ) / len;
    double normDz = ( line.back().z - line.front().z ) / len;
    // slope of <x, y, z>
    return std::make_tuple( normDx, normDy, normDz );
}

float get_normalized_angle( const point &start, const point &end )
{
    // Taking the abs value of the difference puts the values in the first quadrant.
    const float absx = std::abs( std::max( start.x, end.x ) - std::min( start.x, end.x ) );
    const float absy = std::abs( std::max( start.y, end.y ) - std::min( start.y, end.y ) );
    const float max = std::max( absx, absy );
    if( max == 0 ) {
        return 0;
    }
    const float min = std::min( absx, absy );
    return min / max;
}

tripoint move_along_line( const tripoint &loc, const std::vector<tripoint> &line,
                          const int distance )
{
    // May want to optimize this, but it's called fairly infrequently as part of specific attack
    // routines, erring on the side of readability.
    tripoint res( loc );
    const auto slope = slope_of( line );
    res.x += distance * std::get<0>( slope );
    res.y += distance * std::get<1>( slope );
    res.z += distance * std::get<2>( slope );
    return res;
}

std::vector<tripoint> continue_line( const std::vector<tripoint> &line, const int distance )
{
    return line_to_2( line.back(), move_along_line( line.back(), line, distance ) );
}

namespace io
{

template<>
std::string enum_to_string<direction>( direction data )
{
    switch( data ) {
        // *INDENT-OFF*
        case direction::ABOVENORTHWEST: return "above_north_west";
        case direction::NORTHWEST: return "north_west";
        case direction::BELOWNORTHWEST: return "below_north_west";
        case direction::ABOVENORTH: return "above_north";
        case direction::NORTH: return "north";
        case direction::BELOWNORTH: return "below_north";
        case direction::ABOVENORTHEAST: return "above_north_east";
        case direction::NORTHEAST: return "north_east";
        case direction::BELOWNORTHEAST: return "below_north_east";

        case direction::ABOVEWEST: return "above_west";
        case direction::WEST: return "west";
        case direction::BELOWWEST: return "below_west";
        case direction::ABOVECENTER: return "above";
        case direction::CENTER: return "center";
        case direction::BELOWCENTER: return "below";
        case direction::ABOVEEAST: return "above_east";
        case direction::EAST: return "east";
        case direction::BELOWEAST: return "below_east";

        case direction::ABOVESOUTHWEST: return "above_south_west";
        case direction::SOUTHWEST: return "south_west";
        case direction::BELOWSOUTHWEST: return "below_south_west";
        case direction::ABOVESOUTH: return "above_south";
        case direction::SOUTH: return "south";
        case direction::BELOWSOUTH: return "below_south";
        case direction::ABOVESOUTHEAST: return "above_south_east";
        case direction::SOUTHEAST: return "south_east";
        case direction::BELOWSOUTHEAST: return "below_south_east";
        // *INDENT-ON*
        case direction::last:
            break;
    }
    cata_fatal( "Invalid direction" );
}

} // namespace io

direction direction_from( const point &p ) noexcept
{
    return static_cast<direction>( make_xyz( tripoint( p, 0 ) ) );
}

direction direction_from( const tripoint &p ) noexcept
{
    return static_cast<direction>( make_xyz( p ) );
}

direction direction_from( const point &p1, const point &p2 ) noexcept
{
    return direction_from( p2 - p1 );
}

direction direction_from( const tripoint &p, const tripoint &q )
{
    return direction_from( q - p );
}

tripoint displace( direction dir )
{
    switch( dir ) {
        case direction::NORTHWEST:
            return tripoint_north_west;
        case direction::ABOVENORTHWEST:
            return point_north_west + tripoint_above;
        case direction::BELOWNORTHWEST:
            return point_north_west + tripoint_below;
        case direction::NORTH:
            return tripoint_north;
        case direction::ABOVENORTH:
            return point_north + tripoint_above;
        case direction::BELOWNORTH:
            return point_north + tripoint_below;
        case direction::NORTHEAST:
            return tripoint_north_east;
        case direction::ABOVENORTHEAST:
            return point_north_east + tripoint_above;
        case direction::BELOWNORTHEAST:
            return point_north_east + tripoint_below;
        case direction::WEST:
            return tripoint_west;
        case direction::ABOVEWEST:
            return point_west + tripoint_above;
        case direction::BELOWWEST:
            return point_west + tripoint_below;
        case direction::CENTER:
            return tripoint_zero;
        case direction::ABOVECENTER:
            return tripoint_above;
        case direction::BELOWCENTER:
            return tripoint_below;
        case direction::EAST:
            return tripoint_east;
        case direction::ABOVEEAST:
            return point_east + tripoint_above;
        case direction::BELOWEAST:
            return point_east + tripoint_below;
        case direction::SOUTHWEST:
            return tripoint_south_west;
        case direction::ABOVESOUTHWEST:
            return point_south_west + tripoint_above;
        case direction::BELOWSOUTHWEST:
            return point_south_west + tripoint_below;
        case direction::SOUTH:
            return tripoint_south;
        case direction::ABOVESOUTH:
            return point_south + tripoint_above;
        case direction::BELOWSOUTH:
            return point_south + tripoint_below;
        case direction::SOUTHEAST:
            return tripoint_south_east;
        case direction::ABOVESOUTHEAST:
            return point_south_east + tripoint_above;
        case direction::BELOWSOUTHEAST:
            return point_south_east + tripoint_below;
        case direction::last:
            cata_fatal( "Invalid direction" );
    }

    return tripoint_zero;
}

point displace_XY( const direction dir )
{
    switch( dir % 9 ) {
        case direction::NORTHWEST:
        case direction::ABOVENORTHWEST:
        case direction::BELOWNORTHWEST:
            return point_north_west;
        case direction::NORTH:
        case direction::ABOVENORTH:
        case direction::BELOWNORTH:
            return point_north;
        case direction::NORTHEAST:
        case direction::ABOVENORTHEAST:
        case direction::BELOWNORTHEAST:
            return point_north_east;
        case direction::WEST:
        case direction::ABOVEWEST:
        case direction::BELOWWEST:
            return point_west;
        case direction::CENTER:
        case direction::ABOVECENTER:
        case direction::BELOWCENTER:
            return point_zero;
        case direction::EAST:
        case direction::ABOVEEAST:
        case direction::BELOWEAST:
            return point_east;
        case direction::SOUTHWEST:
        case direction::ABOVESOUTHWEST:
        case direction::BELOWSOUTHWEST:
            return point_south_west;
        case direction::SOUTH:
        case direction::ABOVESOUTH:
        case direction::BELOWSOUTH:
            return point_south;
        case direction::SOUTHEAST:
        case direction::ABOVESOUTHEAST:
        case direction::BELOWSOUTHEAST:
            return point_south_east;
        case direction::last:
            cata_fatal( "Invalid direction" );
    }

    return point_zero;
}

namespace
{
std::string direction_name_impl( const direction dir, const bool short_name )
{
    enum : int { size = 3 * 3 * 3 };
    static const auto names = [] {
        using pair_t = std::pair<std::string, std::string>;
        std::array < pair_t, size + 1 > result;

        //~ abbreviated direction names and long direction names
        result[static_cast<size_t>( direction::NORTH )]          = pair_t {translate_marker( "N    " ), translate_marker( "north" )};
        result[static_cast<size_t>( direction::NORTHEAST )]      = pair_t {translate_marker( "NE   " ), translate_marker( "northeast" )};
        result[static_cast<size_t>( direction::EAST )]           = pair_t {translate_marker( "E    " ), translate_marker( "east" )};
        result[static_cast<size_t>( direction::SOUTHEAST )]      = pair_t {translate_marker( "SE   " ), translate_marker( "southeast" )};
        result[static_cast<size_t>( direction::SOUTH )]          = pair_t {translate_marker( "S    " ), translate_marker( "south" )};
        result[static_cast<size_t>( direction::SOUTHWEST )]      = pair_t {translate_marker( "SW   " ), translate_marker( "southwest" )};
        result[static_cast<size_t>( direction::WEST )]           = pair_t {translate_marker( "W    " ), translate_marker( "west" )};
        result[static_cast<size_t>( direction::NORTHWEST )]      = pair_t {translate_marker( "NW   " ), translate_marker( "northwest" )};
        result[static_cast<size_t>( direction::ABOVENORTH )]     = pair_t {translate_marker( "UP_N " ), translate_marker( "north and above" )};
        result[static_cast<size_t>( direction::ABOVENORTHEAST )] = pair_t {translate_marker( "UP_NE" ), translate_marker( "northeast and above" )};
        result[static_cast<size_t>( direction::ABOVEEAST )]      = pair_t {translate_marker( "UP_E " ), translate_marker( "east and above" )};
        result[static_cast<size_t>( direction::ABOVESOUTHEAST )] = pair_t {translate_marker( "UP_SE" ), translate_marker( "southeast and above" )};
        result[static_cast<size_t>( direction::ABOVESOUTH )]     = pair_t {translate_marker( "UP_S " ), translate_marker( "south and above" )};
        result[static_cast<size_t>( direction::ABOVESOUTHWEST )] = pair_t {translate_marker( "UP_SW" ), translate_marker( "southwest and above" )};
        result[static_cast<size_t>( direction::ABOVEWEST )]      = pair_t {translate_marker( "UP_W " ), translate_marker( "west and above" )};
        result[static_cast<size_t>( direction::ABOVENORTHWEST )] = pair_t {translate_marker( "UP_NW" ), translate_marker( "northwest and above" )};
        result[static_cast<size_t>( direction::BELOWNORTH )]     = pair_t {translate_marker( "DN_N " ), translate_marker( "north and below" )};
        result[static_cast<size_t>( direction::BELOWNORTHEAST )] = pair_t {translate_marker( "DN_NE" ), translate_marker( "northeast and below" )};
        result[static_cast<size_t>( direction::BELOWEAST )]      = pair_t {translate_marker( "DN_E " ), translate_marker( "east and below" )};
        result[static_cast<size_t>( direction::BELOWSOUTHEAST )] = pair_t {translate_marker( "DN_SE" ), translate_marker( "southeast and below" )};
        result[static_cast<size_t>( direction::BELOWSOUTH )]     = pair_t {translate_marker( "DN_S " ), translate_marker( "south and below" )};
        result[static_cast<size_t>( direction::BELOWSOUTHWEST )] = pair_t {translate_marker( "DN_SW" ), translate_marker( "southwest and below" )};
        result[static_cast<size_t>( direction::BELOWWEST )]      = pair_t {translate_marker( "DN_W " ), translate_marker( "west and below" )};
        result[static_cast<size_t>( direction::BELOWNORTHWEST )] = pair_t {translate_marker( "DN_NW" ), translate_marker( "northwest and below" )};
        result[static_cast<size_t>( direction::ABOVECENTER )]    = pair_t {translate_marker( "UP_CE" ), translate_marker( "above" )};
        result[static_cast<size_t>( direction::CENTER )]         = pair_t {translate_marker( "CE   " ), translate_marker( "center" )};
        result[static_cast<size_t>( direction::BELOWCENTER )]    = pair_t {translate_marker( "DN_CE" ), translate_marker( "below" )};

        result[size] = pair_t {"BUG.  (line.cpp:direction_name)", "BUG.  (line.cpp:direction_name)"};
        return result;
    }();

    int i = static_cast<int>( dir );
    if( i < 0 || i >= size ) {
        i = size;
    }

    return short_name ? _( names[i].first ) : _( names[i].second );
}
} //namespace

std::string direction_name( const direction dir )
{
    return direction_name_impl( dir, false );
}

std::string direction_name_short( const direction dir )
{
    return direction_name_impl( dir, true );
}

std::string direction_arrow( const direction dir )
{
    std::string arrow;
    if( dir == direction::NORTH ) {
        arrow = "\u21D1";
    } else if( dir == direction::NORTHEAST ) {
        arrow = "\u21D7";
    } else if( dir == direction::EAST ) {
        arrow = "\u21D2";
    } else if( dir == direction::SOUTHEAST ) {
        arrow = "\u21D8";
    } else if( dir == direction::SOUTH ) {
        arrow = "\u21D3";
    } else if( dir == direction::SOUTHWEST ) {
        arrow = "\u21D9";
    } else if( dir == direction::WEST ) {
        arrow = "\u21D0";
    } else if( dir == direction::NORTHWEST ) {
        arrow = "\u21D6";
    }
    return arrow;
}

std::string direction_suffix( const tripoint &p, const tripoint &q )
{
    int dist = square_dist( p, q );
    if( dist <= 0 ) {
        return std::string();
    }
    return string_format( "%d%s", dist, trim( direction_name_short( direction_from( p, q ) ) ) );
}

// Cardinals are cardinals. Result is cardinal and adjacent sub-cardinals.
// Sub-Cardinals are sub-cardinals && abs(x) == abs(y). Result is sub-cardinal and adjacent cardinals.
// Sub-sub-cardinals are direction && abs(x) > abs(y) or vice versa.
// Result is adjacent cardinal and sub-cardinals, plus the nearest other cardinal.
// e.g. if the direction is NNE, also include E.
std::vector<tripoint> squares_closer_to( const tripoint &from, const tripoint &to )
{
    std::vector<tripoint> adjacent_closer_squares;
    adjacent_closer_squares.reserve( 5 );
    const tripoint d( -from + to );
    const point a( std::abs( d.x ), std::abs( d.y ) );
    if( d.z != 0 ) {
        adjacent_closer_squares.push_back( from + tripoint( sgn( d.x ), sgn( d.y ), sgn( d.z ) ) );
    }
    if( a.x > a.y ) {
        // X dominant.
        adjacent_closer_squares.push_back( from + point( sgn( d.x ), 0 ) );
        adjacent_closer_squares.push_back( from + point( sgn( d.x ), 1 ) );
        adjacent_closer_squares.push_back( from + point( sgn( d.x ), -1 ) );
        if( d.y != 0 ) {
            adjacent_closer_squares.push_back( from + point( 0, sgn( d.y ) ) );
        }
    } else if( a.x < a.y ) {
        // Y dominant.
        adjacent_closer_squares.push_back( from + point( 0, sgn( d.y ) ) );
        adjacent_closer_squares.push_back( from + point( 1, sgn( d.y ) ) );
        adjacent_closer_squares.push_back( from + point( -1, sgn( d.y ) ) );
        if( d.x != 0 ) {
            adjacent_closer_squares.push_back( from + point( sgn( d.x ), 0 ) );
        }
    } else if( d.x != 0 ) {
        // Pure diagonal.
        adjacent_closer_squares.push_back( from + point( sgn( d.x ), sgn( d.y ) ) );
        adjacent_closer_squares.push_back( from + point( sgn( d.x ), 0 ) );
        adjacent_closer_squares.push_back( from + point( 0, sgn( d.y ) ) );
    }

    return adjacent_closer_squares;
}

// Returns a vector of the adjacent square in the direction of the target,
// and the two squares flanking it.
std::vector<point> squares_in_direction( const point &p1, const point &p2 )
{
    point center_square = line_to_2( p1, p2 )[0];
    std::vector<point> adjacent_squares;
    adjacent_squares.reserve( 3 );
    adjacent_squares.push_back( center_square );
    if( p1.x == center_square.x ) {
        // Horizontally adjacent.
        adjacent_squares.emplace_back( p1.x + 1, center_square.y );
        adjacent_squares.emplace_back( p1.x - 1, center_square.y );
    } else if( p1.y == center_square.y ) {
        // Vertically adjacent.
        adjacent_squares.emplace_back( center_square.x, p1.y + 1 );
        adjacent_squares.emplace_back( center_square.x, p1.y - 1 );
    } else {
        // Diagonally adjacent.
        adjacent_squares.emplace_back( p1.x, center_square.y );
        adjacent_squares.emplace_back( center_square.x, p1.y );
    }
    return adjacent_squares;
}

rl_vec2d rl_vec3d::xy() const
{
    return rl_vec2d( x, y );
}

float rl_vec2d::magnitude() const
{
    return std::sqrt( x * x + y * y );
}

float rl_vec3d::magnitude() const
{
    return std::sqrt( x * x + y * y + z * z );
}

rl_vec2d rl_vec2d::normalized() const
{
    rl_vec2d ret;
    if( is_null() ) { // shouldn't happen?
        ret.x = ret.y = 1;
        return ret;
    }
    const float m = magnitude();
    ret.x = x / m;
    ret.y = y / m;
    return ret;
}

rl_vec3d rl_vec3d::normalized() const
{
    rl_vec3d ret;
    if( is_null() ) { // shouldn't happen?
        ret.x = ret.y = ret.z = 0;
        return ret;
    }
    const float m = magnitude();
    ret.x = x / m;
    ret.y = y / m;
    ret.z = z / m;
    return ret;
}

rl_vec2d rl_vec2d::rotated( float angle ) const
{
    return rl_vec2d(
               x * std::cos( angle ) - y * std::sin( angle ),
               x * std::sin( angle ) + y * std::cos( angle )
           );
}

rl_vec3d rl_vec3d::rotated( float angle ) const
{
    return rl_vec3d(
               x * std::cos( angle ) - y * std::sin( angle ),
               x * std::sin( angle ) + y * std::cos( angle )
           );
}

float rl_vec2d::dot_product( const rl_vec2d &v ) const
{
    return x * v.x + y * v.y;
}

float rl_vec3d::dot_product( const rl_vec3d &v ) const
{
    return x * v.x + y * v.y + y * v.z;
}

bool rl_vec2d::is_null() const
{
    return !( x || y );
}

point rl_vec2d::as_point() const
{
    return point(
               std::round( x ),
               std::round( y )
           );
}

bool rl_vec3d::is_null() const
{
    return !( x || y || z );
}

tripoint rl_vec3d::as_point() const
{
    return tripoint(
               std::round( x ),
               std::round( y ),
               std::round( z )
           );
}

// scale.
rl_vec2d rl_vec2d::operator*( const float rhs ) const
{
    rl_vec2d ret;
    ret.x = x * rhs;
    ret.y = y * rhs;
    return ret;
}

rl_vec3d &rl_vec3d::operator*=( const float rhs )
{
    x *= rhs;
    y *= rhs;
    z *= rhs;
    return *this;
}

rl_vec3d rl_vec3d::operator*( const float rhs ) const
{
    return rl_vec3d( *this ) *= rhs;
}

// subtract
rl_vec2d rl_vec2d::operator-( const rl_vec2d &rhs ) const
{
    rl_vec2d ret;
    ret.x = x - rhs.x;
    ret.y = y - rhs.y;
    return ret;
}

rl_vec3d rl_vec3d::operator-( const rl_vec3d &rhs ) const
{
    rl_vec3d ret;
    ret.x = x - rhs.x;
    ret.y = y - rhs.y;
    ret.z = z - rhs.z;
    return ret;
}

// unary negation
rl_vec2d rl_vec2d::operator-() const
{
    rl_vec2d ret;
    ret.x = -x;
    ret.y = -y;
    return ret;
}

rl_vec3d rl_vec3d::operator-() const
{
    rl_vec3d ret;
    ret.x = -x;
    ret.y = -y;
    ret.z = -z;
    return ret;
}

rl_vec2d rl_vec2d::operator+( const rl_vec2d &rhs ) const
{
    rl_vec2d ret;
    ret.x = x + rhs.x;
    ret.y = y + rhs.y;
    return ret;
}

rl_vec3d rl_vec3d::operator+( const rl_vec3d &rhs ) const
{
    rl_vec3d ret;
    ret.x = x + rhs.x;
    ret.y = y + rhs.y;
    ret.z = z + rhs.z;
    return ret;
}

rl_vec2d rl_vec2d::operator/( const float rhs ) const
{
    rl_vec2d ret;
    ret.x = x / rhs;
    ret.y = y / rhs;
    return ret;
}

rl_vec3d &rl_vec3d::operator/=( const float rhs )
{
    x /= rhs;
    y /= rhs;
    z /= rhs;
    return *this;
}

rl_vec3d rl_vec3d::operator/( const float rhs ) const
{
    return rl_vec3d( *this ) /= rhs;
}

void calc_ray_end( units::angle angle, const int range, const tripoint &p, tripoint &out )
{
    // forces input angle to be between 0 and 360, calculated from actual input
    angle = fmod( angle, 360_degrees );
    if( angle < 0_degrees ) {
        angle += 360_degrees;
    }
    out.z = p.z;
    if( trigdist ) {
        out.x = p.x + range * cos( angle );
        out.y = p.y + range * sin( angle );
    } else {
        int mult = 0;
        if( angle >= 135_degrees && angle <= 315_degrees ) {
            mult = -1;
        } else {
            mult = 1;
        }

        if( angle <= 45_degrees || ( 135_degrees <= angle && angle <= 215_degrees ) ||
            315_degrees < angle ) {
            out.x = p.x + range * mult;
            out.y = p.y + range * tan( angle ) * mult;
        } else {
            out.x = p.x + range * 1 / tan( angle ) * mult;
            out.y = p.y + range * mult;
        }
    }
}

units::angle coord_to_angle( const tripoint &a, const tripoint &b )
{
    units::angle rad = units::atan2( b.y - a.y, b.x - a.x );
    if( rad < 0_degrees ) {
        rad += 2_pi_radians;
    }
    return rad;
}
