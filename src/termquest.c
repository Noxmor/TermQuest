#include "termquest.h"
#include "mod_list.h"
#include "stack.h"

#include <string.h>
#include <termbox.h>

static int running = 0;

typedef struct GameState
{
    Stack inf_stack;
    usize command_index;
} GameState;

static ModList mod_list;

static GameState game_state;

static void termquest_init(void)
{
    tb_init();
    stack_init(&game_state.inf_stack);

    mod_list_init(&mod_list);
    mod_list_scan(&mod_list);
    mod_list_load(&mod_list);

}

static void termquest_shutdown(void)
{
    tb_shutdown();
}

static void render_string(i32 x, i32 y, const char* str)
{
    for (usize i = 0; i < strlen(str); ++i)
    {
        tb_change_cell(x + i, y, str[i], TB_WHITE, TB_BLACK);
    }
}

static void interface_render(const Interface* inf)
{
    usize margin_x = 10;
    usize margin_y = 10;

    usize width = tb_width() - margin_x * 2;

    usize y = margin_y;
    tb_change_cell(margin_x, y, 0x250C, TB_WHITE, TB_BLACK);
    tb_change_cell(margin_x + width - 1, y, 0x2510, TB_WHITE, TB_BLACK);

    for (usize i = 2; i < width; ++i)
    {
        tb_change_cell(margin_x + i - 1, y, 0x2500, TB_WHITE, TB_BLACK);
    }

    ++y;

    tb_change_cell(margin_x, y, 0x2502, TB_WHITE, TB_BLACK);
    tb_change_cell(margin_x + width - 1, y, 0x2502, TB_WHITE, TB_BLACK);
    render_string(margin_x + 2, y, inf->name);


    ++y;

    tb_change_cell(margin_x, y, 0x251C, TB_WHITE, TB_BLACK);
    tb_change_cell(margin_x + width - 1, y, 0x2524, TB_WHITE, TB_BLACK);

    for (usize i = 2; i < width; ++i)
    {
        tb_change_cell(margin_x + i - 1, y, 0x2500, TB_WHITE, TB_BLACK);
    }

    ++y;

    for (usize i = 0; i < inf->commands_count; ++i)
    {
        tb_change_cell(margin_x, i + y, 0x2502, TB_WHITE, TB_BLACK);

        Command* cmd = &inf->commands[i];

        if (game_state.command_index == i)
        {
            render_string(margin_x + 2, i + y, "> ");
        }

        render_string(margin_x + 4, i + y, cmd->display_key);
        tb_change_cell(margin_x + width - 1, i + y, 0x2502, TB_WHITE, TB_BLACK);
    }

    y += inf->commands_count;
    tb_change_cell(margin_x, y, 0x2514, TB_WHITE, TB_BLACK);
    tb_change_cell(margin_x + width - 1, y, 0x2518, TB_WHITE, TB_BLACK);

    for (usize i = 2; i < width; ++i)
    {
        tb_change_cell(margin_x + i - 1, y, 0x2500, TB_WHITE, TB_BLACK);
    }
}

void termquest_push_interface(Interface* inf)
{
    stack_push(&game_state.inf_stack, inf);
}

static Interface* termquest_peek_interface(void)
{
    return stack_peek(&game_state.inf_stack);
}

Interface* termquest_pop_interface(void)
{
    return stack_pop(&game_state.inf_stack);
}

static void termquest_interface_move_up(void)
{
    if (game_state.command_index > 0)
    {
        --game_state.command_index;
    }
}

static void termquest_interface_move_down(void)
{
    Interface* inf = termquest_peek_interface();
    if (game_state.command_index + 1 < inf->commands_count)
    {
        ++game_state.command_index;
    }
}

void termquest_execute_command(void)
{
    Interface* inf = termquest_peek_interface();
    mod_list_execute_command(&mod_list, inf->commands[game_state.command_index].display_key);
}

void termquest_run(void)
{
    termquest_init();

    struct tb_event event;
    running = 1;

    while (running)
    {
        Interface* active_inf = termquest_peek_interface();

        tb_clear();
        interface_render(active_inf);
        tb_present();

        switch (tb_poll_event(&event))
        {
            case TB_EVENT_KEY:
            {
                if (event.key == TB_KEY_ESC)
                {
                    termquest_quit();
                }
                else if (event.key == TB_KEY_ARROW_UP)
                {
                    termquest_interface_move_up();
                }
                else if (event.key == TB_KEY_ARROW_DOWN)
                {
                    termquest_interface_move_down();
                }
                else if (event.key == TB_KEY_ENTER)
                {
                    termquest_execute_command();
                }

                break;
            }

            default: break;
        }
    }

    termquest_shutdown();
}

void termquest_quit(void)
{
    running = 0;
}
