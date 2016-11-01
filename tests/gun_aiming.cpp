#include "catch/catch.hpp"

#include "game.h"
#include "npc.h"
#include "item_factory.h"

static void test_distribution( const npc &who, int dispersion, int range )
{
    const int N = 50000;
    std::array< std::pair<double, int>, 20 > bins;

    for( int i = 0; i < bins.size(); ++i ) {
        bins[i].first = ( double )( bins.size() - i ) / bins.size();
        bins[i].second = 0;
    }

    for( int i = 0; i < N; ++i ) {
        projectile_attack_aim aim = who.projectile_attack_roll( dispersion, range );
        for( int j = 0; j < bins.size() && aim.missed_by < bins[j].first; ++j ) {
            ++bins[j].second;
        }
    }

    for( int i = 0; i < bins.size(); ++i ) {
        CAPTURE( range );
        CAPTURE( dispersion );
        CAPTURE( bins[i].first );
        REQUIRE( who.projectile_attack_chance( dispersion, range, bins[i].first ) == Approx( ( double )bins[i].second / N ).epsilon( 0.01 ) );
    }
}

static void test_internal( const npc& who, const item &gun )
{
    THEN( "the computed range from accuracy, recoil, and chance is correctly calculated" ) {
        for ( double accuracy = 0.1; accuracy <= 1.0; accuracy += 0.1 ) {
            for ( int chance = 1; chance < 100; ++chance ) {
                for ( double recoil = 0; recoil < 1000; ++recoil ) {
                    double range = who.gun_current_range( gun, recoil, chance, accuracy );
                    double dispersion = who.get_weapon_dispersion( gun ) + recoil;

                    CAPTURE( accuracy );
                    CAPTURE( chance );
                    CAPTURE( recoil );
                    CAPTURE( range );
                    CAPTURE( dispersion );

                    if ( range == gun.gun_range( &who ) ) {
                        REQUIRE( who.projectile_attack_chance( dispersion, range, accuracy ) >= chance / 100.0 );
                    } else {
                        REQUIRE( who.projectile_attack_chance( dispersion, range, accuracy ) == Approx( chance / 100.0 ).epsilon( 0.0005 ) );
                    }
                }
            }
        }
    }

    THEN( "the snapshot range is less than the effective range" ) {
        REQUIRE( who.gun_engagement_range( gun, player::engagement::snapshot ) <=
                 who.gun_engagement_range( gun, player::engagement::effective ) );
    }

    THEN( "the effective range is less than maximum range" ) {
        REQUIRE( who.gun_engagement_range( gun, player::engagement::effective ) <=
                 who.gun_engagement_range( gun, player::engagement::maximum ) );
    }

    WHEN( "the gun it is aimed" ) {
        double penalty = MIN_RECOIL;
        double aimed = penalty - who.aim_per_move( gun, penalty );

        THEN( "recoil is the the same or less" ) {
            REQUIRE( aimed <= penalty );

            AND_THEN( "the effective range is the same or better" ) {
                REQUIRE( who.gun_current_range( gun, penalty ) <=
                         who.gun_current_range( gun, aimed ) );
            }
        }
    }

    WHEN( "a higher accuracy is requested" ) {
        THEN( "the effective range is worse" ) {
            REQUIRE( who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_grazing ) >
                     who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_critical  ) );
        }
    }

    WHEN( "a higher certainty is requested" ) {
        THEN( "the effective range is worse" ) {
            REQUIRE( who.gun_current_range( gun, MIN_RECOIL, 50 ) >
                     who.gun_current_range( gun, MIN_RECOIL, 80 ) );
        }
    }
}

TEST_CASE( "gun_aiming", "[gun] [aim]" ) {
    const int gun_skill    = 4; // marksmanship
    const int weapon_skill = 4; // relevant weapon (eg. rifle)

    // Note that GIVEN statements cannot appear within loops
    GIVEN( "A typical survivor with a loaded gun" ) {

        standard_npc who;
        who.setpos( tripoint( 0, 0, 0 ) );
        who.wear_item( item( "gloves_lsurvivor" ) );
        who.wear_item( item( "mask_lsurvivor" ) );
        who.set_skill_level( skill_id( "gun" ), gun_skill );

        WHEN( "many shots are fired" ) {
            THEN( "the distribution of accuracies is as expected" ) {
                for( int range = 0; range <= MAX_RANGE; ++range ) {
                    for( int dispersion = 0; dispersion < 1200; dispersion += 50 ) {
                        test_distribution( who, dispersion, range );
                    }
                }
            }
        }

        WHEN( "the gun ranges are examined" ) {
            for( const auto& e : item_controller->get_all_itypes() ) {
                if( e.second.gun ) {
                    item gun( e.first );
                    if( !gun.magazine_integral() ) {
                        gun.emplace_back( gun.magazine_default() );
                    }
                    gun.ammo_set( default_ammo( gun.ammo_type() ), gun.ammo_capacity() );

                    who.set_skill_level( gun.gun_skill(), weapon_skill );

                    INFO( "GUN: " << gun.tname() );
                    INFO( "AMMO " << gun.ammo_current() );

                    REQUIRE( gun.is_gun() );
                    REQUIRE( gun.ammo_sufficient() );

                    test_internal( who, gun );

                    // @todo acceptance tests here
                }
            }
        }

    }
}
