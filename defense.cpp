#include "gamemode.h"
#include "game.h"
#include "setvector.h"
#include "keypress.h"
#include "itype.h"
#include "mtype.h"
#include "overmapbuffer.h"
#include <string>
#include <vector>
#include <sstream>

#define SPECIAL_WAVE_CHANCE 5 // One in X chance of single-flavor wave
#define SPECIAL_WAVE_MIN 5 // Don't use a special wave with < X monsters

#define SELCOL(n) (selection == (n) ? c_yellow : c_blue)
#define TOGCOL(n, b) (selection == (n) ? (b ? c_ltgreen : c_yellow) :\
                      (b ? c_green : c_dkgray))
#define NUMALIGN(n) ((n) >= 10000 ? 20 : ((n) >= 1000 ? 21 :\
                     ((n) >= 100 ? 22 : ((n) >= 10 ? 23 : 24))))

std::string caravan_category_name(caravan_category cat);
std::vector<itype_id> caravan_items(caravan_category cat);

int caravan_price(player &u, int price);

void draw_caravan_borders(WINDOW *w, int current_window);
void draw_caravan_categories(WINDOW *w, int category_selected, int total_price,
                             int cash);
void draw_caravan_items(WINDOW *w, game *g, std::vector<itype_id> *items,
                        std::vector<int> *counts, int offset,
                        int item_selected);

std::string defense_style_name(defense_style style);
std::string defense_style_description(defense_style style);
std::string defense_location_name(defense_location location);
std::string defense_location_description(defense_location location);

defense_game::defense_game()
{
 current_wave = 0;
 hunger = false;
 thirst = false;
 sleep  = false;
 zombies = false;
 specials = false;
 spiders = false;
 triffids = false;
 robots = false;
 subspace = false;
 mercenaries = false;
 init_to_style(DEFENSE_EASY);
}

bool defense_game::init(game *g)
{
 g->turn = HOURS(12); // Start at noon
 g->temperature = 65;
 if (!g->u.create(g, PLTYPE_CUSTOM))
  return false;
 g->u.str_cur = g->u.str_max;
 g->u.per_cur = g->u.per_max;
 g->u.int_cur = g->u.int_max;
 g->u.dex_cur = g->u.dex_max;
 init_itypes(g);
 init_mtypes(g);
 init_constructions(g);
 init_recipes(g);
 current_wave = 0;
 hunger = false;
 thirst = false;
 sleep  = false;
 zombies = false;
 specials = false;
 spiders = false;
 triffids = false;
 robots = false;
 subspace = false;
 mercenaries = false;
 init_to_style(DEFENSE_EASY);
 setup();
 g->u.cash = initial_cash;
 popup_nowait("Please wait as the map generates [ 0%]");
 // TODO: support multiple defence games? clean up old defence game
 g->cur_om = &overmap_buffer.get(g, 0, 0);
 init_map(g);
 caravan(g);
 return true;
}

void defense_game::per_turn(game *g)
{
 if (!thirst)
  g->u.thirst = 0;
 if (!hunger)
  g->u.hunger = 0;
 if (!sleep)
  g->u.fatigue = 0;
 if (int(g->turn) % (time_between_waves * 10) == 0) {
  current_wave++;
  if (current_wave > 1 && current_wave % waves_between_caravans == 0) {
   popup("A caravan approaches!  Press spacebar...");
   caravan(g);
  }
  spawn_wave(g);
 }
}

void defense_game::pre_action(game *g, action_id &act)
{
 if (act == ACTION_SLEEP && !sleep) {
  g->add_msg("You don't need to sleep!");
  act = ACTION_NULL;
 }
 if (act == ACTION_SAVE) {
  g->add_msg("You cannot save in defense mode!");
  act = ACTION_NULL;
 }

// Big ugly block for movement
 if ((act == ACTION_MOVE_N && g->u.posy == SEEX * int(MAPSIZE / 2) &&
      g->levy <= 93) ||
     (act == ACTION_MOVE_NE && ((g->u.posy == SEEY * int(MAPSIZE / 2) &&
                                 g->levy <=  93) ||
                                (g->u.posx == SEEX * (1 + int(MAPSIZE / 2))-1 &&
                                 g->levx >= 98))) ||
     (act == ACTION_MOVE_E && g->u.posx == SEEX * (1 + int(MAPSIZE / 2)) - 1 &&
      g->levx >= 98) ||
     (act == ACTION_MOVE_SE && ((g->u.posy == SEEY * (1 + int(MAPSIZE / 2))-1 &&
                                 g->levy >= 98) ||
                                (g->u.posx == SEEX * (1 + int(MAPSIZE / 2))-1 &&
                                 g->levx >= 98))) ||
     (act == ACTION_MOVE_S && g->u.posy == SEEY * (1 + int(MAPSIZE / 2))-1 &&
      g->levy >= 98) ||
     (act == ACTION_MOVE_SW && ((g->u.posy == SEEY * (1 + int(MAPSIZE / 2))-1 &&
                                 g->levy >= 98) ||
                                (g->u.posx == SEEX * int(MAPSIZE / 2) &&
                                 g->levx <=  93))) ||
     (act == ACTION_MOVE_W && g->u.posx == SEEX * int(MAPSIZE / 2) &&
      g->levx <= 93) ||
     (act == ACTION_MOVE_NW && ((g->u.posy == SEEY * int(MAPSIZE / 2) &&
                                 g->levy <=  93) ||
                                (g->u.posx == SEEX * int(MAPSIZE / 2) &&
                                 g->levx <=  93)))) {
  g->add_msg("You cannot leave the %s behind!",
             defense_location_name(location).c_str());
  act = ACTION_NULL;
 }
}

void defense_game::post_action(game *g, action_id act)
{
}

void defense_game::game_over(game *g)
{
 popup("You managed to survive through wave %d!", current_wave);
}

void defense_game::init_itypes(game *g)
{
 g->itypes["2x4"]->volume = 0;
 g->itypes["2x4"]->weight = 0;
 g->itypes["landmine"]->price = 300;
 g->itypes["bot_turret"]->price = 6000;
}

void defense_game::init_mtypes(game *g)
{
 for (int i = 0; i < num_monsters; i++) {
  g->mtypes[i]->difficulty *= 1.5;
  g->mtypes[i]->difficulty += int(g->mtypes[i]->difficulty / 5);
  g->mtypes[i]->flags.push_back(MF_BASHES);
  g->mtypes[i]->flags.push_back(MF_SMELLS);
  g->mtypes[i]->flags.push_back(MF_HEARS);
  g->mtypes[i]->flags.push_back(MF_SEES);
 }
}

void defense_game::init_constructions(game *g)
{
 for (int i = 0; i < g->constructions.size(); i++) {
  for (int j = 0; j < g->constructions[i]->stages.size(); j++) {
   g->constructions[i]->stages[j].time = 1; // Everything takes 1 minute
  }
 }
}

void defense_game::init_recipes(game *g)
{
    for (recipe_map::iterator map_iter = g->recipes.begin(); map_iter != g->recipes.end(); ++map_iter)
    {
        for (recipe_list::iterator list_iter = map_iter->second.begin(); list_iter != map_iter->second.end(); ++list_iter)
        {
            (*list_iter)->time /= 10; // Things take turns, not minutes
        }
    }
}

void defense_game::init_map(game *g)
{
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   g->cur_om->ter(x, y, 0) = ot_field;
   g->cur_om->seen(x, y, 0) = true;
  }
 }

 g->cur_om->save();
 g->levx = 100;
 g->levy = 100;
 g->levz = 0;
 g->u.posx = SEEX;
 g->u.posy = SEEY;

 switch (location) {

 case DEFLOC_HOSPITAL:
  for (int x = 49; x <= 51; x++) {
   for (int y = 49; y <= 51; y++)
    g->cur_om->ter(x, y, 0) = ot_hospital;
  }
  g->cur_om->ter(50, 49, 0) = ot_hospital_entrance;
  break;

 case DEFLOC_MALL:
  for (int x = 49; x <= 51; x++) {
   for (int y = 49; y <= 51; y++)
    g->cur_om->ter(x, y, 0) = ot_megastore;
  }
  g->cur_om->ter(50, 49, 0) = ot_megastore_entrance;
  break;

 case DEFLOC_BAR:
  g->cur_om->ter(50, 50, 0) = ot_bar_north;
  break;

 case DEFLOC_MANSION:
  for (int x = 49; x <= 51; x++) {
   for (int y = 49; y <= 51; y++)
    g->cur_om->ter(x, y, 0) = ot_mansion;
  }
  g->cur_om->ter(50, 49, 0) = ot_mansion_entrance;
  break;
 }
// Init the map
 int old_percent = 0;
 for (int i = 0; i <= MAPSIZE * 2; i += 2) {
  for (int j = 0; j <= MAPSIZE * 2; j += 2) {
   int mx = g->levx - MAPSIZE + i, my = g->levy - MAPSIZE + j;
   int percent = 100 * ((j / 2 + MAPSIZE * (i / 2))) /
                       ((MAPSIZE) * (MAPSIZE + 1));
   if (percent >= old_percent + 1) {
    popup_nowait("Please wait as the map generates [%s%d%]",
                 (percent < 10 ? " " : ""), percent);
    old_percent = percent;
   }
// Round down to the nearest even number
   mx -= mx % 2;
   my -= my % 2;
   tinymap tm(&g->itypes, &g->mapitems, &g->traps);
   tm.generate(g, g->cur_om, mx, my, 0, int(g->turn));
   tm.clear_spawns();
   tm.clear_traps();
   tm.save(g->cur_om, int(g->turn), mx, my, 0);
  }
 }

 g->m.load(g, g->levx, g->levy, g->levz, true);

 g->update_map(g->u.posx, g->u.posy);
 monster generator(g->mtypes[mon_generator], g->u.posx + 1, g->u.posy + 1);
// Find a valid spot to spawn the generator
 std::vector<point> valid;
 for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
  for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
   if (generator.can_move_to(g, x, y) && g->is_empty(x, y))
    valid.push_back( point(x, y) );
  }
 }
 if (!valid.empty()) {
  point p = valid[rng(0, valid.size() - 1)];
  generator.spawn(p.x, p.y);
 }
 generator.friendly = -1;
 g->z.push_back(generator);
}

void defense_game::init_to_style(defense_style new_style)
{
 style = new_style;
 hunger = false;
 thirst = false;
 sleep  = false;
 zombies = false;
 specials = false;
 spiders = false;
 triffids = false;
 robots = false;
 subspace = false;
 mercenaries = false;

 switch (new_style) {
 case DEFENSE_EASY:
  location = DEFLOC_HOSPITAL;
  initial_difficulty = 15;
  wave_difficulty = 10;
  time_between_waves = 30;
  waves_between_caravans = 3;
  initial_cash = 10000;
  cash_per_wave = 1000;
  cash_increase = 300;
  specials = true;
  spiders = true;
  triffids = true;
  mercenaries = true;
  break;

 case DEFENSE_MEDIUM:
  location = DEFLOC_MALL;
  initial_difficulty = 30;
  wave_difficulty = 15;
  time_between_waves = 20;
  waves_between_caravans = 4;
  initial_cash = 6000;
  cash_per_wave = 800;
  cash_increase = 200;
  specials = true;
  spiders = true;
  triffids = true;
  robots = true;
  hunger = true;
  mercenaries = true;
  break;

 case DEFENSE_HARD:
  location = DEFLOC_BAR;
  initial_difficulty = 50;
  wave_difficulty = 20;
  time_between_waves = 10;
  waves_between_caravans = 5;
  initial_cash = 2000;
  cash_per_wave = 600;
  cash_increase = 100;
  specials = true;
  spiders = true;
  triffids = true;
  robots = true;
  subspace = true;
  hunger = true;
  thirst = true;
  break;

  break;

 case DEFENSE_SHAUN:
  location = DEFLOC_BAR;
  initial_difficulty = 30;
  wave_difficulty = 15;
  time_between_waves = 5;
  waves_between_caravans = 6;
  initial_cash = 5000;
  cash_per_wave = 500;
  cash_increase = 100;
  zombies = true;
  break;

 case DEFENSE_DAWN:
  location = DEFLOC_MALL;
  initial_difficulty = 60;
  wave_difficulty = 20;
  time_between_waves = 30;
  waves_between_caravans = 4;
  initial_cash = 8000;
  cash_per_wave = 500;
  cash_increase = 0;
  zombies = true;
  hunger = true;
  thirst = true;
  mercenaries = true;
  break;

 case DEFENSE_SPIDERS:
  location = DEFLOC_MALL;
  initial_difficulty = 60;
  wave_difficulty = 10;
  time_between_waves = 10;
  waves_between_caravans = 4;
  initial_cash = 6000;
  cash_per_wave = 500;
  cash_increase = 100;
  spiders = true;
  break;

 case DEFENSE_TRIFFIDS:
  location = DEFLOC_MANSION;
  initial_difficulty = 60;
  wave_difficulty = 20;
  time_between_waves = 30;
  waves_between_caravans = 2;
  initial_cash = 10000;
  cash_per_wave = 600;
  cash_increase = 100;
  triffids = true;
  hunger = true;
  thirst = true;
  sleep = true;
  mercenaries = true;
  break;

 case DEFENSE_SKYNET:
  location = DEFLOC_HOSPITAL;
  initial_difficulty = 20;
  wave_difficulty = 20;
  time_between_waves = 20;
  waves_between_caravans = 6;
  initial_cash = 12000;
  cash_per_wave = 1000;
  cash_increase = 200;
  robots = true;
  hunger = true;
  thirst = true;
  mercenaries = true;
  break;

 case DEFENSE_LOVECRAFT:
  location = DEFLOC_MANSION;
  initial_difficulty = 20;
  wave_difficulty = 20;
  time_between_waves = 120;
  waves_between_caravans = 8;
  initial_cash = 4000;
  cash_per_wave = 1000;
  cash_increase = 100;
  subspace = true;
  hunger = true;
  thirst = true;
  sleep = true;
 }
}

void defense_game::setup()
{
 WINDOW* w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, 0, 0);
 int selection = 1;
 refresh_setup(w, selection);

 while (true) {
  char ch = input();

  if (ch == 'S') {
   if (!zombies && !specials && !spiders && !triffids && !robots && !subspace) {
    popup("You must choose at least one monster group!");
    refresh_setup(w, selection);
   } else
    return;
  } else if (ch == '+' || ch == '>' || ch == 'j') {
   if (selection == 19)
    selection = 1;
   else
    selection++;
   refresh_setup(w, selection);
  } else if (ch == '-' || ch == '<' || ch == 'k') {
   if (selection == 1)
    selection = 19;
   else
    selection--;
   refresh_setup(w, selection);
  } else if (ch == '!') {
   std::string name = string_input_popup("Template Name:", 20);
   refresh_setup(w, selection);
  } else if (ch == 'S')
   return;

  else {
   switch (selection) {
    case 1:	// Scenario selection
     if (ch == 'l') {
      if (style == defense_style(NUM_DEFENSE_STYLES - 1))
       style = defense_style(1);
      else
       style = defense_style(style + 1);
     }
     if (ch == 'h') {
      if (style == defense_style(1))
       style = defense_style(NUM_DEFENSE_STYLES - 1);
      else
       style = defense_style(style - 1);
     }
     init_to_style(style);
     break;

    case 2:	// Location selection
     if (ch == 'l') {
      if (location == defense_location(NUM_DEFENSE_LOCATIONS - 1))
       location = defense_location(1);
      else
       location = defense_location(location + 1);
     }
     if (ch == 'h') {
      if (location == defense_location(1))
       location = defense_location(NUM_DEFENSE_LOCATIONS - 1);
      else
       location = defense_location(location - 1);
     }
     mvwprintz(w, 5, 2, c_black, "\
 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
     mvwprintz(w, 5, 2, c_yellow, defense_location_name(location).c_str());
     mvwprintz(w,  5, 28, c_ltgray,
               defense_location_description(location).c_str());
     break;

    case 3:	// Difficulty of the first wave
     if (ch == 'h' && initial_difficulty > 10)
      initial_difficulty -= 5;
     if (ch == 'l' && initial_difficulty < 995)
      initial_difficulty += 5;
     mvwprintz(w, 7, 22, c_black, "xxx");
     mvwprintz(w, 7, NUMALIGN(initial_difficulty), c_yellow, "%d",
               initial_difficulty);
     break;

    case 4:	// Wave Difficulty
     if (ch == 'h' && wave_difficulty > 10)
      wave_difficulty -= 5;
     if (ch == 'l' && wave_difficulty < 995)
      wave_difficulty += 5;
     mvwprintz(w, 8, 22, c_black, "xxx");
     mvwprintz(w, 8, NUMALIGN(wave_difficulty), c_yellow, "%d",
               wave_difficulty);
     break;

    case 5:
     if (ch == 'h' && time_between_waves > 5)
      time_between_waves -= 5;
     if (ch == 'l' && time_between_waves < 995)
      time_between_waves += 5;
     mvwprintz(w, 10, 22, c_black, "xxx");
     mvwprintz(w, 10, NUMALIGN(time_between_waves), c_yellow, "%d",
               time_between_waves);
     break;

    case 6:
     if (ch == 'h' && waves_between_caravans > 1)
      waves_between_caravans -= 1;
     if (ch == 'l' && waves_between_caravans < 50)
      waves_between_caravans += 1;
     mvwprintz(w, 11, 22, c_black, "xxx");
     mvwprintz(w, 11, NUMALIGN(waves_between_caravans), c_yellow, "%d",
               waves_between_caravans);
     break;

    case 7:
     if (ch == 'h' && initial_cash > 0)
      initial_cash -= 100;
     if (ch == 'l' && initial_cash < 99900)
      initial_cash += 100;
     mvwprintz(w, 13, 20, c_black, "xxxxx");
     mvwprintz(w, 13, NUMALIGN(initial_cash), c_yellow, "%d", initial_cash);
     break;

    case 8:
     if (ch == 'h' && cash_per_wave > 0)
      cash_per_wave -= 100;
     if (ch == 'l' && cash_per_wave < 9900)
      cash_per_wave += 100;
     mvwprintz(w, 14, 21, c_black, "xxxx");
     mvwprintz(w, 14, NUMALIGN(cash_per_wave), c_yellow, "%d", cash_per_wave);
     break;

    case 9:
     if (ch == 'h' && cash_increase > 0)
      cash_increase -= 50;
     if (ch == 'l' && cash_increase < 9950)
      cash_increase += 50;
     mvwprintz(w, 15, 21, c_black, "xxxx");
     mvwprintz(w, 15, NUMALIGN(cash_increase), c_yellow, "%d", cash_increase);
     break;

    case 10:
     if (ch == ' ' || ch == '\n') {
      zombies = !zombies;
      specials = false;
     }
     mvwprintz(w, 18, 2, (zombies ? c_ltgreen : c_yellow), "Zombies");
     mvwprintz(w, 18, 14, c_yellow, "Special Zombies");
     break;

    case 11:
     if (ch == ' ' || ch == '\n') {
      specials = !specials;
      zombies = false;
     }
     mvwprintz(w, 18, 2, c_yellow, "Zombies");
     mvwprintz(w, 18, 14, (specials ? c_ltgreen : c_yellow), "Special Zombies");
     break;

    case 12:
     if (ch == ' ' || ch == '\n')
      spiders = !spiders;
     mvwprintz(w, 18, 34, (spiders ? c_ltgreen : c_yellow), "Spiders");
     break;

    case 13:
     if (ch == ' ' || ch == '\n')
      triffids = !triffids;
     mvwprintz(w, 18, 46, (triffids ? c_ltgreen : c_yellow), "Triffids");
     break;

    case 14:
     if (ch == ' ' || ch == '\n')
      robots = !robots;
     mvwprintz(w, 18, 59, (robots ? c_ltgreen : c_yellow), "Robots");
     break;

    case 15:
     if (ch == ' ' || ch == '\n')
      subspace = !subspace;
     mvwprintz(w, 18, 70, (subspace ? c_ltgreen : c_yellow), "Subspace");
     break;

    case 16:
     if (ch == ' ' || ch == '\n')
      hunger = !hunger;
     mvwprintz(w, 21, 2, (hunger ? c_ltgreen : c_yellow), "Food");
     break;

    case 17:
     if (ch == ' ' || ch == '\n')
      thirst = !thirst;
     mvwprintz(w, 21, 16, (thirst ? c_ltgreen : c_yellow), "Water");
     break;

    case 18:
     if (ch == ' ' || ch == '\n')
      sleep = !sleep;
     mvwprintz(w, 21, 31, (sleep ? c_ltgreen : c_yellow), "Sleep");
     break;

    case 19:
     if (ch == ' ' || ch == '\n')
      mercenaries = !mercenaries;
     mvwprintz(w, 21, 46, (mercenaries ? c_ltgreen : c_yellow), "Mercenaries");
     break;
   }
  }
  if (ch == 'h' || ch == 'l' || ch == ' ' || ch == '\n')
   refresh_setup(w, selection);
 }
}

void defense_game::refresh_setup(WINDOW* w, int selection)
{
 werase(w);
 mvwprintz(w,  0,  1, c_ltred, "DEFENSE MODE");
 mvwprintz(w,  0, 28, c_ltred, "Press +/- or >/< to cycle, spacebar to toggle");
 mvwprintz(w,  1, 28, c_ltred, "Press S to start, ! to save as a template");
 mvwprintz(w,  2,  2, c_ltgray, "Scenario:");
 mvwprintz(w,  3,  2, SELCOL(1), defense_style_name(style).c_str());
 mvwprintz(w,  3, 28, c_ltgray, defense_style_description(style).c_str());
 mvwprintz(w,  4,  2, c_ltgray, "Location:");
 mvwprintz(w,  5,  2, SELCOL(2), defense_location_name(location).c_str());
 mvwprintz(w,  5, 28, c_ltgray, defense_location_description(location).c_str());

 mvwprintz(w,  7,  2, c_ltgray, "Initial Difficulty:");
 mvwprintz(w,  7, NUMALIGN(initial_difficulty), SELCOL(3), "%d",
           initial_difficulty);
 mvwprintz(w,  7, 28, c_ltgray, "The difficulty of the first wave.");
 mvwprintz(w,  8,  2, c_ltgray, "Wave Difficulty:");
 mvwprintz(w,  8, NUMALIGN(wave_difficulty), SELCOL(4), "%d", wave_difficulty);
 mvwprintz(w,  8, 28, c_ltgray, "The increase of difficulty with each wave.");

 mvwprintz(w, 10,  2, c_ltgray, "Time b/w Waves:");
 mvwprintz(w, 10, NUMALIGN(time_between_waves), SELCOL(5), "%d",
           time_between_waves);
 mvwprintz(w, 10, 28, c_ltgray, "The time, in minutes, between waves.");
 mvwprintz(w, 11,  2, c_ltgray, "Waves b/w Caravans:");
 mvwprintz(w, 11, NUMALIGN(waves_between_caravans), SELCOL(6), "%d",
           waves_between_caravans);
 mvwprintz(w, 11, 28, c_ltgray, "The number of waves in between caravans.");

 mvwprintz(w, 13,  2, c_ltgray, "Initial Cash:");
 mvwprintz(w, 13, NUMALIGN(initial_cash), SELCOL(7), "%d", initial_cash);
 mvwprintz(w, 13, 28, c_ltgray, "The amount of money the player starts with.");
 mvwprintz(w, 14,  2, c_ltgray, "Cash for 1st Wave:");
 mvwprintz(w, 14, NUMALIGN(cash_per_wave), SELCOL(8), "%d", cash_per_wave);
 mvwprintz(w, 14, 28, c_ltgray, "The cash awarded for the first wave.");
 mvwprintz(w, 15,  2, c_ltgray, "Cash Increase:");
 mvwprintz(w, 15, NUMALIGN(cash_increase), SELCOL(9), "%d", cash_increase);
 mvwprintz(w, 15, 28, c_ltgray, "The increase in the award each wave.");

 mvwprintz(w, 17,  2, c_ltgray, "Enemy Selection:");
 mvwprintz(w, 18,  2, TOGCOL(10, zombies), "Zombies");
 mvwprintz(w, 18, 14, TOGCOL(11, specials), "Special Zombies");
 mvwprintz(w, 18, 34, TOGCOL(12, spiders), "Spiders");
 mvwprintz(w, 18, 46, TOGCOL(13, triffids), "Triffids");
 mvwprintz(w, 18, 59, TOGCOL(14, robots), "Robots");
 mvwprintz(w, 18, 70, TOGCOL(15, subspace), "Subspace");

 mvwprintz(w, 20,  2, c_ltgray, "Needs:");
 mvwprintz(w, 21,  2, TOGCOL(16, hunger), "Food");
 mvwprintz(w, 21, 16, TOGCOL(17, thirst), "Water");
 mvwprintz(w, 21, 31, TOGCOL(18, sleep), "Sleep");
 mvwprintz(w, 21, 46, TOGCOL(19, mercenaries), "Mercenaries");
 wrefresh(w);
}

std::string defense_style_name(defense_style style)
{
// 24 Characters Max!
 switch (style) {
  case DEFENSE_CUSTOM:		return "Custom";
  case DEFENSE_EASY:		return "Easy";
  case DEFENSE_MEDIUM:		return "Medium";
  case DEFENSE_HARD:		return "Hard";
  case DEFENSE_SHAUN:		return "Shaun of the Dead";
  case DEFENSE_DAWN:		return "Dawn of the Dead";
  case DEFENSE_SPIDERS:		return "Eight-Legged Freaks";
  case DEFENSE_TRIFFIDS:	return "Day of the Triffids";
  case DEFENSE_SKYNET:		return "Skynet";
  case DEFENSE_LOVECRAFT:	return "The Call of Cthulhu";
  default:			return "Bug! (bug in defense.cpp:defense_style_name)";
 }
}

std::string defense_style_description(defense_style style)
{
// 51 Characters Max!
 switch (style) {
  case DEFENSE_CUSTOM:
   return "A custom game.";
  case DEFENSE_EASY:
   return "Easy monsters and lots of money.";
  case DEFENSE_MEDIUM:
   return "Harder monsters.  You have to eat.";
  case DEFENSE_HARD:
   return "All monsters.  You have to eat and drink.";
  case DEFENSE_SHAUN:
   return "Defend a bar against classic zombies.  Easy and fun.";
  case DEFENSE_DAWN:
   return "Classic zombies.  Slower and more realistic.";
  case DEFENSE_SPIDERS:
   return "Fast-paced spider-fighting fun!";
  case DEFENSE_TRIFFIDS:
   return "Defend your mansion against the triffids.";
  case DEFENSE_SKYNET:
   return "The robots have decided that humans are the enemy!";
  case DEFENSE_LOVECRAFT:
   return "Ward off legions of eldritch horrors.";
  default:
   return "What the heck is this I don't even know. (defense.cpp:defense_style_description)";
 }
}

std::string defense_location_name(defense_location location)
{
 switch (location) {
 case DEFLOC_NULL:	return "Nowhere?! (bug in defense.cpp:defense_location_name)";
 case DEFLOC_HOSPITAL:	return "Hospital";
 case DEFLOC_MALL:	return "Megastore";
 case DEFLOC_BAR:	return "Bar";
 case DEFLOC_MANSION:	return "Mansion";
 default:		return "a ghost's house (bug in defense.cpp:defense_location_name)";
 }
}

std::string defense_location_description(defense_location location)
{
 switch (location) {
 case DEFLOC_NULL:
  return "NULL Bug. (defense.cpp:defense_location_description)";
 case DEFLOC_HOSPITAL:
  return                 "One entrance and many rooms.  Some medical supplies.";
 case DEFLOC_MALL:
  return                 "A large building with various supplies.";
 case DEFLOC_BAR:
  return                 "A small building with plenty of alcohol.";
 case DEFLOC_MANSION:
  return                 "A large house with many rooms and.";
 default:
  return "Unknown data bug. (defense.cpp:defense_location_description)";
 }
}

void defense_game::caravan(game *g)
{
 std::vector<itype_id> items[NUM_CARAVAN_CATEGORIES];
 std::vector<int> item_count[NUM_CARAVAN_CATEGORIES];

// Init the items for each category
 for (int i = 0; i < NUM_CARAVAN_CATEGORIES; i++) {
  items[i] = caravan_items( caravan_category(i) );
  for (int j = 0; j < items[i].size(); j++) {
   if (current_wave == 0 || !one_in(4))
    item_count[i].push_back(0); // Init counts to 0 for each item
   else { // Remove the item
    items[i].erase( items[i].begin() + j);
    j--;
   }
  }
 }

 int total_price = 0;

 WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, 0, 0);

 int offset = 0, item_selected = 0, category_selected = 0;

 int current_window = 0;

 draw_caravan_borders(w, current_window);
 draw_caravan_categories(w, category_selected, total_price, g->u.cash);

 bool done = false;
 bool cancel = false;
 while (!done) {

  char ch = input();
  switch (ch) {
   case '?':
    popup_top("\
CARAVAN:\n\
Start by selecting a category using your favorite up/down keys.\n\
Switch between category selection and item selecting by pressing Tab.\n\
Pick an item with the up/down keys, press + to buy 1 more, - to buy 1 less.\n\
Press Enter to buy everything in your cart, Esc to buy nothing.");
    draw_caravan_categories(w, category_selected, total_price, g->u.cash);
    draw_caravan_items(w, g, &(items[category_selected]),
                       &(item_count[category_selected]), offset, item_selected);
    draw_caravan_borders(w, current_window);
    break;

   case 'j':
    if (current_window == 0) { // Categories
     category_selected++;
     if (category_selected == NUM_CARAVAN_CATEGORIES)
      category_selected = CARAVAN_CART;
     draw_caravan_categories(w, category_selected, total_price, g->u.cash);
     offset = 0;
     item_selected = 0;
     draw_caravan_items(w, g, &(items[category_selected]),
                        &(item_count[category_selected]), offset,
                        item_selected);
     draw_caravan_borders(w, current_window);
    } else if (items[category_selected].size() > 0) { // Items
     if (item_selected < items[category_selected].size() - 1)
      item_selected++;
     else {
      item_selected = 0;
      offset = 0;
     }
     if (item_selected > offset + 22)
      offset++;
     draw_caravan_items(w, g, &(items[category_selected]),
                        &(item_count[category_selected]), offset,
                        item_selected);
     draw_caravan_borders(w, current_window);
    }
    break;

   case 'k':
    if (current_window == 0) { // Categories
     if (category_selected == 0)
      category_selected = NUM_CARAVAN_CATEGORIES - 1;
     else
      category_selected--;
     if (category_selected == NUM_CARAVAN_CATEGORIES)
      category_selected = CARAVAN_CART;
     draw_caravan_categories(w, category_selected, total_price, g->u.cash);
     offset = 0;
     item_selected = 0;
     draw_caravan_items(w, g, &(items[category_selected]),
                        &(item_count[category_selected]), offset,
                        item_selected);
     draw_caravan_borders(w, current_window);
    } else if (items[category_selected].size() > 0) { // Items
     if (item_selected > 0)
      item_selected--;
     else {
      item_selected = items[category_selected].size() - 1;
      offset = item_selected - 22;
      if (offset < 0)
       offset = 0;
     }
     if (item_selected < offset)
      offset--;
     draw_caravan_items(w, g, &(items[category_selected]),
                        &(item_count[category_selected]), offset,
                        item_selected);
     draw_caravan_borders(w, current_window);
    }
    break;

   case '+':
   case 'l':
    if (current_window == 1 && items[category_selected].size() > 0) {
     item_count[category_selected][item_selected]++;
     itype_id tmp_itm = items[category_selected][item_selected];
     total_price += caravan_price(g->u, g->itypes[tmp_itm]->price);
     if (category_selected == CARAVAN_CART) { // Find the item in its category
      for (int i = 1; i < NUM_CARAVAN_CATEGORIES; i++) {
       for (int j = 0; j < items[i].size(); j++) {
        if (items[i][j] == tmp_itm)
         item_count[i][j]++;
       }
      }
     } else { // Add / increase the item in the shopping cart
      bool found_item = false;
      for (int i = 0; i < items[0].size() && !found_item; i++) {
       if (items[0][i] == tmp_itm) {
        found_item = true;
        item_count[0][i]++;
       }
      }
      if (!found_item) {
       items[0].push_back(items[category_selected][item_selected]);
       item_count[0].push_back(1);
      }
     }
     draw_caravan_categories(w, category_selected, total_price, g->u.cash);
     draw_caravan_items(w, g, &(items[category_selected]),
                       &(item_count[category_selected]), offset, item_selected);
     draw_caravan_borders(w, current_window);
    }
    break;

   case '-':
   case 'h':
    if (current_window == 1 && items[category_selected].size() > 0 &&
        item_count[category_selected][item_selected] > 0) {
     item_count[category_selected][item_selected]--;
     itype_id tmp_itm = items[category_selected][item_selected];
     total_price -= caravan_price(g->u, g->itypes[tmp_itm]->price);
     if (category_selected == CARAVAN_CART) { // Find the item in its category
      for (int i = 1; i < NUM_CARAVAN_CATEGORIES; i++) {
       for (int j = 0; j < items[i].size(); j++) {
        if (items[i][j] == tmp_itm)
         item_count[i][j]--;
       }
      }
     } else { // Decrease / remove the item in the shopping cart
      bool found_item = false;
      for (int i = 0; i < items[0].size() && !found_item; i++) {
       if (items[0][i] == tmp_itm) {
        found_item = true;
        item_count[0][i]--;
        if (item_count[0][i] == 0) {
         item_count[0].erase(item_count[0].begin() + i);
         items[0].erase(items[0].begin() + i);
        }
       }
      }
     }
     draw_caravan_categories(w, category_selected, total_price, g->u.cash);
     draw_caravan_items(w, g, &(items[category_selected]),
                       &(item_count[category_selected]), offset, item_selected);
     draw_caravan_borders(w, current_window);
    }
    break;

   case '\t':
    current_window = (current_window + 1) % 2;
    draw_caravan_borders(w, current_window);
    break;

   case KEY_ESCAPE:
    if (query_yn("Really buy nothing?")) {
     cancel = true;
     done = true;
    } else {
     draw_caravan_categories(w, category_selected, total_price, g->u.cash);
     draw_caravan_items(w, g, &(items[category_selected]),
                       &(item_count[category_selected]), offset, item_selected);
     draw_caravan_borders(w, current_window);
    }
    break;

   case '\n':
    if (total_price > g->u.cash)
     popup("You can't afford those items!");
    else if ((items[0].empty() && query_yn("Really buy nothing?")) ||
             (!items[0].empty() &&
              query_yn("Buy %d items, leaving you with $%d?", items[0].size(),
                       g->u.cash - total_price)))
     done = true;
    if (!done) { // We canceled, so redraw everything
     draw_caravan_categories(w, category_selected, total_price, g->u.cash);
     draw_caravan_items(w, g, &(items[category_selected]),
                       &(item_count[category_selected]), offset, item_selected);
     draw_caravan_borders(w, current_window);
    }
    break;
  } // switch (ch)

 } // while (!done)

 if (!cancel) {
  g->u.cash -= total_price;
  bool dropped_some = false;
  for (int i = 0; i < items[0].size(); i++) {
   item tmp(g->itypes[ items[0][i] ], g->turn);
   tmp = tmp.in_its_container(&(g->itypes));
   for (int j = 0; j < item_count[0][i]; j++) {
    if (g->u.volume_carried() + tmp.volume() <= g->u.volume_capacity() &&
        g->u.weight_carried() + tmp.weight() <= g->u.weight_capacity() &&
        g->u.inv.size() < inv_chars.size())
     g->u.i_add(tmp);
    else { // Could fit it in the inventory!
     dropped_some = true;
     g->m.add_item(g->u.posx, g->u.posy, tmp);
    }
   }
  }
  if (dropped_some)
   g->add_msg("You drop some items.");
 }
}

std::string caravan_category_name(caravan_category cat)
{
 switch (cat) {
  case CARAVAN_CART:		return "Shopping Cart";
  case CARAVAN_MELEE:		return "Melee Weapons";
  case CARAVAN_GUNS:		return "Firearms & Ammo";
  case CARAVAN_COMPONENTS:	return "Crafting & Construction Components";
  case CARAVAN_FOOD:		return "Food & Drugs";
  case CARAVAN_CLOTHES:		return "Clothing & Armor";
  case CARAVAN_TOOLS:		return "Tools, Traps & Grenades";
 }
 return "BUG (defense.cpp:caravan_category_name)";
}

std::vector<itype_id> caravan_items(caravan_category cat)
{
 std::vector<itype_id> ret;
 switch (cat) {
 case CARAVAN_CART:
  return ret;

 case CARAVAN_MELEE:
  setvector(&ret,
"hammer", "bat", "mace", "morningstar", "hammer_sledge", "hatchet",
"knife_combat", "rapier", "machete", "katana", "spear_knife",
"pike", "chainsaw_off", NULL);
  break;

 case CARAVAN_GUNS:
  setvector(&ret,
"crossbow", "bolt_steel", "compbow", "arrow_cf", "marlin_9a",
"22_lr", "hk_mp5", "9mm", "taurus_38", "38_special", "deagle_44",
"44magnum", "m1911", "hk_ump45", "45_acp", "fn_p90", "57mm",
"remington_870", "shot_00", "shot_slug", "browning_blr", "3006",
"ak47", "762_m87", "m4a1", "556", "savage_111f", "hk_g3",
"762_51", "hk_g80", "12mm", "plasma_rifle", "plasma", NULL);
  break;

 case CARAVAN_COMPONENTS:
  setvector(&ret,
"rag", "fur", "leather", "superglue", "string_36", "chain",
"processor", "RAM", "power_supply", "motor", "hose", "pot",
"2x4", "battery", "nail", "gasoline", NULL);
  break;

 case CARAVAN_FOOD:
  setvector(&ret,
"1st_aid", "water", "energy_drink", "whiskey", "can_beans",
"mre_beef", "flour", "inhaler", "codeine", "oxycodone", "adderall",
"cig", "meth", "royal_jelly", "mutagen", "purifier", NULL);
 break;

 case CARAVAN_CLOTHES:
  setvector(&ret,
"backpack", "vest", "trenchcoat", "jacket_leather", "kevlar",
"gloves_fingerless", "mask_filter", "mask_gas", "glasses_eye",
"glasses_safety", "goggles_ski", "goggles_nv", "helmet_ball",
"helmet_riot", NULL);
  break;

 case CARAVAN_TOOLS:
  setvector(&ret,
"screwdriver", "wrench", "saw", "hacksaw", "lighter", "sewing_kit",
"scissors", "extinguisher", "flashlight", "hotplate",
"soldering_iron", "shovel", "jackhammer", "landmine", "teleporter",
"grenade", "flashbang", "EMPbomb", "smokebomb", "bot_manhack",
"bot_turret", "UPS_off", "mininuke", NULL);
  break;
 }

 return ret;
}

void draw_caravan_borders(WINDOW *w, int current_window)
{
// First, do the borders for the category window
 nc_color col = c_ltgray;
 if (current_window == 0)
  col = c_yellow;

 mvwputch(w, 0, 0, col, LINE_OXXO);
 for (int i = 1; i <= 38; i++) {
  mvwputch(w,  0, i, col, LINE_OXOX);
  mvwputch(w, 11, i, col, LINE_OXOX);
 }
 for (int i = 1; i <= 10; i++) {
  mvwputch(w, i,  0, col, LINE_XOXO);
  mvwputch(w, i, 39, c_yellow, LINE_XOXO); // Shared border, always yellow
 }
 mvwputch(w, 11,  0, col, LINE_XXXO);

// These are shared with the items window, and so are always "on"
 mvwputch(w,  0, 39, c_yellow, LINE_OXXX);
 mvwputch(w, 11, 39, c_yellow, LINE_XOXX);

 col = (current_window == 1 ? c_yellow : c_ltgray);
// Next, draw the borders for the item description window--always "off" & gray
 for (int i = 12; i <= 23; i++) {
  mvwputch(w, i,  0, c_ltgray, LINE_XOXO);
  mvwputch(w, i, 39, col,      LINE_XOXO);
 }
 for (int i = 1; i <= 38; i++)
  mvwputch(w, FULL_SCREEN_HEIGHT-1, i, c_ltgray, LINE_OXOX);

 mvwputch(w, FULL_SCREEN_HEIGHT-1,  0, c_ltgray, LINE_XXOO);
 mvwputch(w, FULL_SCREEN_HEIGHT-1, 39, c_ltgray, LINE_XXOX);

// Finally, draw the item section borders
 for (int i = 40; i <= FULL_SCREEN_WIDTH-2; i++) {
  mvwputch(w,  0, i, col, LINE_OXOX);
  mvwputch(w, FULL_SCREEN_HEIGHT-1, i, col, LINE_OXOX);
 }
 for (int i = 1; i <= FULL_SCREEN_HEIGHT-2; i++)
  mvwputch(w, i, FULL_SCREEN_WIDTH-1, col, LINE_XOXO);

 mvwputch(w, FULL_SCREEN_HEIGHT-1, 39, col, LINE_XXOX);
 mvwputch(w,  0, FULL_SCREEN_WIDTH-1, col, LINE_OOXX);
 mvwputch(w, FULL_SCREEN_HEIGHT-1, FULL_SCREEN_WIDTH-1, col, LINE_XOOX);

// Quick reminded about help.
 mvwprintz(w, FULL_SCREEN_HEIGHT-1, 2, c_red, "Press ? for help.");
 wrefresh(w);
}

void draw_caravan_categories(WINDOW *w, int category_selected, int total_price,
                             int cash)
{
// Clear the window
 for (int i = 1; i <= 10; i++)
  mvwprintz(w, i, 1, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
 mvwprintz(w, 1, 1, c_white, "Your Cash:%s%d",
           (cash >= 10000 ? " " : (cash >= 1000 ? "  " : (cash >= 100 ? "   " :
            (cash >= 10 ? "    " : "     ")))), cash);
 wprintz(w, c_ltgray, " -> ");
 wprintz(w, (total_price > cash ? c_red : c_green), "%d", cash - total_price);

 for (int i = 0; i < NUM_CARAVAN_CATEGORIES; i++)
  mvwprintz(w, i + 3, 1, (i == category_selected ? h_white : c_white),
            caravan_category_name( caravan_category(i) ).c_str());
 wrefresh(w);
}

void draw_caravan_items(WINDOW *w, game *g, std::vector<itype_id> *items,
                        std::vector<int> *counts, int offset,
                        int item_selected)
{
// Print the item info first.  This is important, because it contains \n which
// will corrupt the item list.

// Actually, clear the item info first.
 for (int i = 12; i <= FULL_SCREEN_HEIGHT-2; i++)
  mvwprintz(w, i, 1, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
// THEN print it--if item_selected is valid
 if (item_selected < items->size()) {
  item tmp(g->itypes[ (*items)[item_selected] ], 0); // Dummy item to get info
  mvwprintz(w, 12, 0, c_white, tmp.info().c_str());
 }
// Next, clear the item list on the right
 for (int i = 1; i <= FULL_SCREEN_HEIGHT-2; i++)
  mvwprintz(w, i, 40, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
// Finally, print the item list on the right
 for (int i = offset; i <= offset + FULL_SCREEN_HEIGHT-2 && i < items->size(); i++) {
  mvwprintz(w, i - offset + 1, 40, (item_selected == i ? h_white : c_white),
            g->itypes[ (*items)[i] ]->name.c_str());
  wprintz(w, c_white, " x %s%d", ((*counts)[i] >= 10 ? "" : " "), (*counts)[i]);
  if ((*counts)[i] > 0) {
   int price = caravan_price(g->u, g->itypes[(*items)[i]]->price *(*counts)[i]);
   wprintz(w, (price > g->u.cash ? c_red : c_green),
              "($%s%d)", (price >= 100000 ? "" : (price >= 10000 ? " " :
                          (price >= 1000 ? "  " : (price >= 100 ? "   " :
                           (price >= 10 ? "    " : "     "))))), price);
  }
 }
 wrefresh(w);
}

int caravan_price(player &u, int price) {
  if (u.skillLevel("barter") > 10)
   return int( double(price) * .5);
  return int( double(price) * (1.0 - double(u.skillLevel("barter")) * .05));
}

void defense_game::spawn_wave(game *g)
{
 g->add_msg("********");
 int diff = initial_difficulty + current_wave * wave_difficulty;
 bool themed_wave = one_in(SPECIAL_WAVE_CHANCE); // All a single monster type
 g->u.cash += cash_per_wave + (current_wave - 1) * cash_increase;
 std::vector<mon_id> valid;
 valid = pick_monster_wave(g);
 while (diff > 0) {
// Clear out any monsters that exceed our remaining difficulty
  for (int i = 0; i < valid.size(); i++) {
   if (g->mtypes[valid[i]]->difficulty > diff) {
    valid.erase(valid.begin() + i);
    i--;
   }
  }
  if (valid.size() == 0) {
   g->add_msg("Welcome to Wave %d!", current_wave);
   g->add_msg("********");
   return;
  }
  int rn = rng(0, valid.size() - 1);
  mtype *type = g->mtypes[valid[rn]];
  if (themed_wave) {
   int num = diff / type->difficulty;
   if (num >= SPECIAL_WAVE_MIN) {
// TODO: Do we want a special message here?
    for (int i = 0; i < num; i++)
     spawn_wave_monster(g, type);
    g->add_msg( special_wave_message(type->name).c_str() );
    g->add_msg("********");
    return;
   } else
    themed_wave = false; // No partially-themed waves
  }
  diff -= type->difficulty;
  spawn_wave_monster(g, type);
 }
 g->add_msg("Welcome to Wave %d!", current_wave);
 g->add_msg("********");
}

std::vector<mon_id> defense_game::pick_monster_wave(game *g)
{
 std::vector<std::string> valid;
 std::vector<mon_id> ret;

 if (zombies || specials) {
  if (specials)
   valid.push_back("GROUP_ZOMBIE");
  else
   valid.push_back("GROUP_VANILLA");
 }
 if (spiders)
  valid.push_back("GROUP_SPIDER");
 if (triffids)
  valid.push_back("GROUP_TRIFFID");
 if (robots)
  valid.push_back("GROUP_ROBOT");
 if (subspace)
  valid.push_back("GROUP_NETHER");

 if (valid.empty())
  debugmsg("Couldn't find a valid monster group for defense!");
 else
  ret = MonsterGroupManager::GetMonstersFromGroup(valid[rng(0, valid.size() - 1)]);

 return ret;
}

void defense_game::spawn_wave_monster(game *g, mtype *type)
{
 monster tmp(type);
 if (location == DEFLOC_HOSPITAL || location == DEFLOC_MALL) {
  tmp.posy = SEEY; // Always spawn to the north!
  tmp.posx = rng(SEEX * (MAPSIZE / 2), SEEX * (1 + MAPSIZE / 2));
 } else if (one_in(2)) {
  tmp.spawn(rng(SEEX * (MAPSIZE / 2), SEEX * (1 + MAPSIZE / 2)), rng(1, SEEY));
  if (one_in(2))
   tmp.posy = SEEY * MAPSIZE - 1 - tmp.posy;
 } else {
  tmp.spawn(rng(1, SEEX), rng(SEEY * (MAPSIZE / 2), SEEY * (1 + MAPSIZE / 2)));
  if (one_in(2))
   tmp.posx = SEEX * MAPSIZE - 1 - tmp.posx;
 }
 tmp.wandx = g->u.posx;
 tmp.wandy = g->u.posy;
 tmp.wandf = 150;
// We wanna kill!
 tmp.anger = 100;
 tmp.morale = 100;
 g->z.push_back(tmp);
}

std::string defense_game::special_wave_message(std::string name)
{
 std::stringstream ret;
 ret << "Wave " << current_wave << ": ";
 name[0] += 'A' - 'a'; // Capitalize

 for (int i = 2; i < name.size(); i++) {
  if (name[i - 1] == ' ')
   name[i] += 'A' - 'a';
 }

 switch (rng(1, 6)) {
  case 1: ret << name << " Invasion!"; break;
  case 2: ret << "Attack of the " << name << "s!"; break;
  case 3: ret << name << " Attack!"; break;
  case 4: ret << name << "s from Hell!"; break;
  case 5: ret << "Beware! " << name << "!"; break;
  case 6: ret << "The Day of the " << name << "!"; break;
  case 7: ret << name << " Party!"; break;
  case 8: ret << "Revenge of the " << name << "s!"; break;
  case 9: ret << "Rise of the " << name << "s!"; break;
 }

 return ret.str();
}
