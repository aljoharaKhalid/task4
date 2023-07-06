#pragma once
#ifndef CATA_SRC_TALKER_H
#define CATA_SRC_TALKER_H

#include "coordinates.h"
#include "effect.h"
#include "item.h"
#include "messages.h"
#include "type_id.h"
#include "item_location.h"
#include "output.h"
#include "units.h"
#include "units_fwd.h"
#include <list>

class computer;
class faction;
class faction_price_rule;
class mission;
class monster;
class npc;
struct npc_opinion;
class Character;
class recipe;
struct tripoint;
class vehicle;

using bodytype_id = std::string;

/*
 * Talker is an entity independent way of providing a participant in a dialogue.
 * Talker is a virtual abstract class and should never really be used.  Instead,
 * entity specific talker child classes such as character_talker should be used.
 */
class talker
{
    public:
        virtual ~talker() = default;
        virtual std::unique_ptr<talker> clone() const {
            return std::make_unique<talker>();
        }
        // virtual member accessor functions
        virtual Character *get_character() {
            return nullptr;
        }
        virtual const Character *get_character() const {
            return nullptr;
        }
        virtual npc *get_npc() {
            return nullptr;
        }
        virtual const npc *get_npc() const {
            return nullptr;
        }
        virtual item_location *get_item() {
            return nullptr;
        }
        virtual item_location *get_item() const {
            return nullptr;
        }
        virtual monster *get_monster() {
            return nullptr;
        }
        virtual const monster *get_monster() const {
            return nullptr;
        }
        virtual Creature *get_creature() {
            return nullptr;
        }
        virtual const Creature *get_creature() const {
            return nullptr;
        }
        virtual computer *get_computer() {
            return nullptr;
        }
        virtual const computer *get_computer() const {
            return nullptr;
        }
        // identity and location
        virtual std::string disp_name() const {
            return "";
        }
        virtual character_id getID() const {
            return character_id( 0 );
        }
        virtual bool is_male() const {
            return false;
        }
        virtual std::vector<std::string> get_grammatical_genders() const {
            return {};
        }
        virtual int posx() const {
            return 0;
        }
        virtual int posy() const {
            return 0;
        }
        virtual int posz() const {
            return 0;
        }
        virtual tripoint pos() const {
            return {};
        }
        virtual tripoint_abs_ms global_pos() const {
            return {};
        }
        virtual tripoint_abs_omt global_omt_location() const {
            return {};
        }
        virtual void set_pos( tripoint ) {}
        virtual std::string distance_to_goal() const {
            return "";
        }

        // mandatory functions for starting a dialogue
        virtual bool will_talk_to_u( const Character &, bool ) {
            return false;
        }
        virtual std::vector<std::string> get_topics( bool ) {
            return {};
        }
        virtual void check_missions() {}
        virtual void update_missions( const std::vector<mission *> & ) {}
        virtual bool check_hostile_response( int ) const {
            return false;
        }
        virtual int parse_mod( const std::string &, int ) const {
            return 0;
        }
        virtual int trial_chance_mod( const std::string & ) const {
            return 0;
        }
        virtual int get_cur_hp( const bodypart_id & ) const {
            return 0;
        }
        virtual int get_cur_part_temp( const bodypart_id & ) const {
            return 0;
        }
        virtual bool get_is_alive() const {
            return false;
        }

        // stats, skills, traits, bionics, and magic
        virtual int str_cur() const {
            return 0;
        }
        virtual int dex_cur() const {
            return 0;
        }
        virtual int int_cur() const {
            return 0;
        }
        virtual int per_cur() const {
            return 0;
        }
        virtual void set_str_max( int ) {}
        virtual void set_dex_max( int ) {}
        virtual void set_int_max( int ) {}
        virtual void set_per_max( int ) {}
        virtual void set_str_bonus( int ) {}
        virtual void set_dex_bonus( int ) {}
        virtual void set_int_bonus( int ) {}
        virtual void set_per_bonus( int ) {}
        virtual int get_str_max() const {
            return 0;
        }
        virtual int get_dex_max() const {
            return 0;
        }
        virtual int get_int_max() const {
            return 0;
        }
        virtual int get_per_max() const {
            return 0;
        }
        virtual int get_str_bonus() const {
            return 0;
        }
        virtual int get_dex_bonus() const {
            return 0;
        }
        virtual int get_int_bonus() const {
            return 0;
        }
        virtual int get_per_bonus() const {
            return 0;
        }
        virtual int get_skill_level( const skill_id & ) const {
            return 0;
        }
        virtual int get_spell_level( const trait_id & ) const {
            return 0;
        }
        virtual int get_spell_level( const spell_id & ) const {
            return 0;
        }
        virtual int get_spell_exp( const spell_id & ) const {
            return 0;
        }
        virtual int get_highest_spell_level() const {
            return 0;
        }
        virtual void set_spell_level( const spell_id &, int ) {}
        virtual void set_spell_exp( const spell_id &, int ) {}
        virtual void set_skill_level( const skill_id &, int ) {}
        virtual bool has_trait( const trait_id & ) const {
            return false;
        }
        virtual bool has_recipe( const recipe_id & ) const {
            return false;
        }
        virtual void learn_recipe( const recipe_id & ) {}
        virtual void forget_recipe( const recipe_id & ) {}
        virtual void mutate( const int &, const bool & ) {}
        virtual void mutate_category( const mutation_category_id &, const bool & ) {}
        virtual void set_mutation( const trait_id & ) {}
        virtual void unset_mutation( const trait_id & ) {}
        virtual void activate_mutation( const trait_id & ) {}
        virtual void deactivate_mutation( const trait_id & ) {}
        virtual void set_fatigue( int ) {};
        virtual bool has_flag( const json_character_flag & ) const {
            return false;
        }
        virtual bool has_species( const species_id & ) const {
            return false;
        }
        virtual bool bodytype( const bodytype_id & ) const {
            return false;
        }
        virtual int get_grab_strength() const {
            return 0;
        }
        virtual bool crossed_threshold() const {
            return false;
        }
        virtual int num_bionics() const {
            return 0;
        }
        virtual bool has_max_power() const {
            return false;
        }
        virtual bool has_bionic( const bionic_id & ) const {
            return false;
        }
        virtual bool knows_spell( const spell_id & ) const {
            return false;
        }
        virtual bool knows_proficiency( const proficiency_id & ) const {
            return false;
        }
        virtual time_duration proficiency_practiced_time( const proficiency_id & ) const {
            return 0_seconds;
        }
        virtual void set_proficiency_practiced_time( const proficiency_id &, int ) {}
        virtual std::vector<skill_id> skills_offered_to( const talker & ) const {
            return {};
        }
        virtual std::vector<skill_id> skills_teacheable() const {
            return {};
        }
        virtual std::string skill_seminar_text( const skill_id & ) const {
            return {};
        }
        virtual std::string skill_training_text( const talker &, const skill_id & ) const {
            return {};
        }
        virtual std::vector<proficiency_id> proficiencies_offered_to( const talker & ) const {
            return {};
        }
        virtual std::string proficiency_training_text( const talker &, const proficiency_id & ) const {
            return {};
        }
        virtual std::vector<matype_id> styles_offered_to( const talker & ) const {
            return {};
        }
        virtual std::string style_training_text( const talker &, const matype_id & ) const {
            return {};
        }
        virtual std::vector<spell_id> spells_offered_to( talker & ) {
            return {};
        }
        virtual std::string spell_training_text( talker &, const spell_id & ) {
            return {};
        }
        virtual void store_chosen_training( const skill_id &, const matype_id &,
                                            const spell_id &, const proficiency_id & ) {
        }

        // effects and values
        virtual bool has_effect( const efftype_id &, const bodypart_id & ) const {
            return false;
        }
        virtual effect get_effect( const efftype_id &, const bodypart_id & ) const {
            return effect::null_effect;
        }
        virtual bool is_deaf() const {
            return false;
        }
        virtual bool can_see() const {
            return false;
        }
        virtual bool is_mute() const {
            return false;
        }
        virtual void add_effect( const efftype_id &, const time_duration &, const std::string &, bool, bool,
                                 int ) {}
        virtual void remove_effect( const efftype_id & ) {}
        virtual void add_bionic( const bionic_id & ) {}
        virtual void remove_bionic( const bionic_id & ) {}
        virtual std::string get_value( const std::string & ) const {
            return "";
        }
        virtual void set_value( const std::string &, const std::string & ) {}
        virtual void remove_value( const std::string & ) {}

        // inventory, buying, and selling
        virtual bool is_wearing( const itype_id & ) const {
            return false;
        }
        virtual int charges_of( const itype_id & ) const {
            return 0;
        }
        virtual bool has_charges( const itype_id &, int ) const {
            return false;
        }
        // bool = match tool containing charges of itype_id
        virtual bool has_charges( const itype_id &, int, bool ) const {
            return false;
        }
        virtual std::list<item> use_charges( const itype_id &, int ) {
            return {};
        }
        // bool = match tool containing charges of itype_id
        virtual std::list<item> use_charges( const itype_id &, int, bool ) {
            return {};
        }
        virtual bool has_amount( const itype_id &, int ) const {
            return false;
        }
        virtual int get_amount( const itype_id & ) const {
            return 0;
        }
        virtual std::list<item> use_amount( const itype_id &, int ) {
            return {};
        }
        virtual int value( const item & ) const {
            return 0;
        }
        virtual int value( const item &, double ) const {
            return 0;
        }
        virtual int cash() const {
            return 0;
        }
        virtual int debt() const {
            return 0;
        }
        virtual void add_debt( int ) {}
        virtual int sold() const {
            return 0;
        }
        virtual void add_sold( int ) {}
        virtual std::vector<item *> items_with( const std::function<bool( const item & )> & ) const {
            return {};
        }
        virtual std::vector<const item *> const_items_with( const std::function<bool( const item & )> & )
        const {
            return {};
        }
        virtual void i_add( const item & ) {}
        virtual bool i_add_or_drop( item &, int = 1, const item * = nullptr, const item * = nullptr ) {
            return false;
        }
        virtual ret_val<item_location> i_add_or_fill( item &, bool, const item *, const item *, bool, bool,
                bool ) {
            return ret_val<item_location>::make_failure( item_location::nowhere );
        }
        virtual void remove_items_with( const std::function<bool( const item & )> &, int = INT_MAX ) {}
        virtual bool unarmed_attack() const {
            return false;
        }
        virtual bool can_stash_weapon() const {
            return false;
        }
        virtual bool has_stolen_item( const talker & ) const {
            return false;
        }
        virtual int cash_to_favor( int ) const {
            return 0;
        }
        virtual std::string give_item_to( bool ) {
            return _( "Nope." );
        }
        virtual bool buy_from( int ) {
            return false;
        }
        virtual bool buy_monster( talker &, const mtype_id &, int, int, bool,
                                  const translation & ) {
            return false;
        }

        // missions
        virtual std::vector<mission *> available_missions() const {
            return {};
        }
        virtual std::vector<mission *> assigned_missions() const {
            return {};
        }
        virtual mission *selected_mission() const {
            return nullptr;
        }
        virtual void select_mission( mission * ) {
        }
        virtual void add_mission( const mission_type_id & ) {}
        virtual void set_companion_mission( const std::string & ) {}

        // factions and alliances
        virtual faction *get_faction() const {
            return nullptr;
        }
        virtual void set_fac( const faction_id & ) {}
        virtual void add_faction_rep( int ) {}
        virtual bool is_following() const {
            return false;
        }
        virtual bool is_friendly( const Character & )  const {
            return false;
        }
        virtual bool is_player_ally()  const {
            return false;
        }
        virtual bool turned_hostile() const {
            return false;
        }
        virtual bool is_enemy() const {
            return false;
        }
        virtual void make_angry() {}

        // ai rules
        virtual bool has_ai_rule( const std::string &, const std::string & ) const {
            return false;
        }
        virtual void toggle_ai_rule( const std::string &, const std::string & ) {}
        virtual void set_ai_rule( const std::string &, const std::string & ) {}
        virtual void clear_ai_rule( const std::string &, const std::string & ) {}

        // other descriptors
        virtual std::string get_job_description() const {
            return "";
        }
        virtual std::string evaluation_by( const talker & ) const {
            return "";
        }
        virtual std::string short_description() const {
            return "";
        }
        virtual bool has_activity() const {
            return false;
        }
        virtual bool is_mounted() const {
            return false;
        }
        virtual bool is_myclass( const npc_class_id & ) const {
            return false;
        }
        virtual void set_class( const npc_class_id & ) {}
        virtual int get_activity_level() const {
            return 0;
        }
        virtual int get_fatigue() const {
            return 0;
        }
        virtual int get_hunger() const {
            return 0;
        }
        virtual int get_thirst() const {
            return 0;
        }
        virtual int get_instant_thirst() const {
            return 0;
        }
        virtual int get_stored_kcal() const {
            return 0;
        }
        virtual int get_healthy_kcal() const {
            return 0;
        }
        virtual int get_size() const {
            return 0;
        }
        virtual int get_stim() const {
            return 0;
        }
        virtual int get_addiction_intensity( const addiction_id & ) const {
            return 0;
        }
        virtual int get_addiction_turns( const addiction_id & ) const {
            return 0;
        }
        virtual void set_addiction_turns( const addiction_id &, int ) {}
        virtual void set_stored_kcal( int ) {}
        virtual void set_stim( int ) {}
        virtual void set_thirst( int ) {}
        virtual bool is_in_control_of( const vehicle & ) const {
            return false;
        }

        // speaking
        virtual void say( const std::string & ) {}
        virtual void shout( const std::string & = "", bool = false ) {}

        // miscellaneous
        virtual bool enslave_mind() {
            return false;
        }
        virtual std::string opinion_text() const {
            return "";
        }
        virtual void add_opinion( const npc_opinion & ) {}
        virtual void set_first_topic( const std::string & ) {}
        virtual bool is_safe() const {
            return true;
        }
        virtual void mod_pain( int ) {}
        virtual void set_pain( int ) {}
        virtual int pain_cur() const {
            return 0;
        }
        virtual void attack_target( Creature &, bool, const matec_id &,
                                    bool, int ) {}

        virtual int attack_speed() const {
            return 0;
        }
        virtual double armor_at( damage_type_id &, bodypart_id & ) const {
            return 0;
        }
        virtual bool worn_with_flag( const flag_id &, const bodypart_id & ) const {
            return false;
        }
        virtual bool wielded_with_flag( const flag_id & ) const {
            return false;
        }
        virtual bool has_item_with_flag( const flag_id & ) const {
            return false;
        }
        virtual int item_rads( const flag_id &, aggregate_type ) const {
            return 0;
        }
        virtual units::energy power_cur() const {
            return 0_kJ;
        }
        virtual units::energy power_max() const {
            return 0_kJ;
        }
        virtual void set_power_cur( units::energy ) {}
        virtual int mana_cur() const {
            return 0;
        }
        virtual int mana_max() const {
            return 0;
        }
        virtual void set_mana_cur( int ) {}
        virtual void mod_daily_health( int, int ) {}
        virtual int morale_cur() const {
            return 0;
        }
        virtual int focus_cur() const {
            return 0;
        }
        virtual void mod_focus( int ) {}
        virtual int get_pkill() const {
            return 0;
        }
        virtual void set_pkill( int ) {}
        virtual int get_stamina() const {
            return 0;
        }
        virtual void set_stamina( int ) {}
        virtual int get_sleep_deprivation() const {
            return 0;
        }
        virtual void set_sleep_deprivation( int ) {}
        virtual int get_rad() const {
            return 0;
        }
        virtual void set_rad( int ) {}
        virtual int get_anger() const {
            return 0;
        }
        virtual void set_anger( int ) {}
        virtual void set_morale( int ) {}
        virtual int get_friendly() const {
            return 0;
        }
        virtual void set_friendly( int ) {}
        virtual void add_morale( const morale_type &, int, int, time_duration, time_duration, bool ) {}
        virtual void remove_morale( const morale_type & ) {}
        virtual void set_kill_xp( int ) {}
        virtual int get_kill_xp() const {
            return 0;
        }
        virtual void set_age( int ) {}
        virtual int get_age() const {
            return 0;
        }
        virtual void set_height( int ) {}
        virtual int get_height() const {
            return 0;
        }
        virtual void set_npc_trust( int ) {}
        virtual int get_npc_trust() const {
            return 0;
        }
        virtual void set_npc_fear( int ) {}
        virtual int get_npc_fear() const {
            return 0;
        }
        virtual void set_npc_value( int ) {}
        virtual int get_npc_value() const {
            return 0;
        }
        virtual void set_npc_anger( int ) {}
        virtual int get_npc_anger() const {
            return 0;
        }
        virtual int get_bmi_permil() const {
            return 0;
        }
        virtual const move_mode_id &get_move_mode() const {
            return move_mode_id::NULL_ID();
        }
        virtual int get_fine_detail_vision_mod() const {
            return 0;
        }
        virtual int get_health() const {
            return 0;
        }
        virtual int get_body_temp() const {
            return 0;
        }
        virtual int get_body_temp_delta() const {
            return 0;
        }
        virtual std::vector<bodypart_id> get_all_body_parts( bool, bool ) const {
            return std::vector<bodypart_id>();
        }
        virtual int get_part_hp_cur( const bodypart_id & ) const {
            return 0;
        }
        virtual int get_part_hp_max( const bodypart_id & ) const {
            return 0;
        }
        virtual void set_part_hp_cur( const bodypart_id &, int ) const {}
        virtual void die() {}
        virtual matec_id get_random_technique( Creature &, bool, bool,
                                               bool, const std::vector<matec_id> & = {} ) const {
            return matec_id();
        }
        virtual void learn_martial_art( const matype_id & ) const {}
        virtual void forget_martial_art( const matype_id & ) const {}
        virtual bool knows_martial_art( const matype_id & ) const {
            return false;
        }
        virtual ret_val<void> wants_to_sell( const item_location &it, int at_price ) const {
            return ret_val<void>::make_failure( _( "Not defined for this talker type." ) );
        }
        virtual ret_val<void> wants_to_buy( const item &it, int at_price ) const {
            return ret_val<void>::make_failure( _( "Not defined for this talker type." ) );
        }
        virtual faction_price_rule const *get_price_rules( item const &it ) const {
            return nullptr;
        }
        virtual void drop_invalid_inventory() {}
        virtual void shop_restock() {}
        virtual bool is_avatar() const {
            return false;
        }
        virtual bool is_shopkeeper() const {
            return false;
        }
        virtual bool will_exchange_items_freely() const {
            return false;
        }
        virtual int max_credit_extended() const {
            return 0;
        }
        virtual bool can_fit_items( std::vector<std::pair<item_location, int>> const &to_trade ) const {
            return false;
        }
        virtual int max_willing_to_owe() const {
            return 0;
        }
        virtual bool is_wielding( const item &target ) const {
            return false;
        }
        virtual std::string replace_with_npc_name( std::string input ) const {
            replace_substring( input, "<npcname>", disp_name(), true );
            return input;
        }
        virtual bool is_worn( const item &clothing ) const {
            return false;
        }
        virtual ret_val<void> can_takeoff( const item &it, const std::list<item> *res ) {
            return ret_val<void>::make_failure( _( "Not defined for this talker type." ) );
        }
        virtual item_location get_wielded_item() const {
            return item_location();
        }
        virtual std::vector<item_location> top_items_loc() {
            return std::vector<item_location>();
        }
        virtual int get_debt() const {
            return 0;
        }
};
template <class T, class B = talker>
class talker_cloner : public B
{
    public:
        std::unique_ptr<talker> clone() const override {
            return std::make_unique<T>( static_cast<T const &>( *this ) );
        }
};
#endif // CATA_SRC_TALKER_H
