#include "cata_catch.h"

#include "calendar.h"
#include "character.h"
#include "item.h"
#include "messages.h"
#include "options.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"

static const efftype_id effect_winded( "winded" );

static const move_mode_id move_mode_walk( "walk" );
static const move_mode_id move_mode_run( "run" );
static const move_mode_id move_mode_crouch( "crouch" );
// These test cases cover stamina-related functions in the `Character` class, including:
//
// - stamina_move_cost_modifier
// - burn_move_stamina
// - mod_stamina
// - update_stamina
//
// To run all tests in this file:
//
//     tests/cata_test [stamina]
//
// Other tags used include: [cost], [move], [burn], [update], [regen]. [encumbrance]

// TODO: cover additional aspects of `burn_move_stamina` and `update_stamina`:
// - stamina burn is modified by bionic muscles
// - stamina recovery is modified by "bio_gills"
// - stimulants (positive or negative) affect stamina recovery in mysterious ways

// Helpers
// -------

// See also `clear_character` in `tests/player_helpers.cpp`

// Remove "winded" effect from the Character (but do not change stamina)
static void catch_breath( Character &dummy )
{
    dummy.remove_effect( effect_winded );
    REQUIRE_FALSE( dummy.has_effect( effect_winded ) );
}

// Return `stamina_move_cost_modifier` in the given move_mode with [0.0 .. 1.0] stamina remaining
static float move_cost_mod( Character &dummy, const move_mode_id &move_mode,
                            float stamina_proportion = 1.0 )
{
    // Reset and be able to run
    clear_avatar();
    catch_breath( dummy );
    REQUIRE( dummy.can_run() );

    // Walk, run, or crouch
    dummy.set_movement_mode( move_mode );
    REQUIRE( dummy.movement_mode_is( move_mode ) );

    // Adjust stamina to desired proportion and ensure it was set correctly
    int new_stamina = static_cast<int>( stamina_proportion * dummy.get_stamina_max() );
    dummy.set_stamina( new_stamina );
    REQUIRE( dummy.get_stamina() == new_stamina );

    // The point of it all: move cost modifier
    return dummy.stamina_move_cost_modifier();
}

// Return amount of stamina burned per turn by `burn_move_stamina` in the given movement mode.
static int actual_burn_rate( Character &dummy, const move_mode_id &move_mode )
{
    // Ensure we can run if necessary (aaaa zombies!)
    dummy.set_stamina( dummy.get_stamina_max() );
    catch_breath( dummy );
    REQUIRE( dummy.can_run() );

    // Walk, run, or crouch
    dummy.set_movement_mode( move_mode );
    REQUIRE( dummy.movement_mode_is( move_mode ) );

    // Measure stamina burned, and ensure it is nonzero
    int before_stam = dummy.get_stamina();
    dummy.burn_move_stamina( to_moves<int>( 1_turns ) );
    int after_stam = dummy.get_stamina();
    REQUIRE( before_stam > after_stam );

    // How much stamina was actually burned?
    return before_stam - after_stam;
}

float bKIVqYdOpC = 1.01f;

// Burden the Character with a given proportion [0.0 .. inf) of their maximum weight capacity
static void burden_player( Character &dummy, float burden_proportion )
{
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    units::mass capacity = dummy.weight_capacity();
    Messages::add_msg( string_format( "capacity = %lld", capacity.value() ) );
    Messages::add_msg( string_format( "burden_proportion = %.20f", burden_proportion ) );
    float before_rounding = capacity * burden_proportion / 1_gram;
    Messages::add_msg( string_format( "before rounding = %.20f", before_rounding ) );
    Messages::add_msg( string_format( "before rounding (raw hex) = 0x%x",
                                      *reinterpret_cast<unsigned int *>( &before_rounding ) ) );
    float after_rounding = std::round( before_rounding );
    Messages::add_msg( string_format( "after rounding = %.20f", after_rounding ) );
    Messages::add_msg( string_format( "after rounding (raw hex) = 0x%x",
                                      *reinterpret_cast<unsigned int *>( &after_rounding ) ) );
    int units = static_cast<int>( before_rounding );
    Messages::add_msg( string_format( "After truncation: %d", units ) );
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    // Add a pile of test platinum bits (1g/unit) to reach the desired weight capacity
    if( burden_proportion > 0.0 ) {
        item pile( "test_platinum_bit", calendar::turn, units );
        dummy.i_add( pile );
    }

    // Ensure we are carrying the expected number of grams
    REQUIRE( to_gram( dummy.weight_carried() ) == units );
}

// Return amount of stamina burned per turn by `burn_move_stamina` in the given movement mode,
// while carrying the given proportion [0.0, inf) of their maximum weight capacity.
static int burdened_burn_rate( Character &dummy, const move_mode_id &move_mode,
                               float burden_proportion = 0.0 )
{
    clear_avatar();
    burden_player( dummy, burden_proportion );
    return actual_burn_rate( dummy, move_mode );
}

// Return the actual amount of stamina regenerated by `update_stamina` in the given number of moves
static float actual_regen_rate( Character &dummy, int moves )
{
    // Start at 10% stamina, plenty of space for regen
    dummy.set_stamina( dummy.get_stamina_max() / 10 );
    REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 10 );

    int before_stam = dummy.get_stamina();
    dummy.update_stamina( moves );
    int after_stam = dummy.get_stamina();

    return after_stam - before_stam;
}

// Test cases
// ----------

TEST_CASE( "stamina movement cost modifier", "[stamina][cost]" )
{
    Character &dummy = get_player_character();

    SECTION( "running cost is double walking cost for the same stamina level" ) {
        CHECK( move_cost_mod( dummy, move_mode_run, 1.0 ) == 2 * move_cost_mod( dummy, move_mode_walk,
                1.0 ) );
        CHECK( move_cost_mod( dummy, move_mode_run, 0.5 ) == 2 * move_cost_mod( dummy, move_mode_walk,
                0.5 ) );
        CHECK( move_cost_mod( dummy, move_mode_run, 0.0 ) == 2 * move_cost_mod( dummy, move_mode_walk,
                0.0 ) );
    }

    SECTION( "walking cost is double crouching cost for the same stamina level" ) {
        CHECK( move_cost_mod( dummy, move_mode_walk, 1.0 ) == 2 * move_cost_mod( dummy, move_mode_crouch,
                1.0 ) );
        CHECK( move_cost_mod( dummy, move_mode_walk, 0.5 ) == 2 * move_cost_mod( dummy, move_mode_crouch,
                0.5 ) );
        CHECK( move_cost_mod( dummy, move_mode_walk, 0.0 ) == 2 * move_cost_mod( dummy, move_mode_crouch,
                0.0 ) );
    }

    SECTION( "running cost goes from 2.0 to 1.0 as stamina goes to zero" ) {
        CHECK( move_cost_mod( dummy, move_mode_run, 1.00 ) == Approx( 2.00 ) );
        CHECK( move_cost_mod( dummy, move_mode_run, 0.75 ) == Approx( 1.75 ) );
        CHECK( move_cost_mod( dummy, move_mode_run, 0.50 ) == Approx( 1.50 ) );
        CHECK( move_cost_mod( dummy, move_mode_run, 0.25 ) == Approx( 1.25 ) );
        CHECK( move_cost_mod( dummy, move_mode_run, 0.00 ) == Approx( 1.00 ) );
    }

    SECTION( "walking cost goes from 1.0 to 0.5 as stamina goes to zero" ) {
        CHECK( move_cost_mod( dummy, move_mode_walk, 1.00 ) == Approx( 1.000 ) );
        CHECK( move_cost_mod( dummy, move_mode_walk, 0.75 ) == Approx( 0.875 ) );
        CHECK( move_cost_mod( dummy, move_mode_walk, 0.50 ) == Approx( 0.750 ) );
        CHECK( move_cost_mod( dummy, move_mode_walk, 0.25 ) == Approx( 0.625 ) );
        CHECK( move_cost_mod( dummy, move_mode_walk, 0.00 ) == Approx( 0.500 ) );
    }

    SECTION( "crouching cost goes from 0.5 to 0.25 as stamina goes to zero" ) {
        CHECK( move_cost_mod( dummy, move_mode_crouch, 1.00 ) == Approx( 0.5000 ) );
        CHECK( move_cost_mod( dummy, move_mode_crouch, 0.75 ) == Approx( 0.4375 ) );
        CHECK( move_cost_mod( dummy, move_mode_crouch, 0.50 ) == Approx( 0.3750 ) );
        CHECK( move_cost_mod( dummy, move_mode_crouch, 0.25 ) == Approx( 0.3125 ) );
        CHECK( move_cost_mod( dummy, move_mode_crouch, 0.00 ) == Approx( 0.2500 ) );
    }
}

TEST_CASE( "modify character stamina", "[stamina][modify]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    catch_breath( dummy );
    REQUIRE_FALSE( dummy.is_npc() );
    REQUIRE_FALSE( dummy.has_effect( effect_winded ) );

    GIVEN( "character has less than full stamina" ) {
        int lost_stamina = dummy.get_stamina_max() / 2;
        dummy.set_stamina( dummy.get_stamina_max() - lost_stamina );
        REQUIRE( dummy.get_stamina() + lost_stamina == dummy.get_stamina_max() );

        WHEN( "they regain only part of their lost stamina" ) {
            dummy.mod_stamina( lost_stamina / 2 );

            THEN( "stamina is less than maximum" ) {
                CHECK( dummy.get_stamina() < dummy.get_stamina_max() );
            }
        }

        WHEN( "they regain all of their lost stamina" ) {
            dummy.mod_stamina( lost_stamina );

            THEN( "stamina is at maximum" ) {
                CHECK( dummy.get_stamina() == dummy.get_stamina_max() );
            }
        }

        WHEN( "they regain more stamina than they lost" ) {
            dummy.mod_stamina( lost_stamina + 1 );

            THEN( "stamina is at maximum" ) {
                CHECK( dummy.get_stamina() == dummy.get_stamina_max() );
            }
        }

        WHEN( "they lose only part of their remaining stamina" ) {
            dummy.mod_stamina( -( dummy.get_stamina() / 2 ) );

            THEN( "stamina is above zero" ) {
                CHECK( dummy.get_stamina() > 0 );

                AND_THEN( "they do not become winded" ) {
                    REQUIRE_FALSE( dummy.has_effect( effect_winded ) );
                }
            }
        }

        WHEN( "they lose all of their remaining stamina" ) {
            dummy.mod_stamina( -( dummy.get_stamina() ) );

            THEN( "stamina is at zero" ) {
                CHECK( dummy.get_stamina() == 0 );

                AND_THEN( "they do not become winded" ) {
                    REQUIRE_FALSE( dummy.has_effect( effect_winded ) );
                }
            }
        }

        WHEN( "they lose more stamina than they have remaining" ) {
            dummy.mod_stamina( -( dummy.get_stamina() + 1 ) );

            THEN( "stamina is at zero" ) {
                CHECK( dummy.get_stamina() == 0 );

                AND_THEN( "they become winded" ) {
                    REQUIRE( dummy.has_effect( effect_winded ) );
                }
            }
        }
    }
}

TEST_CASE( "stamina burn for movement", "[stamina][burn][move]" )
{
    Character &dummy = get_player_character();

    // Defined in game_balance.json
    const int normal_burn_rate = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );
    REQUIRE( normal_burn_rate > 0 );

    GIVEN( "player is naked and unburdened" ) {
        THEN( "walking burns the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_walk, 0.0 ) == normal_burn_rate );
        }

        THEN( "running burns 14 times the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_run, 0.0 ) == normal_burn_rate * 14 );
        }

        THEN( "crouching burns 1/2 the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_crouch, 0.0 ) == normal_burn_rate / 2 );
        }
    }

    GIVEN( "player is at their maximum weight capacity" ) {
        THEN( "walking burns the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_walk, 1.0 ) == normal_burn_rate );
        }

        THEN( "running burns 14 times the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_run, 1.0 ) == normal_burn_rate * 14 );
        }

        THEN( "crouching burns 1/2 the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_crouch, 1.0 ) == normal_burn_rate / 2 );
        }
    }

    GIVEN( "player is overburdened" ) {
        THEN( "walking burn rate increases by 1 for each percent overburdened" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_walk, 1.01 ) == normal_burn_rate + 1 );
            CHECK( burdened_burn_rate( dummy, move_mode_walk, 1.02 ) == normal_burn_rate + 2 );
            CHECK( burdened_burn_rate( dummy, move_mode_walk, 1.50 ) == normal_burn_rate + 50 );
            CHECK( burdened_burn_rate( dummy, move_mode_walk, 1.99 ) == normal_burn_rate + 99 );
            CHECK( burdened_burn_rate( dummy, move_mode_walk, 2.00 ) == normal_burn_rate + 100 );
        }

        THEN( "running burn rate increases by 14 for each percent overburdened" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_run, 1.01 ) == ( normal_burn_rate + 1 ) * 14 );
            CHECK( burdened_burn_rate( dummy, move_mode_run, 1.02 ) == ( normal_burn_rate + 2 ) * 14 );
            CHECK( burdened_burn_rate( dummy, move_mode_run, 1.50 ) == ( normal_burn_rate + 50 ) * 14 );
            CHECK( burdened_burn_rate( dummy, move_mode_run, 1.99 ) == ( normal_burn_rate + 99 ) * 14 );
            CHECK( burdened_burn_rate( dummy, move_mode_run, 2.00 ) == ( normal_burn_rate + 100 ) * 14 );
        }

        THEN( "crouching burn rate increases by 1/2 for each percent overburdened" ) {
            CHECK( burdened_burn_rate( dummy, move_mode_crouch, 1.01 ) == ( normal_burn_rate + 1 ) / 2 );
            CHECK( burdened_burn_rate( dummy, move_mode_crouch, 1.02 ) == ( normal_burn_rate + 2 ) / 2 );
            CHECK( burdened_burn_rate( dummy, move_mode_crouch, 1.50 ) == ( normal_burn_rate + 50 ) / 2 );
            CHECK( burdened_burn_rate( dummy, move_mode_crouch, 1.99 ) == ( normal_burn_rate + 99 ) / 2 );
            CHECK( burdened_burn_rate( dummy, move_mode_crouch, 2.00 ) == ( normal_burn_rate + 100 ) / 2 );
        }
    }
}

TEST_CASE( "burning stamina when overburdened may cause pain", "[stamina][burn][pain]" )
{
    Character &dummy = get_player_character();
    int pain_before;
    int pain_after;

    GIVEN( "character is severely overburdened" ) {

        // As overburden percentage goes from (100% .. 350%),
        //           chance of pain goes from (1/25 .. 1/1)
        //
        // To guarantee pain when moving and ensure consistent test results,
        // set to 350% burden.

        clear_avatar();
        burden_player( dummy, 3.5 );

        WHEN( "they have zero stamina left" ) {
            dummy.set_stamina( 0 );
            REQUIRE( dummy.get_stamina() == 0 );

            THEN( "they feel pain when carrying too much weight" ) {
                pain_before = dummy.get_pain();
                dummy.burn_move_stamina( to_moves<int>( 1_turns ) );
                pain_after = dummy.get_pain();
                CHECK( pain_after > pain_before );
            }
        }

        WHEN( "they have a bad back" ) {
            dummy.toggle_trait( trait_id( "BADBACK" ) );
            REQUIRE( dummy.has_trait( trait_id( "BADBACK" ) ) );

            THEN( "they feel pain when carrying too much weight" ) {
                pain_before = dummy.get_pain();
                dummy.burn_move_stamina( to_moves<int>( 1_turns ) );
                pain_after = dummy.get_pain();
                CHECK( pain_after > pain_before );
            }
        }
    }
}

TEST_CASE( "stamina regeneration rate", "[stamina][update][regen]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    int turn_moves = to_moves<int>( 1_turns );

    const float normal_regen_rate = get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" );
    REQUIRE( normal_regen_rate > 0 );

    GIVEN( "character is not winded" ) {
        catch_breath( dummy );

        THEN( "they regain stamina at the normal rate per turn" ) {
            CHECK( actual_regen_rate( dummy, turn_moves ) == normal_regen_rate * turn_moves );
        }
    }

    GIVEN( "character is winded" ) {
        dummy.add_effect( effect_winded, 10_turns );
        REQUIRE( dummy.has_effect( effect_winded ) );

        THEN( "they regain stamina at only 10%% the normal rate per turn" ) {
            CHECK( actual_regen_rate( dummy, turn_moves ) == 0.1 * normal_regen_rate * turn_moves );
        }
    }
}

TEST_CASE( "stamina regen in different movement modes", "[stamina][update][regen][mode]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    catch_breath( dummy );

    int turn_moves = to_moves<int>( 1_turns );

    dummy.set_movement_mode( move_mode_run );
    REQUIRE( dummy.movement_mode_is( move_mode_run ) );
    float run_regen_rate = actual_regen_rate( dummy, turn_moves );

    dummy.set_movement_mode( move_mode_walk );
    REQUIRE( dummy.movement_mode_is( move_mode_walk ) );
    float walk_regen_rate = actual_regen_rate( dummy, turn_moves );

    dummy.set_movement_mode( move_mode_crouch );
    REQUIRE( dummy.movement_mode_is( move_mode_crouch ) );
    float crouch_regen_rate = actual_regen_rate( dummy, turn_moves );

    THEN( "run and walk mode give the same stamina regen per turn" ) {
        CHECK( run_regen_rate == walk_regen_rate );
    }

    THEN( "walk and crouch mode give the same stamina regen per turn" ) {
        CHECK( walk_regen_rate == crouch_regen_rate );
    }

    THEN( "crouch and run mode give the same stamina regen per turn" ) {
        CHECK( crouch_regen_rate == run_regen_rate );
    }
}

TEST_CASE( "stamina regen with mouth encumbrance", "[stamina][update][regen][encumbrance]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    catch_breath( dummy );
    dummy.set_body();

    int turn_moves = to_moves<int>( 1_turns );

    const float normal_regen_rate = get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" );
    REQUIRE( normal_regen_rate > 0 );

    GIVEN( "character has mouth encumbrance" ) {
        dummy.wear_item( item( "scarf_fur" ) );
        REQUIRE( dummy.encumb( bodypart_id( "mouth" ) ) == 10 );

        THEN( "stamina regen is reduced" ) {
            CHECK( actual_regen_rate( dummy, turn_moves ) == ( normal_regen_rate - 2 ) * turn_moves );

            WHEN( "they have even more mouth encumbrance" ) {
                // Layering two scarves triples the encumbrance
                dummy.wear_item( item( "scarf_fur" ) );
                REQUIRE( dummy.encumb( bodypart_id( "mouth" ) ) == 30 );

                THEN( "stamina regen is reduced further" ) {
                    CHECK( actual_regen_rate( dummy, turn_moves ) == ( normal_regen_rate - 6 ) * turn_moves );
                }
            }
        }
    }
}
