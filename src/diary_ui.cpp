#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <ncurses_def.cpp>

//#include "avatar.h"
//#include "calendar.h"
#include "color.h"
#include "debug.h"
#include "input.h"
//#include "mission.h"
//#include "npc.h"
#include "output.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "cursesdef.h"

#include "diary.h"


namespace {
    /**print list scrollable, printed std::vector<std::string> as list with scroll*/
    void print_list_scrollable(catacurses::window* win, std::vector<std::string> list, int* selection, int entries_per_page, int xoffset, int width, bool active, bool border) {
        if (*selection < 0 && !list.empty()) {
            *selection = list.size() - 1;
        }
        else if (*selection >= list.size()) {
            *selection = 0;
        }
        const int borderspace = border ? 1 : 0;
        entries_per_page = entries_per_page - borderspace * 2;

        //const int top_of_page = *selection <= entries_per_page ? 0 : *selection - entries_per_page;
        const int top_of_page = entries_per_page * (*selection / entries_per_page);

        const int bottom_of_page =
            std::min(top_of_page + entries_per_page, static_cast<int>(list.size()));

        for (int i = top_of_page; i < bottom_of_page; i++) {

            const nc_color col = c_white; //c_light_green
            const int y = i - top_of_page;
            trim_and_print(*win, point(xoffset + 1, y + borderspace), width - 1 - borderspace,
                (static_cast<int>(*selection) == i && active) ? hilite(col) : col,
                (static_cast<int>(*selection) == i && active) ? remove_color_tags(list[i]) : list[i]);
        }
        if (border) draw_border(*win);
        if (active && entries_per_page < list.size()) draw_scrollbar(*win, *selection, entries_per_page, list.size(), point(xoffset, 0 + borderspace));

    };
    void print_list_scrollable(catacurses::window* win, std::vector<std::string> list, int* selection, bool active, bool border) {

        print_list_scrollable(win, list, selection, getmaxy(*win), 0, getmaxx(*win), active, border);
    }
    void print_list_scrollable(catacurses::window* win, std::string text, int* selection, int entries_per_page, int xoffset, int width, bool active, bool border) {
        int borderspace = border ? 1 : 0;
        std::vector<std::string> list = foldstring(text, width - 1 - borderspace * 2);


        print_list_scrollable(win, list, selection, entries_per_page, xoffset, width, active, border);
    }
    void print_list_scrollable(catacurses::window* win, std::string text, int* selection, bool active, bool border) {

        print_list_scrollable(win, text, selection, getmaxy(*win), 0, getmaxx(*win), active, border);
    }

    void draw_diary_border(catacurses::window* win, const nc_color& color = c_white) {
        wattron(*win, color);
        //mvwprintw(const window & win, const point & p, const std::string & text)
        const int maxx = getmaxx(*win) - 1;
        const int maxy = getmaxy(*win) - 1;
        //
        const int midx = maxx % 2 == 0 ? maxx / 2 : maxx / 2 - 1;
        for (int i = 4; i <= maxy - 4; i++) {
            mvwprintw(*win, point(0, i), "||||");
            mvwprintw(*win, point(maxx - 3, i), "||||");
            mvwprintw(*win, point(midx, i), " | ");
        }
        for (int i = 4; i <= maxx - 4; i++) {
            if (!(i >= midx && i <= midx + 2)) {
                mvwprintw(*win, point(i, 0), "____");
                mvwprintw(*win, point(i, maxy - 2), "____");
                mvwprintw(*win, point(i, maxy - 1), "====");
                mvwprintw(*win, point(i, maxy), "----");
            }
        }
        //top left corner
        mvwprintw(*win, point(0, 0), "    ");
        mvwprintw(*win, point(0, 1), ".-/|");
        mvwprintw(*win, point(0, 2), "||||");
        mvwprintw(*win, point(0, 3), "||||");
        //bottom left corner
        mvwprintw(*win, point(0, maxy - 3), "||||");
        mvwprintw(*win, point(0, maxy - 2), "||||");
        mvwprintw(*win, point(0, maxy - 1), "||/=");
        mvwprintw(*win, point(0, maxy - 0), "`'--");
        //top right corner
        mvwprintw(*win, point(maxx - 3, 0), "    ");
        mvwprintw(*win, point(maxx - 3, 1), "|\\-.");
        mvwprintw(*win, point(maxx - 3, 2), "||||");
        mvwprintw(*win, point(maxx - 3, 3), "||||");
        //bottom right corner
        mvwprintw(*win, point(maxx - 3, maxy - 3), "||||");
        mvwprintw(*win, point(maxx - 3, maxy - 2), "||||");
        mvwprintw(*win, point(maxx - 3, maxy - 1), "=\\||");
        mvwprintw(*win, point(maxx - 3, maxy - 0), "--''");

        //mid top
        mvwprintw(*win, point(midx, 0), "   ");
        mvwprintw(*win, point(midx, 1), "\\ /");
        mvwprintw(*win, point(midx, 2), " | ");
        mvwprintw(*win, point(midx, 3), " | ");
        //mid bottom
        mvwprintw(*win, point(midx, maxy - 3), " | ");
        mvwprintw(*win, point(midx, maxy - 2), " | ");
        mvwprintw(*win, point(midx, maxy - 1), "\\|/");
        mvwprintw(*win, point(midx, maxy - 0), "___");
        wattroff(*win, color);
    }

   

    //std::pair<int, int> get_line_and_position(std::vector<std::string> foldedtext, int position, int maxx) {
    //    //beim n�chsten mal builden und dann weiter sehen.
    //    int counter = 0;
    //    int lineposition;
    //    int line;
    //    for (int i = 0; i < foldedtext.size(); i++) {
    //        int temp = foldedtext[i].size();
    //        temp += (foldedtext[i].back() != ' ') ? 1 : 0;
    //        if (counter + temp > position) {
    //            lineposition = position - counter;
    //            line = i;
    //            return std::make_pair(line,lineposition);
    //        }
    //        else {
    //            counter += temp;
    //        }
    //    }
    //    return std::make_pair(0, 0);
    //}

    //void print_editor(catacurses::window& win, std::string text, int position) {
    //    
    //    const int maxx = getmaxx(win);
    //    const int maxy = getmaxy(win);
    //    const int middelofpage = maxy / 2;
    //    const auto foldettext = foldstring(text, maxx);
    //    if (position >= text.size()) position = 0;
    //    if (position < 0) position = text.size() - 1;
    //    auto line_position = get_line_and_position(foldettext, position, maxx);
    //    
    //    int topoflist=0;
    //    int bottomoflist = std::min(topoflist + maxy, static_cast<int>(foldettext.size()));
    //    if (maxy <= foldettext.size()) {
    //        if (line_position.first > middelofpage) {
    //            topoflist = line_position.first - middelofpage;
    //            bottomoflist = topoflist + maxy;
    //        }
    //        if (line_position.first + middelofpage >= foldettext.size()) {
    //            bottomoflist = static_cast<int>(foldettext.size());
    //            topoflist = bottomoflist - maxy;
    //        }
    //    }
    //    
    //    //int topoflist = std::max(line_position.first - middelofpage, 0);
    //    ////if (topoflist + maxy >= foldettext.size()) topoflist = foldettext.size() - maxy-1;
    //    //const auto bottomoflist = std::min(topoflist + maxy, static_cast<int>(foldettext.size()));

    //    for (int i = topoflist; i < bottomoflist; i++) {
    //        int y = i - topoflist;
    //        trim_and_print(win, point(0, y), maxx, c_white, foldettext[i]);
    //        if (i == line_position.first) {
    //            wattron(win, h_white);
    //            mvwprintw(win, point(line_position.second, y), std::string(1, text[position]));
    //            wattroff(win, h_white);
    //        }
    //    }
    //    
    //}
}

class editor_window {
private:
    catacurses::window _win;
    int _maxx;
    int _maxy;
    std::string _text;

    nc_color _string_color = c_magenta;
    nc_color _cursor_color = h_light_gray;

    bool _ignore_custom_actions = true;

    int _position = -1;

    std::unique_ptr<input_context> ctxt_ptr;
    input_context* ctxt = nullptr;

    bool _canceled = false;
    bool _confirmed = false;
    bool _handled = false;

    std::map<long, std::function<bool()>> callbacks;
public:
    editor_window(catacurses::window& win, std::string text) {
        _text = text;
        _win = win;
        _maxx = getmaxx(win);
        _maxy = getmaxy(win);
    }

    std::pair<int, int> get_line_and_position(std::vector<std::string> foldedtext, int position, int maxx) {
        //hier noch die frage, was ich mir den ���� zeichen mache. 
        int counter = 0;
        int lineposition;
        int line;
        for (int i = 0; i < foldedtext.size(); i++) {
            int temp = foldedtext[i].size();
            temp += (foldedtext[i].back() != ' ') ? 1 : 0;
            if (counter + temp > position) {
                lineposition = position - counter;
                line = i;
                return std::make_pair(line, lineposition);
            }
            else {
                counter += temp;
            }
        }
        return std::make_pair(0, 0);
    }

    void print_editor(catacurses::window& win, std::string text, int position) {

        /*const int maxx = getmaxx(win);
        const int maxy = getmaxy(win);*/
        const int middelofpage = _maxy / 2;
        const auto foldettext = foldstring(text, _maxx-1);
        if (position >= text.size()) position = 0;
        if (position < 0) position = text.size() - 1;
        auto line_position = get_line_and_position(foldettext, position, _maxx-1);

        int topoflist = 0;
        int bottomoflist = std::min(topoflist + _maxy, static_cast<int>(foldettext.size()));
        if (_maxy <= foldettext.size()) {
            if (line_position.first > middelofpage) {
                topoflist = line_position.first - middelofpage;
                bottomoflist = topoflist + _maxy;
            }
            if (line_position.first + middelofpage >= foldettext.size()) {
                bottomoflist = static_cast<int>(foldettext.size());
                topoflist = bottomoflist - _maxy;
            }
        }

        //int topoflist = std::max(line_position.first - middelofpage, 0);
        ////if (topoflist + maxy >= foldettext.size()) topoflist = foldettext.size() - maxy-1;
        //const auto bottomoflist = std::min(topoflist + maxy, static_cast<int>(foldettext.size()));

        for (int i = topoflist; i < bottomoflist; i++) {
            int y = i - topoflist;
            trim_and_print(win, point(1, y), _maxx, c_white, foldettext[i]);
            if (i == line_position.first) {
                wattron(win, h_white);
                mvwprintw(win, point(line_position.second+1, y), std::string(1, text[position]));
                wattroff(win, h_white);
            }
        }
        //hier noch scrollbar. 
    }
    
    
    bool canceled() const {
        return _canceled;
    }
    
    bool confirmed() const {
        return _confirmed;
    }
    
    bool handled() const {
        return _handled;
    }

    void create_context()
    {
        ctxt_ptr = std::make_unique<input_context>("STRING_INPUT", keyboard_mode::keychar);
        ctxt = ctxt_ptr.get();
        ctxt->register_action("ANY_INPUT");
    }

    const std::string& query_string(const bool loop, const bool draw_only)
    {

        if (!ctxt) {
            create_context();
        }

        utf8_wrapper ret(_text);
        utf8_wrapper edit(ctxt->get_edittext());
        if (_position == -1) {
            _position = ret.length();
        }
        //const int scrmax = std::max(0, _endx - _startx);

        std::unique_ptr<ui_adaptor> ui;


        int ch = 0;

        _canceled = false;
        _confirmed = false;
        do {
            if (_position < 0) {
                _position = 0;
            }



            /*if (ui) {
                ui_manager::redraw();
            }
            else {*/
                werase(_win);
                print_editor(_win, _text, _position);
                wnoutrefresh(_win);
            //}

            if (draw_only) {
                return _text;
            }

            const std::string action = ctxt->handle_input();
            const input_event ev = ctxt->get_raw_input();
            ch = ev.type == input_event_t::keyboard_char ? ev.get_first_input() : 0;
            _handled = true;

            if (callbacks[ch]) {
                if (callbacks[ch]()) {
                    continue;
                }
            }

            if (_ignore_custom_actions && action != "ANY_INPUT") {
                _handled = false;
                continue;
            }

            if (ch == KEY_ESCAPE) {
#if defined(__ANDROID__)
                if (get_option<bool>("ANDROID_AUTO_KEYBOARD")) {
                    SDL_StopTextInput();
                }
#endif
                //_text.clear();
                _position = -1;
                _canceled = true;
                return _text;
            }
            /*else if (ch == '\n') {

            }*/
            else if (ch == KEY_UP) {
                _position -= _maxx;
                if (_position < 0) _position = ret.size();
            }
            else if (ch == KEY_DOWN) {
                _position += _maxx;
                if (_position > static_cast<int>(ret.size())) _position = 0;

            }
            else if (ch == KEY_NPAGE || ch == KEY_PPAGE || ch == KEY_BTAB || ch == '\t') {
                _handled = false;
            }
            else if (ch == KEY_RIGHT) {
                if (_position + 1 <= static_cast<int>(ret.size())) {
                    _position++;
                }
                else {
                    _position = 0;
                }
            }
            else if (ch == KEY_LEFT) {
                if (_position > 0) {
                    _position--;
                }
                else {
                    _position = ret.size();
                }
            }
            else if (ch == 0x15) {                      // ctrl-u: delete all the things
                _position = 0;
                ret.erase(0);
            }
            else if (ch == KEY_BACKSPACE) {
                if (_position > 0 && _position <= static_cast<int>(ret.size())) {
                    _position--;
                    ret.erase(_position, 1);
                }
            }
            else if (ch == KEY_HOME) {
                _position = 0;
            }
            else if (ch == KEY_END) {
                _position = ret.size();
            }
            else if (ch == KEY_DC) {
                if (_position < static_cast<int>(ret.size())) {
                    ret.erase(_position, 1);
                }
            }
            else if (ch == 0x16 || ch == KEY_F(2) || !ev.text.empty()) {
                // ctrl-v, f2, or text input
                // bail out early if already at length limit
                //if (_max_length <= 0 || ret.display_width() < static_cast<size_t>(_max_length)) {
                std::string entered;
                if (ch == 0x16) {
#if defined(TILES)
                    if (edit.empty()) {
                        char* const clip = SDL_GetClipboardText();
                        if (clip) {
                            entered = clip;
                            SDL_free(clip);
                        }
                    }
#endif
                }
                else if (ch == KEY_F(2)) {
                    if (edit.empty()) {
                        entered = get_input_string_from_file();
                    }
                }
                else {
                    entered = ev.text;
                }
                if (!entered.empty()) {
                    utf8_wrapper insertion;
                    const char* str = entered.c_str();
                    int len = entered.length();

                    while (len > 0) {
                        const uint32_t ch = UTF8_getch(&str, &len);
                        if (ch != '\n' && ch != '\r') {


                            insertion.append(utf8_wrapper(utf32_to_utf8(ch)));


                        }
                    }
                    ret.insert(_position, insertion);
                    _position += insertion.length();
                    edit = utf8_wrapper();
                    ctxt->set_edittext(std::string());
                }
                //}
            }
            else if (ev.edit_refresh) {
                edit = utf8_wrapper(ev.edit);
                ctxt->set_edittext(ev.edit);
            }
            else {
                _handled = false;
            }
        } while (loop);
        _text = ret.str();
        return _text;
    }
};

void diary::show_diary_ui(diary * c_diary)
{
    //c_diary->deserialize();
    
    catacurses::window w_diary;
    catacurses::window w_pages;
    catacurses::window w_text;
    catacurses::window w_changes;
    catacurses::window w_border;
    catacurses::window w_desc;
    catacurses::window w_head;
    catacurses::window w_info;
    enum class window_mode :int {PAGE_WIN=0, CHANGE_WIN,TEXT_WIN, NUM_WIN,FIRST_WIN = 0 , LAST_WIN = NUM_WIN -1};
    window_mode currwin = window_mode::PAGE_WIN;

    /*size_t selection = 0;
    size_t line_text = 0;*/
    std::map<window_mode, int> selected = { {window_mode::PAGE_WIN , 0},{window_mode::CHANGE_WIN, 0},{window_mode::TEXT_WIN,0} };

    int entries_per_page = 0;
    input_context ctxt( "DIARY" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action("NEW_PAGE");
    ctxt.register_action("DELETE PAGE");
    ctxt.register_action("EXPORT_Diary");
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        
        w_diary = new_centered_win( TERMY/2, TERMX/2); //FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH
        w_pages = catacurses::newwin(getmaxy(w_diary) +5, getmaxx(w_diary) * 3 / 10, point(getbegx(w_diary)-5 - getmaxx(w_diary) * 3 / 10, getbegy(w_diary) -2));
        w_changes = catacurses::newwin(getmaxy(w_diary)-3, getmaxx(w_diary) * 5 / 10 -1, point(getbegx(w_diary)  , getbegy(w_diary)+3));
        w_text = catacurses::newwin(getmaxy(w_diary)-3, getmaxx(w_diary) * 5 / 10, point(getbegx(w_diary) + getmaxx(w_diary) * 5 / 10 , getbegy(w_diary)+3));
        w_border = catacurses::newwin(getmaxy(w_diary) + 5, getmaxx(w_diary) +8, point(getbegx(w_diary) -4, getbegy(w_diary) - 2));
        w_desc = catacurses::newwin(3, getmaxx(w_diary) * 3 / 10+ getmaxx(w_diary)+9, point(getbegx(w_diary) - 5 - getmaxx(w_diary) * 3 / 10, getbegy(w_diary) -6));
        w_head = catacurses::newwin(3, getmaxx(w_diary) , point(getbegx(w_diary), getbegy(w_diary)));
        w_info = catacurses::newwin(7, getmaxx(w_diary) + 8, point(getbegx(w_diary) - 4,getbegy(w_diary)+getmaxy(w_diary)+4));
        // content ranges from y=3 to FULL_SCREEN_HEIGHT - 2
        entries_per_page = FULL_SCREEN_HEIGHT - 2;

        ui.position_from_window( w_diary );
    } );
    ui.mark_resize();

    
    //std::vector<diary_page *> *v_pages = &c_diary->pages;


    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_diary );
        werase(w_pages);
        werase(w_changes);
        werase(w_text); 
        werase(w_border); 
        werase(w_desc); 
        werase(w_head);
        werase(w_info);
        
        
        draw_border(w_diary);        
        draw_border(w_desc);
        draw_border(w_info);
        //draw_border(w_head);
        draw_diary_border(&w_border);
        
        
        center_print(w_desc, 0, c_light_gray, string_format(_("%s�s Diary"), c_diary->owner));
        center_print(w_info, 0, c_light_gray, string_format(_("Info")));

        std::string desc = string_format(_("%s, %s, %s, %s"),
            ctxt.get_desc("NEW_PAGE", "new page", input_context::allow_all_keys),
            ctxt.get_desc("CONFIRM", "Edit text", input_context::allow_all_keys),
            ctxt.get_desc("DELETE PAGE", "Delete page", input_context::allow_all_keys),
            ctxt.get_desc("EXPORT_Diary", "Export diary", input_context::allow_all_keys)
            );
        center_print(w_desc, 1,  c_white, desc);
        
        selected[window_mode::PAGE_WIN] = c_diary->set_opend_page(selected[window_mode::PAGE_WIN]);
        print_list_scrollable(&w_pages, c_diary->get_pages_list(), &selected[window_mode::PAGE_WIN], currwin == window_mode::PAGE_WIN,true);
        print_list_scrollable(&w_changes, c_diary->get_change_list(), &selected[window_mode::CHANGE_WIN], currwin == window_mode::CHANGE_WIN,false);
        print_list_scrollable(&w_text, c_diary->get_page_text(), &selected[window_mode::TEXT_WIN],  currwin == window_mode::TEXT_WIN,false);
        //print_editor(w_text, c_diary->get_page_text(), selected[window_mode::TEXT_WIN]);
        trim_and_print(w_head, point(1, 1),getmaxx(w_head)-2,c_white,c_diary->get_head_text());
        if (currwin == window_mode::CHANGE_WIN|| currwin ==window_mode::TEXT_WIN) {
            fold_and_print(w_info, point(1, 1), getmaxx(w_info) - 2, c_white, string_format("%s", c_diary->get_desc_map()[selected[window_mode::CHANGE_WIN]]));
        }
        
        center_print(w_pages, 0, c_light_gray, string_format(_("pages: %d"), c_diary->get_pages_list().size()));

        wnoutrefresh( w_diary );
        wnoutrefresh(w_border);
        wnoutrefresh(w_head);
        wnoutrefresh(w_pages);
        wnoutrefresh(w_changes);
        wnoutrefresh(w_text);
        wnoutrefresh(w_desc);
        wnoutrefresh(w_info);
        
    } );

    while( true ) {
        
        if((!c_diary->pages.empty() && selected[window_mode::PAGE_WIN] >= c_diary->pages.size()) || (c_diary->pages.empty() && selected[window_mode::PAGE_WIN] != 0)) {
            //debugmsg("Sanity check failed: selection=%d, size=%d", static_cast<int>(selection),
                //static_cast<int>(umissions.size()));
            selected[window_mode::PAGE_WIN] = 0;
        }
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "RIGHT" ) {
            currwin = static_cast<window_mode>(static_cast<int>(currwin) + 1);
            if (currwin >= window_mode::NUM_WIN) {
                currwin = window_mode::FIRST_WIN;
            }
            selected[window_mode::TEXT_WIN] = 0;
        }
        else if (action == "LEFT") {
            currwin = static_cast<window_mode>(static_cast<int>(currwin) - 1);
            if (currwin < window_mode::FIRST_WIN) {
                currwin = window_mode::LAST_WIN;
            }
            selected[window_mode::TEXT_WIN] = 0;
        } else if( action == "DOWN" ) {
            /*if(currwin == window_mode::PAGE_WIN){
                selection++;
                if (selection >= c_diary->pages.size()) {
                    selection = 0;
                }
            }
            else if (currwin == window_mode::TEXT_WIN) {
                line_text++;
            }*/
            selected[currwin]++;
            if (selected[window_mode::PAGE_WIN] >= c_diary->pages.size()) {
                selected[window_mode::PAGE_WIN] = 0;
            }
        } else if( action == "UP" ) {
            /*if (currwin == window_mode::PAGE_WIN) {
                if (selection == 0) {
                    selection = c_diary->pages.empty() ? 0 : c_diary->pages.size() - 1;
                }
                else {
                    selection--;
                }
            }
            else if (currwin == window_mode::TEXT_WIN) {
                line_text--;
            }*/
            selected[currwin]--;
            if (selected[window_mode::PAGE_WIN] < 0) {
                selected[window_mode::PAGE_WIN] = c_diary->pages.empty() ? 0 : c_diary->pages.size() - 1;
            }
            
        }
        else if (action == "CONFIRM") {
            c_diary->edit_page_ui(w_text);
            //c_diary->edit_page_ui();
        }else if(action == "NEW_PAGE"){
            c_diary->new_page();
            selected[window_mode::PAGE_WIN] = c_diary->pages.size() - 1;

        }
        else if (action == "DELETE PAGE") {
            if (query_yn(_("Really delete Page?"))) {
                c_diary->delete_page();
                if(selected[window_mode::PAGE_WIN] >= c_diary->pages.size()) {
                    selected[window_mode::PAGE_WIN] --;
                }
            }
        }
        else if (action == "EXPORT_Diary") {
            if (query_yn(_("Export Diary as .txt?"))) {
                c_diary->export_to_txt();
            }
            
        }
        else if (action == "QUIT") {
            //c_diary->serialize();
            break;
        }
        
    }
}
//TODO: redo this, so it can be used like a Editor. 
void diary::edit_page_ui() {
    std::string title = _("Text:");
    static constexpr int max_note_length = 20000;
    static constexpr int max_note_display_length = 45;


    static const int npm_width = 3;

    static const int npm_height = 3;

    int entries_per_page = 0;

    const std::string old_text = get_page_ptr()->m_text;
    std::string new_text = old_text;

    

    bool esc_pressed = false;
    string_input_popup input_popup;
    input_popup
        .title(title)
        .width(max_note_length)
        .text(new_text)
        .description("What happend Today?")
        .title_color(c_white)
        .desc_color(c_light_gray)
        .string_color(c_yellow)
        .identifier("diary");
        

    do {
        new_text = input_popup.query_string(false);
        if (input_popup.canceled()) {
            new_text = old_text;
            esc_pressed = true;
            break;
        }
        else if (input_popup.confirmed()) {
            break;
        }
        
    } while (true);

    if (!esc_pressed && new_text.empty() && !old_text.empty()) {
        if (query_yn(_("Really delete note?"))) {
            get_page_ptr()->m_text = "";
        }
    }
    else if (!esc_pressed && old_text != new_text) {
        get_page_ptr()->m_text = new_text;
    }

}

void diary::edit_page_ui(catacurses::window& win) {
    const std::string old_text = get_page_ptr()->m_text;
    std::string new_text = old_text;



    bool esc_pressed = false;
    /*string_input_popup input_popup;
    input_popup
        .title(title)
        .width(max_note_length)
        .text(new_text)
        .description("What happend Today?")
        .title_color(c_white)
        .desc_color(c_light_gray)
        .string_color(c_yellow)
        .identifier("diary");*/

    editor_window ed = editor_window(win, get_page_ptr()->m_text);

    do {
        new_text = ed.query_string(false, false);
        
        if (ed.canceled()) {
            break;
        }
        else if (ed.confirmed()) {
            break;
        }

    } while (true);

    if (new_text.empty() && !old_text.empty()) {
        if (query_yn(_("Really delete note?"))) {
            get_page_ptr()->m_text = "";
        }
    }
    else if (old_text != new_text) {
        if(query_yn(_("Save entry?"))) get_page_ptr()->m_text = new_text;
    }
}


