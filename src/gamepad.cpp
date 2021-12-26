#include "gamepad.h"
#include "debug.h"
#include "sdl_wrappers.h"

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

extern input_event last_input;

namespace gamepad
{

constexpr int max_triggers = 2;
constexpr int max_sticks = 2;
constexpr int max_axis = 6;
constexpr int max_buttons = 30;

static int triggers_axis[max_triggers] = {
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT
};
static int sticks_axis[max_sticks][2] = {
    { SDL_CONTROLLER_AXIS_LEFTX,  SDL_CONTROLLER_AXIS_LEFTY  },
    { SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY }
};

static int triggers_state[max_triggers] = {0, 0};
static int sticks_state[max_sticks] = {0, 0};
static int hat_state = 0;

static int triggers_threshold = 16000;
static int sticks_threshold = 16000;
static int error_margin = 2000;

static int sticks_map[max_sticks][16] = {0};
static int triggers_map[max_triggers] = {0};
static int buttons_map[max_buttons] = {0};
static int hat_map[16] = {0};

struct task_t {
    Uint32 when;
    int button;
    int counter;
    int state;
};

constexpr int max_tasks           = max_buttons + max_sticks + max_triggers + 1;
constexpr int buttons_task_index  = 0;
constexpr int sticks_task_index   = max_buttons;
constexpr int triggers_task_index = max_buttons + max_sticks;
constexpr int hat_task_index      = max_buttons + max_sticks + max_triggers;

task_t all_tasks[max_tasks];
task_t &hat_task = all_tasks[hat_task_index];

static int repeat_delay = 400;
static int repeat_interval = 200;
static int diagonal_detect_delay = 250;

// SDL related stuff
SDL_TimerID timer_id;
SDL_GameController *controller = nullptr;


static Uint32 timer_func( Uint32 interval, __attribute__( ( unused ) ) void *param )
{
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = GAMEPAD_SCHEDULER;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = GAMEPAD_SCHEDULER;
    event.user = userevent;

    SDL_PushEvent( &event );
    return( interval );
}

void init()
{
    // FIXME maybe stick/trigger "buttons" need their own names?
    sticks_map[0][0b0001] = JOY_UP;
    sticks_map[0][0b0010] = JOY_RIGHT;
    sticks_map[0][0b0100] = JOY_DOWN;
    sticks_map[0][0b1000] = JOY_LEFT;
    sticks_map[0][0b0011] = JOY_RIGHTUP;
    sticks_map[0][0b0110] = JOY_RIGHTDOWN;
    sticks_map[0][0b1100] = JOY_LEFTDOWN;
    sticks_map[0][0b1001] = JOY_LEFTUP;

    sticks_map[1][0b0001] = JOY_21;
    sticks_map[1][0b0010] = JOY_22;
    sticks_map[1][0b0100] = JOY_23;
    sticks_map[1][0b1000] = JOY_24;
    sticks_map[1][0b0011] = JOY_25;
    sticks_map[1][0b0110] = JOY_26;
    sticks_map[1][0b1100] = JOY_27;
    sticks_map[1][0b1001] = JOY_28;

    triggers_map[0] = JOY_29;
    triggers_map[1] = JOY_30;

    hat_map[0b0001] = JOY_UP;
    hat_map[0b0010] = JOY_RIGHT;
    hat_map[0b0100] = JOY_DOWN;
    hat_map[0b1000] = JOY_LEFT;
    hat_map[0b0011] = JOY_RIGHTUP;
    hat_map[0b0110] = JOY_RIGHTDOWN;
    hat_map[0b1100] = JOY_LEFTDOWN;
    hat_map[0b1001] = JOY_LEFTUP;

    for( int i = 0; i < max_buttons; ++i ) {
        buttons_map[i] = JOY_0 + i;
    }

    for( int i = 0; i < max_tasks; ++i ) {
        all_tasks[i].counter = 0;
    }

    int ret = SDL_Init( SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER );
    if( ret < 0 ) {
        printErrorIf( ret != 0, "Init gamecontroller+timer failed" );
        return;
    }
    int numjoy = SDL_NumJoysticks();
    if( numjoy >= 1 ) {
        if( numjoy > 1 ) {
            dbg( D_WARNING ) <<
                             "You have more than one gamepads/joysticks plugged in, only the first will be used.";
        }
        controller = SDL_GameControllerOpen( 0 );
        printErrorIf( controller == nullptr, "SDL_GameControllerOpen failed" );
        if( controller ) {
            printErrorIf( SDL_GameControllerEventState( SDL_ENABLE ) < 0,
                          "SDL_GameControllerEventState(SDL_ENABLE) failed" );
        }
    } else {
        controller = nullptr;
        return;
    }

    timer_id = SDL_AddTimer( 50, timer_func, NULL );
    printErrorIf( timer_id == 0, "SDL_AddTimer failed" );
}

void quit()
{
    if( timer_id ) {
        SDL_RemoveTimer( timer_id );
        timer_id = 0;
    }
    if( controller ) {
        SDL_GameControllerClose( controller );
        controller = nullptr;
    }
}

SDL_GameController *get_controller()
{
    return controller;
}

static int one_of_two( int arr[2], int val )
{
    if( arr[0] == val ) {
        return 0;
    }
    if( arr[1] == val ) {
        return 1;
    }
    return -1;
}

static void schedule_task( task_t &task, Uint32 when, int button, int state )
{
    task.when = when;
    task.button = button;
    task.counter = 1;
    task.state = state;
}

static void cancel_task( task_t &task )
{
    task.counter = 0;
}

static void send_input( int ibtn, input_event_t itype = input_event_t::gamepad )
{
    last_input = input_event( ibtn, itype );
}

/*
#define STATE_TO_BINARY(state)  \
  (state & 0b1000 ? '1' : '0'), \
  (state & 0b0100 ? '1' : '0'), \
  (state & 0b0010 ? '1' : '0'), \
  (state & 0b0001 ? '1' : '0')
*/

static void dpad_changes( task_t &task, int m[16], Uint32 now, int old_state, int new_state )
{
    // get rid of unneeded bits
    old_state &= 0b1111;
    new_state &= 0b1111;

    if( old_state == new_state ) {
        return;
    }

    switch( new_state ) {
        case SDL_HAT_LEFTUP:
        case SDL_HAT_LEFTDOWN:
        case SDL_HAT_RIGHTUP:
        case SDL_HAT_RIGHTDOWN:
            send_input( m[new_state] );
            schedule_task( task, now + repeat_delay, m[new_state], new_state );
            break;
        case SDL_HAT_CENTERED: {
            if( task.counter == 1 )
                switch( old_state ) {
                    // detecting quick CENTER -> Left|Up|Right|Down -> CENTER transition
                    case SDL_HAT_UP:
                    case SDL_HAT_DOWN:
                    case SDL_HAT_RIGHT:
                    case SDL_HAT_LEFT:
                        send_input( task.button );
                }
            cancel_task( task );
            break;
        }
        case SDL_HAT_UP:
        case SDL_HAT_DOWN:
        case SDL_HAT_RIGHT:
        case SDL_HAT_LEFT:
            schedule_task( task, now + diagonal_detect_delay, m[new_state], new_state );
            task.counter += old_state;
            break;
    }
    //SDL_LogDebug(0, "os=%c%c%c%c ns=%c%c%c%c task={%lu,%i,%i,%c%c%c%c} now=%lu",
    //             STATE_TO_BINARY(old_state), STATE_TO_BINARY(new_state),
    //             task.when, task.button, task.counter, STATE_TO_BINARY(task.state), now);
}

void handle_axis_event( SDL_Event &event )
{
    if( event.type != SDL_CONTROLLERAXISMOTION ) {
        return;
    }

    int axis = event.caxis.axis;
    int value = event.caxis.value;
    Uint32 now = event.caxis.timestamp;

    // check triggers
    int idx = one_of_two( triggers_axis, axis );
    if( idx >= 0 ) {
        int state = triggers_state[idx];
        int button = triggers_map[idx];
        task_t &task = all_tasks[triggers_task_index + idx];
        if( !state && value > triggers_threshold + error_margin ) {
            send_input( button );
            triggers_state[idx] = 1;
            schedule_task( task, now + repeat_delay, button, 1 );
        }
        if( state && value < triggers_threshold - error_margin ) {
            triggers_state[idx] = 0;
            cancel_task( task );
        }
        return;
    }

    // check sticks
    for( int i = 0; i < max_sticks; ++i ) {
        int idx = one_of_two( sticks_axis[i], axis );
        if( idx >= 0 ) {
            int old_state = sticks_state[i];
            int new_state = old_state;
            task_t &task = all_tasks[sticks_task_index + i];

            // calculate dpad state for the analog sticks, simulating joystick hat state (4 bits)
            if( idx ) { // vertical stick movement
                if( !( old_state & 0b0100 ) && value > sticks_threshold + error_margin ) {
                    new_state |= 0b0100;    // turn on bit _x__
                } else if( ( old_state & 0b0100 ) && value < sticks_threshold - error_margin ) {
                    new_state &= 0b1011;    // turn off bit _x__
                } else if( !( old_state & 0b0001 ) && value < -sticks_threshold - error_margin ) {
                    new_state |= 0b0001;    // turn on bit ___x
                } else if( ( old_state & 0b0001 ) && value > -sticks_threshold + error_margin ) {
                    new_state &= 0b1110;    // turn off bit ___x
                }
            } else { // horizontal stick movement
                if( !( old_state & 0b0010 ) && value > sticks_threshold + error_margin ) {
                    new_state |= 0b0010;    // turn on bit __x_
                } else if( ( old_state & 0b0010 ) && value < sticks_threshold - error_margin ) {
                    new_state &= 0b1101;    // turn off bit __x_
                } else if( !( old_state & 0b1000 ) && value < -sticks_threshold - error_margin ) {
                    new_state |= 0b1000;    // turn on bit x___
                } else if( ( old_state & 0b1000 ) && value > -sticks_threshold + error_margin ) {
                    new_state &= 0b0111;    // turn off bit x___
                }
            }

            sticks_state[i] = new_state;
            dpad_changes( task, sticks_map[i], now, old_state, new_state );
        }
    }
}

void handle_hat_event( SDL_Event &event )
{
    if( event.type != SDL_JOYHATMOTION ) {
        return;
    }

    int old_state = hat_state;
    hat_state = event.jhat.value;
    Uint32 now = event.jhat.timestamp;
    dpad_changes( hat_task, hat_map, now, old_state, hat_state );
}

void handle_button_event( SDL_Event &event )
{
    switch( event.type ) {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            int button = event.cbutton.button;
            int state = event.cbutton.state;
            Uint32 now = event.cbutton.timestamp;
            task_t &task = all_tasks[button];
            if( state ) {
                switch( button ) {
                    case SDL_CONTROLLER_BUTTON_BACK:    // BACK -> Escape
                        send_input( KEY_ESCAPE, input_event_t::keyboard_code );
                        break;
                    case SDL_CONTROLLER_BUTTON_START:   // START -> Enter
                        send_input( '\n', input_event_t::keyboard_char );
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        send_input( KEY_UP, input_event_t::keyboard_char );
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        send_input( KEY_DOWN, input_event_t::keyboard_char );
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        send_input( KEY_LEFT, input_event_t::keyboard_char );
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        send_input( KEY_RIGHT, input_event_t::keyboard_char );
                        break;
                    default:
                        send_input( button );
                        schedule_task( task, now + repeat_delay, buttons_map[button], state );
                }
            } else {
                cancel_task( task );
            }
        }
    }
}

void handle_scheduler_event( SDL_Event &event )
{
    Uint32 now = event.user.timestamp;
    for( auto &task : all_tasks )
        if( task.counter && task.when <= now ) {
            send_input( task.button );
            task.counter += 1;
            task.when = now + repeat_interval;
        }
}

} // namespace gamepad
