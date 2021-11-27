#pragma once
#ifndef CATA_SRC_DIARY_H
#define CATA_SRC_DIARY_H

#include <string>
#include <list>
#include <vector>

#include "achievement.h"
#include "character.h"
#include "kill_tracker.h"
#include "skill.h"
#include "stats_tracker.h"
#include "ui.h"
#include "units.h"


/// <summary>
/// diary page, to save current character progression
/// </summary>
struct diary_page {
    diary_page();
    virtual ~diary_page() = default;
    /*the text the player added to the page*/
    std::string m_text;
    time_point privius_Page_turn;
    std::vector<std::string> diff_to_previous_page;
    /*turn the page was created*/
    time_point turn;
    /*mission ids for completed/active and faild missions*/
    std::vector<int> mission_completed;
    std::vector<int> mission_active;
    std::vector<int> mission_faild;
    /*monster id and kill count of Killed monster*/
    std::map<mtype_id, int> kills;
    /*names of killed npc`s*/
    std::vector<std::string> npc_kills;
    /*gender*/
    bool male;
    /*base character stats*/
    int strength;
    int dexterity;
    int intelligence;
    int perception;
    /*traits id the character has*/
    std::vector<trait_id> traits;
    /*spells id with level the character has*/
    std::map<spell_id, int> known_spells;
    /*bionics id`s the character has*/
    std::vector<bionic_id> bionics;
    /*skill id's with level the character has*/
    std::map<skill_id, int> skillsL;
    /*known and learning profession id of the character*/
    std::vector<proficiency_id> known_profs;
    std::vector<proficiency_id> learning_profs;
    /*maximal power level the character has*/
    units::energy max_power_level;
};

/// <summary>
/// diary is connectet to the player avatar.
/// the player is able to add new pages every page saves the current character progression and shows the improvements compared to the previous pages
/// The player is also able to add a Text in every page.
/// </summary>
class diary
{
        //attribute
    private:
        /*character name who owns the diary*/
        std::string owner;
        /*list of all pages added to the diary*/
        std::vector< std::unique_ptr<diary_page>> pages;

        /*current opend page*/
        int opend_page = 0; // NOLINT(cata-serialize)
        /*list of changes from opened page to previous page*/
        std::vector<std::string> change_list; // NOLINT(cata-serialize)
        /*maps discription to position in change list*/
        std::map<int, std::string> desc_map; // NOLINT(cata-serialize)


        //methoden
    public:
        diary();
        virtual ~diary() = default;
        /*static methode to open a diary ui*/
        static void show_diary_ui( diary *c_diary );
        /*last entry in the diary, will be called after character death */
        void death_entry();

        /*serialize and deserialize*/
        bool store();
        void load();
        void serialize( std::ostream &fout );
        void deserialize( std::istream &fin );
        void deserialize( JsonIn &jsin );
        void serialize( JsonOut &jsout );

    private:
        /*uses string_popup_window to edit the text on a page. Is not optimal,
        because its just one line*/
        void edit_page_ui();
        /*Uses editor window class to edit the text.*/
        void edit_page_ui( catacurses::window &win );
        /*set page to be be shown in ui*/
        int set_opend_page( int pagenum );
        /*create a new page and adds current character progression*/
        void new_page();
        /*set page text*/
        void set_page_text( std::string text );
        /*delite current page*/
        void delete_page();

        /*get opend page nummer*/
        int get_opend_page_num();
        /*returns a list with all pages by the its date*/
        std::vector<std::string> get_pages_list();
        /*returns a list with all changes compared to the previous page*/
        std::vector<std::string> get_change_list();
        /*returns a map corresponding to the change_list with descriptions*/
        std::map<int, std::string> get_desc_map();

        /*returns pointer to current page*/
        diary_page *get_page_ptr( int offset = 0 );
        /*returns the text of opend page*/
        std::string get_page_text();
        /*returns text for head of page*/
        std::string get_head_text();
        /*the following methods are used to fill the change_list and desc_map in comparison to the previous page*/
        void skill_changes();
        void kill_changes();
        void trait_changes();
        void bionic_changes();
        void stat_changes();
        void prof_changes();
        void mission_changes();
        void spell_changes();

        /*expots the diary to a readable .txt file. If its the lastexport, its exportet to memorial otherwise its exportet to the world folder*/
        void export_to_txt( bool lastexport = false );
        /*method for adding changes to the changelist. with the possibility to connect a desciption*/
        void add_to_change_list( std::string entry, std::string desc = "" );
};
#endif // CATA_SRC_DIARY_H
