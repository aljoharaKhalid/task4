#pragma once
#ifndef CATA_SRC_TYPE_ID_H
#define CATA_SRC_TYPE_ID_H

// IWYU pragma: begin_exports
#include "int_id.h"
#include "string_id.h"
// IWYU pragma: end_exports

class achievement;
using achievement_id = string_id<achievement>;

class activity_type;
using activity_id = string_id<activity_type>;

class ammunition_type;
using ammotype = string_id<ammunition_type>;

struct ammo_effect;
using ammo_effect_id = int_id<ammo_effect>;
using ammo_effect_str_id = string_id<ammo_effect>;

class ascii_art;
using ascii_art_id = string_id<ascii_art>;

struct bionic_data;
using bionic_id = string_id<bionic_data>;

struct body_part_type;
using bodypart_id = int_id<body_part_type>;
using bodypart_str_id = string_id<body_part_type>;

struct construction;
using construction_id = int_id<construction>;
using construction_str_id = string_id<construction>;

struct construction_category;
using construction_category_id = string_id<construction_category>;

struct construction_group;
using construction_group_str_id = string_id<construction_group>;

struct clothing_mod;
using clothing_mod_id = string_id<clothing_mod>;

class effect_type;
using efftype_id = string_id<effect_type>;

class disease_type;
using diseasetype_id = string_id<disease_type>;

struct effect_on_condition;
using effect_on_condition_id = string_id<effect_on_condition>;

class emit;
using emit_id = string_id<emit>;

class enchantment;
using enchantment_id = string_id<enchantment>;

class fault;
using fault_id = string_id<fault>;

struct field_type;
using field_type_id = int_id<field_type>;
using field_type_str_id = string_id<field_type>;

struct furn_t;
using furn_id = int_id<furn_t>;
using furn_str_id = string_id<furn_t>;

class gun_mode;
using gun_mode_id = string_id<gun_mode>;

class harvest_list;
using harvest_id = string_id<harvest_list>;

class item_category;
using item_category_id = string_id<item_category>;

class Item_spawn_data;
// note: "dynamic" string id, see string_id_params in string_id.h
using item_group_id = string_id<Item_spawn_data>;

struct itype;
using itype_id = string_id<itype>;

class json_flag;
using flag_id = string_id<json_flag>;

class ma_buff;
using mabuff_id = string_id<ma_buff>;

class martialart;
using matype_id = string_id<martialart>;

class ma_technique;
using matec_id = string_id<ma_technique>;

class material_type;
using material_id = string_id<material_type>;

struct mission_type;
using mission_type_id = string_id<mission_type>;

struct MOD_INFORMATION;
using mod_id = string_id<MOD_INFORMATION>;

class monfaction;
using mfaction_id = int_id<monfaction>;
using mfaction_str_id = string_id<monfaction>;

struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

class morale_type_data;
using morale_type = string_id<morale_type_data>;

struct mtype;
using mtype_id = string_id<mtype>;

class npc_class;
using npc_class_id = string_id<npc_class>;

class faction;
using faction_id = string_id<faction>;

struct oter_t;
using oter_id = int_id<oter_t>;
using oter_str_id = string_id<oter_t>;

struct oter_type_t;
using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

class profession;
using profession_id = string_id<profession>;

class recipe;
using recipe_id = string_id<recipe>;

struct requirement_data;
using requirement_id = string_id<requirement_data>;

class scent_type;
using scenttype_id = string_id<scent_type>;

class Skill;
using skill_id = string_id<Skill>;

class SkillDisplayType;
using skill_displayType_id = string_id<SkillDisplayType>;

struct species_type;
using species_id = string_id<species_type>;

class spell_type;
using spell_id = string_id<spell_type>;

class start_location;
using start_location_id = string_id<start_location>;

class move_mode;
using move_mode_id = string_id<move_mode>;

struct mutation_category_trait;
using mutation_category_id = string_id<mutation_category_trait>;

class proficiency;
using proficiency_id = string_id<proficiency>;

struct ter_t;
using ter_id = int_id<ter_t>;
using ter_str_id = string_id<ter_t>;

class ter_furn_transform;
using ter_furn_transform_id = string_id<ter_furn_transform>;

class Trait_group;
namespace trait_group
{
using Trait_group_tag = string_id<Trait_group>;
// note: "dynamic" string id, see string_id_params in string_id.h
} // namespace trait_group

class translation;
using snippet_id = string_id<translation>;

struct trap;
using trap_id = int_id<trap>;
using trap_str_id = string_id<trap>;

struct mutation_branch;
using trait_id = string_id<mutation_branch>;

struct quality;
using quality_id = string_id<quality>;

class VehicleGroup;
using vgroup_id = string_id<VehicleGroup>;

class vitamin;
using vitamin_id = string_id<vitamin>;

class vpart_info;
using vpart_id = string_id<vpart_info>;

struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;

struct weather_type;
using weather_type_id = string_id<weather_type>;

class zone_type;
using zone_type_id = string_id<zone_type>;

#endif // CATA_SRC_TYPE_ID_H
