#include <algorithm>
#include <cstdlib>
#include <functional>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "ballistics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "cata_catch.h"
#include "creature.h"
#include "dispersion.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "ret_val.h"
#include "test_statistics.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

using firing_statistics = statistics<bool>;

class Threshold
{
    public:
        Threshold( const double accuracy, const double chance )
            : _accuracy( accuracy ), _chance( chance ) {
        }
        double accuracy() const {
            return _accuracy;
        }
        double chance() const {
            return _chance;
        }
    private:
        double _accuracy;
        double _chance;
};

template < class T >
std::ostream &operator <<( std::ostream &os, const std::vector<T> &v )
{
    os << "[";
    for( typename std::vector<T>::const_iterator ii = v.begin(); ii != v.end(); ++ii ) {
        os << " " << *ii;
    }
    os << " ]";
    return os;
}

std::ostream &operator<<( std::ostream &stream, const dispersion_sources &sources )
{
    if( !sources.normal_sources.empty() ) {
        stream << "Normal: " << sources.normal_sources << std::endl;
    }
    if( !sources.linear_sources.empty() ) {
        stream << "Linear: " << sources.linear_sources << std::endl;
    }
    if( !sources.multipliers.empty() ) {
        stream << "Mult: " << sources.multipliers << std::endl;
    }
    return stream;
}

static firing_statistics firing_test( const dispersion_sources &dispersion,
                                      const int range, const Threshold &threshold )
{
    firing_statistics firing_stats( Z99_99 );
    bool threshold_within_confidence_interval = false;
    do {
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        dispersion_sources local_dispersion = dispersion;
        // On each trip through the loop, grab a sample attack roll and add its results to
        // the stat object.  Keep sampling until our calculated confidence interval doesn't overlap
        // any thresholds we care about.  This is a mechanism to limit the number of samples
        // we have to accumulate before we declare that the true average is
        // either above or below the threshold.
        const projectile_attack_aim aim = projectile_attack_roll( local_dispersion, range, 0.5 );
        threshold_within_confidence_interval = false;
        firing_stats.add( aim.missed_by < threshold.accuracy() );
        if( firing_stats.n() < 100 ) {
            threshold_within_confidence_interval = true;
        }
        const double error = firing_stats.margin_of_error();
        const double avg = firing_stats.avg();
        if( avg + error > threshold.chance() && avg - error < threshold.chance() ) {
            threshold_within_confidence_interval = true;
        }
    } while( threshold_within_confidence_interval && firing_stats.n() < 10000000 );
    return firing_stats;
}

static std::vector<firing_statistics> firing_test( const dispersion_sources &dispersion,
        const int range, const std::vector< Threshold > &thresholds )
{
    std::vector<firing_statistics> firing_stats;
    firing_stats.reserve( thresholds.size() );
    for( const Threshold &pear : thresholds ) {
        firing_stats.push_back( firing_test( dispersion, range, pear ) );
    }
    return firing_stats;
}

static dispersion_sources get_dispersion( npc &shooter, const int aim_time )
{
    item &gun = shooter.get_wielded_item();
    dispersion_sources dispersion = shooter.get_weapon_dispersion( gun );

    shooter.moves = aim_time;
    shooter.recoil = MAX_RECOIL;
    // Aim as well as possible within the provided time.
    shooter.aim();
    if( aim_time > 0 ) {
        REQUIRE( shooter.recoil < MAX_RECOIL );
    }
    dispersion.add_range( shooter.recoil );

    return dispersion;
}

static void test_shooting_scenario( npc &shooter, const int min_quickdraw_range,
                                    const int min_good_range, const int max_good_range )
{
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 0 );
        std::vector<firing_statistics> minimum_stats = firing_test( dispersion, min_quickdraw_range, {
            Threshold( accuracy_grazing, 0.2 ),
            Threshold( accuracy_standard, 0.1 )
        } );
        INFO( dispersion );
        INFO( "Range: " << min_quickdraw_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), shooter.recoil ) );
        CAPTURE( shooter.ranged_dispersion_modifier_hands() );
        CAPTURE( minimum_stats[0].n() );
        CAPTURE( minimum_stats[0].margin_of_error() );
        CAPTURE( minimum_stats[1].n() );
        CAPTURE( minimum_stats[1].margin_of_error() );
        CHECK( minimum_stats[0].avg() < 0.2 );
        CHECK( minimum_stats[1].avg() < 0.1 );
    }
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 300 );
        firing_statistics good_stats = firing_test( dispersion, min_good_range, Threshold( accuracy_goodhit,
                                       0.5 ) );
        INFO( dispersion );
        INFO( "Range: " << min_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), shooter.recoil ) );
        CAPTURE( shooter.ranged_dispersion_modifier_hands() );
        CAPTURE( good_stats.n() );
        CAPTURE( good_stats.margin_of_error() );
        CHECK( good_stats.avg() > 0.5 );
    }
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 500 );
        firing_statistics good_stats = firing_test( dispersion, max_good_range, Threshold( accuracy_goodhit,
                                       0.1 ) );
        INFO( dispersion );
        INFO( "Range: " << max_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), shooter.recoil ) );
        CAPTURE( shooter.ranged_dispersion_modifier_hands() );
        CAPTURE( good_stats.n() );
        CAPTURE( good_stats.margin_of_error() );
        CHECK( good_stats.avg() < 0.1 );
    }
}

static void test_fast_shooting( npc &shooter, const int moves, float hit_rate )
{
    // When enermy is too close it is better to just draw the weapon and shoot.
    // The character will use 'rough aiming' and how good the weapon and shooter is not so important.
    // So fast_shooting_range is change from 3 to 5.
    const int fast_shooting_range = 5;
    const float hit_rate_cap = hit_rate + 0.3f;
    const dispersion_sources dispersion = get_dispersion( shooter, moves );
    firing_statistics fast_stats = firing_test( dispersion, fast_shooting_range,
                                   Threshold( accuracy_standard, hit_rate ) );
    firing_statistics fast_stats_upper = firing_test( dispersion, fast_shooting_range,
                                         Threshold( accuracy_standard, hit_rate_cap ) );
    INFO( dispersion );
    INFO( "Range: " << fast_shooting_range );
    INFO( "Max aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), MAX_RECOIL ) );
    INFO( "Min aim speed: " << shooter.aim_per_move( shooter.get_wielded_item(), shooter.recoil ) );
    CAPTURE( shooter.ranged_dispersion_modifier_hands() );
    CAPTURE( shooter.get_wielded_item().gun_skill().str() );
    CAPTURE( shooter.get_skill_level( shooter.get_wielded_item().gun_skill() ) );
    CAPTURE( shooter.get_dex() );
    CAPTURE( to_milliliter( shooter.get_wielded_item().volume() ) );
    CAPTURE( fast_stats.n() );
    CAPTURE( fast_stats.margin_of_error() );
    CHECK( fast_stats.avg() > hit_rate );
    CAPTURE( fast_stats_upper.n() );
    CAPTURE( fast_stats_upper.margin_of_error() );
    CHECK( fast_stats_upper.avg() < hit_rate_cap );
}

static void assert_encumbrance( npc &shooter, int encumbrance )
{
    for( const bodypart_id &bp : shooter.get_all_body_parts() ) {
        INFO( "Body Part: " << body_part_name( bp ) );
        REQUIRE( shooter.encumb( bp ) == encumbrance );
    }
}

static constexpr tripoint shooter_pos( 60, 60, 0 );

// To simulate players in the early game
TEST_CASE( "unskilled_shooter_accuracy", "[ranged] [balance] [slow]" )
{
    clear_map();
    standard_npc shooter( "Shooter", shooter_pos, {}, 0, 8, 8, 8, 8 );
    shooter.set_body();
    shooter.worn.emplace_back( "backpack" );
    equip_shooter( shooter, { "bastsandals", "armguard_hard", "armguard_soft", "armor_chitin", "beekeeping_gloves", "mask_guy_fawkes", "cowboy_hat" } );
    assert_encumbrance( shooter, 10 );

    SECTION( "an unskilled shooter with a common pistol" ) {
        arm_shooter( shooter, "glock_19" );
        test_shooting_scenario( shooter, 4, 5, 15 );
        test_fast_shooting( shooter, 50, 0.2 );
    }
    SECTION( "an unskilled archer with a common bow" ) {
        arm_shooter( shooter, "shortbow", { "bow_sight_pin" }, "arrow_field_point_fletched" );
        test_shooting_scenario( shooter, 4, 3, 12 );
        test_fast_shooting( shooter, 120, 0.2 );
    }
    SECTION( "an unskilled archer with a common crossbow" ) {
        arm_shooter( shooter, "crossbow", {}, "bolt_makeshift" );
        test_shooting_scenario( shooter, 4, 5, 18 );
        test_fast_shooting( shooter, 90, 0.2 );
    }
    SECTION( "an unskilled shooter with a common shotgun" ) {
        arm_shooter( shooter, "remington_870" );
        test_shooting_scenario( shooter, 4, 5, 18 );
        test_fast_shooting( shooter, 75, 0.2 );
    }
    SECTION( "an unskilled shooter with a common smg" ) {
        arm_shooter( shooter, "mp40semi" );
        test_shooting_scenario( shooter, 4, 5, 18 );
        test_fast_shooting( shooter, 75, 0.2 );
    }
    SECTION( "an unskilled shooter with a common rifle" ) {
        arm_shooter( shooter, "ar15" );
        test_shooting_scenario( shooter, 4, 5, 25 );
        test_fast_shooting( shooter, 90, 0.2 );
    }
}

// To simulate well-equipped players in mid game
TEST_CASE( "competent_shooter_accuracy", "[ranged] [balance]" )
{
    clear_map();
    standard_npc shooter( "Shooter", shooter_pos, {}, 5, 10, 10, 10, 10 );
    shooter.set_body();
    equip_shooter( shooter, { "cloak_wool", "footrags_wool", "gloves_wraps_fur", "glasses_safety", "balclava" } );
    assert_encumbrance( shooter, 5 );

    SECTION( "a skilled shooter with an accurate pistol" ) {
        arm_shooter( shooter, "sw_619", { "red_dot_sight" } );
        test_shooting_scenario( shooter, 10, 12, 33 );
        test_fast_shooting( shooter, 50, 0.35 );
    }
    SECTION( "a skilled archer with an accurate bow" ) {
        arm_shooter( shooter, "recurbow", { "bow_sight" } );
        test_shooting_scenario( shooter, 8, 10, 32 );
        test_fast_shooting( shooter, 90, 0.35 );
    }
    SECTION( "a skilled archer with an accurate crossbow" ) {
        arm_shooter( shooter, "compositecrossbow", { "acog_scope" }, "bolt_steel" );
        test_shooting_scenario( shooter, 9, 10, 33 );
        test_fast_shooting( shooter, 90, 0.35 );
    }
    SECTION( "a skilled shooter with a shotgun with red dot sight" ) {
        arm_shooter( shooter, "mossberg_930", { "red_dot_sight" } );
        test_shooting_scenario( shooter, 9, 12, 33 );
        test_fast_shooting( shooter, 75, 0.35 );
    }
    SECTION( "a skilled shooter with a smg equipped with red dot sight" ) {
        arm_shooter( shooter, "hk_mp5", { "red_dot_sight" } );
        test_shooting_scenario( shooter, 12, 12, 40 );
        test_fast_shooting( shooter, 75, 0.35 );
    }
    SECTION( "a skilled shooter with a carbine equipped with red dot sight" ) {
        arm_shooter( shooter, "m4_carbine", { "red_dot_sight" }, "556_m855a1" );
        test_shooting_scenario( shooter, 10, 12, 48 );
        test_fast_shooting( shooter, 75, 0.35 );
    }
    SECTION( "a skilled shooter with a snipper rifle" ) {
        arm_shooter( shooter, "m110a1" );
        test_shooting_scenario( shooter, 10, 12, 70 );
        test_fast_shooting( shooter, 90, 0.35 );
    }
}

// To simulate extreme case
TEST_CASE( "expert_shooter_accuracy", "[ranged] [balance]" )
{
    clear_map();
    standard_npc shooter( "Shooter", shooter_pos, {}, 10, 20, 20, 20, 20 );
    shooter.set_body();
    equip_shooter( shooter, { } );
    assert_encumbrance( shooter, 0 );

    SECTION( "an expert shooter with an excellent pistol" ) {
        arm_shooter( shooter, "sw629", { "pistol_scope" } );
        test_shooting_scenario( shooter, 18, 16, 140 );
        test_fast_shooting( shooter, 50, 0.5 );
    }
    SECTION( "an expert archer with an excellent bow" ) {
        arm_shooter( shooter, "compbow_high", { "bow_scope" }, "arrow_cf" );
        test_shooting_scenario( shooter, 12, 20, 80 );
        test_fast_shooting( shooter, 50, 0.5 );
    }
    SECTION( "an expert archer with an excellent crossbow" ) {
        arm_shooter( shooter, "compcrossbow", { "holo_sight" }, "bolt_cf" );
        test_shooting_scenario( shooter, 12, 20, 100 );
        test_fast_shooting( shooter, 70, 0.5 );
    }
    SECTION( "an expert shooter with an excellent shotgun" ) {
        arm_shooter( shooter, "mossberg_930", { "holo_sight" } );
        test_shooting_scenario( shooter, 18, 24, 124 );
        test_fast_shooting( shooter, 75, 0.5 );
    }
    SECTION( "an expert shooter with an excellent smg" ) {
        arm_shooter( shooter, "ppsh", { "holo_sight" } );
        test_shooting_scenario( shooter, 20, 30, 190 );
        test_fast_shooting( shooter, 75, 0.5 );
    }
    SECTION( "an expert shooter with an with a carbine equipped with holographic sight" ) {
        arm_shooter( shooter, "m4_carbine", { "holo_sight" } );
        test_shooting_scenario( shooter, 20, 36, 900 );
        test_fast_shooting( shooter, 75, 0.5 );
    }
    SECTION( "an expert shooter with an excellent snipper rifle" ) {
        arm_shooter( shooter, "browning_blr", { "rifle_scope" } );
        test_shooting_scenario( shooter, 25, 80, 900 );
        test_fast_shooting( shooter, 90, 0.5 );
    }
}

static void range_test( const Threshold &test_threshold, bool write_data = false )
{
    std::vector <int> data;
    // Start at an absurdly high dispersion and count down.
    int prev_dispersion = 6000;
    for( int r = 1; r <= 60; ++r ) {
        int found_dispersion = -1;
        // We carry forward prev_dispersion because we never expet the next tier of range to hit the target accuracy level with a lower dispersion.
        for( int d = prev_dispersion; d >= 0; --d ) {
            firing_statistics stats = firing_test( dispersion_sources( d ), r, test_threshold );
            // Switch this from INFO to WARN to debug the scanning process itself.
            INFO( "Samples: " << stats.n() << " Range: " << r << " Dispersion: " << d <<
                  " avg hit rate: " << stats.avg() );
            if( stats.avg() > test_threshold.chance() ) {
                found_dispersion = d;
                prev_dispersion = d;
                break;
            }
            // The intent here is to skip over dispersion values proportionally to how far from converging we are.
            // As long as we check several adjacent dispersion values before a hit, we're good.
            d -= static_cast<int>( ( 1 - ( stats.avg() / test_threshold.chance() ) ) * 15 ) * 5;
        }
        if( found_dispersion == -1 ) {
            WARN( "No matching dispersion found" );
        } else {
            WARN( "Range: " << r << " Dispersion: " << found_dispersion );
            data.push_back( found_dispersion );
        }
    }
    if( write_data ) {
        const bool similar_to_previous_test_results =
            std::equal( data.begin(), data.end(),
                        Creature::dispersion_for_even_chance_of_good_hit.begin(),
                        Creature::dispersion_for_even_chance_of_good_hit.end(),
        []( const int a, const int b ) -> bool {
            return a > 0 && b > 0 && std::abs( static_cast<float>( a - b ) / b ) < 0.15;
        } );

        if( !similar_to_previous_test_results ) {
            write_to_file( "./data/json/hit_range.json", [&]( std::ostream & fsa ) {
                JsonOut j_out( fsa );
                j_out.start_array();
                j_out.start_object();
                j_out.member( "type", "hit_range" );
                j_out.member( "even_good", data );
                j_out.end_object();
                j_out.end_array();
            }, _( "hit_range file" ) );
        } else {
            WARN( "Didn't write.  Data too similar to previous test results." );
        }
        REQUIRE( similar_to_previous_test_results );
    }
}

// I added this to find inflection points where accuracy at a particular range crosses a threshold.
// I don't see any assertions we can make about these thresholds offhand.
TEST_CASE( "synthetic_range_test", "[.]" )
{
    SECTION( "quickdraw thresholds" ) {
        range_test( Threshold( accuracy_grazing, 0.1 ) );
    }
    SECTION( "max range thresholds" ) {
        range_test( Threshold( accuracy_goodhit, 0.1 ) );
    }
    SECTION( "good hit thresholds" ) {
        range_test( Threshold( accuracy_goodhit, 0.5 ), true );
    }
}

static void shoot_monster( std::string ammo_type, int range, int expected_damage_min,
                           int expected_damage_max, std::string monster_type )
{
    const tripoint shooter_pos( 60, 60, 0 );
    const tripoint monster_pos = shooter_pos + ( point_east * range );
    standard_npc shooter( "Shooter", shooter_pos, {}, 5, 10, 10, 10, 10 );
    shooter.set_body();
    arm_shooter( shooter, "shotgun_s", {}, ammo_type );
    shooter.recoil = 0;
    monster &mon = spawn_test_monster( monster_type, monster_pos );
    int prev_HP = mon.get_hp();
    REQUIRE( shooter.fire_gun( monster_pos, 1, shooter.get_wielded_item() ) == 1 );
    int actual_damage = prev_HP - mon.get_hp();
    CAPTURE( ammo_type );
    CAPTURE( range );
    CAPTURE( monster_type );
    CHECK( actual_damage >= expected_damage_min );
    CHECK( actual_damage <= expected_damage_max );
    clear_creatures();
}

TEST_CASE( "shot_features", "[gun]" )
{
    clear_map();
    // BIRDSHOT
    // Unarmored target
    // Minor damage at range.
    // shoot_monster( "shot_bird", 10, 10, 15, "mon_zombie" );
    // More serious damage at close range.
    shoot_monster( "shot_bird", 5, 20, 30, "mon_wolf_mutant_huge" );
    // Grevious damage at point blank.
    shoot_monster( "shot_bird", 1, 50, 100, "mon_wolf_mutant_huge" );

    // Triviallly armored target (armor_bullet: 1)
    // Can rarely if ever inflict damage at range.
    // shoot_monster( "shot_bird", 10, 0, 5, "mon_zombie_tough" );
    // Can barely hurt at close range.
    shoot_monster( "shot_bird", 5, 0, 10, "mon_zombie_tough" );
    // Can seriously injure trivially armored enemy at point blank,
    shoot_monster( "shot_bird", 1, 45, 75, "mon_zombie_tough" );

    // Armored target (armor_bullet: 5)
    // Can't hurt at range,
    // shoot_monster( "shot_bird", 10, 0, 5, "mon_zombie_brute" );
    // Can't hurt at close range.
    shoot_monster( "shot_bird", 5, 0, 10, "mon_zombie_brute" );
    // Serioualy injure at point blank.
    shoot_monster( "shot_bird", 1, 40, 75, "mon_zombie_brute" );
    // TODO: can't harm heavily armored enemies at point blank

    // Heavily Armored target (armor_bullet: 36)
    // Can't hurt at range,
    shoot_monster( "shot_bird", 10, 0, 7, "mon_skeleton_hulk" );
    // Can't hurt at close range.
    shoot_monster( "shot_bird", 5, 0, 7, "mon_skeleton_hulk" );
    // Barely injure at point blank.
    shoot_monster( "shot_bird", 1, 10, 75, "mon_skeleton_hulk" );
    // TODO: can't harm heavily armored enemies even at point blank.

    // BUCKSHOT
    // Unarmored target
    // Heavy damage at range.
    shoot_monster( "shot_00", 10, 135, 195, "mon_wolf_mutant_huge" );
    // More damage at close range.
    shoot_monster( "shot_00", 5, 135, 195, "mon_wolf_mutant_huge" );
    // Extreme damage at point blank range.
    shoot_monster( "shot_00", 1, 50, 90, "mon_wolf_mutant_huge" );

    // Lightly armored target (armor_bullet: 5)
    // Outcomes for lightly armored enemies are very similar.
    shoot_monster( "shot_00", 10, 90, 130, "mon_boomer_glutton" );
    shoot_monster( "shot_00", 5, 90, 130, "mon_boomer_glutton" );
    shoot_monster( "shot_00", 1, 45, 85, "mon_boomer_glutton" );

    // Armored target (armor_bullet: 10)
    shoot_monster( "shot_00", 10, 45, 65, "mon_hulk_pupa_decoy" );
    shoot_monster( "shot_00", 5, 45, 65, "mon_hulk_pupa_decoy" );
    shoot_monster( "shot_00", 1, 40, 75, "mon_hulk_pupa_decoy" );
}
