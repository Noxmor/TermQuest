#include "termquest.h"
#include "mod_list.h"
#include "stack.h"

#include <string.h>
#include <termbox.h>

#define TQ_PROMPT_BUFFER_CAPACITY 128

static int running = 0;

typedef struct GameState
{
    Stack inf_stack;
    usize command_box_height;
    usize margin_x;
    usize margin_y;
    char prompt_buffer[TQ_PROMPT_BUFFER_CAPACITY];
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

    game_state.margin_x = 4;
    game_state.margin_y = 4;
    game_state.command_box_height = 8;
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
    usize width = tb_width() - game_state.margin_x * 2;

    usize y = game_state.margin_y;
    tb_change_cell(game_state.margin_x, y, 0x250C, TB_WHITE, TB_BLACK);
    tb_change_cell(game_state.margin_x + width - 1, y, 0x2510, TB_WHITE, TB_BLACK);

    for (usize i = 2; i < width; ++i)
    {
        tb_change_cell(game_state.margin_x + i - 1, y, 0x2500, TB_WHITE, TB_BLACK);
    }

    ++y;

    tb_change_cell(game_state.margin_x, y, 0x2502, TB_WHITE, TB_BLACK);
    tb_change_cell(game_state.margin_x + width - 1, y, 0x2502, TB_WHITE, TB_BLACK);

    usize name_offset = width / 2 - strlen(inf->name) / 2;
    render_string(game_state.margin_x + name_offset, y, inf->name);


    ++y;

    tb_change_cell(game_state.margin_x, y, 0x251C, TB_WHITE, TB_BLACK);
    tb_change_cell(game_state.margin_x + width - 1, y, 0x2524, TB_WHITE, TB_BLACK);

    for (usize i = 2; i < width; ++i)
    {
        tb_change_cell(game_state.margin_x + i - 1, y, 0x2500, TB_WHITE, TB_BLACK);
    }

    while (++y < tb_height() - game_state.margin_y - 1 - game_state.command_box_height)
    {
        tb_change_cell(game_state.margin_x, y, 0x2502, TB_WHITE, TB_BLACK);
        tb_change_cell(game_state.margin_x + width - 1, y, 0x2502, TB_WHITE, TB_BLACK);
    }

    tb_change_cell(game_state.margin_x, y, 0x251C, TB_WHITE, TB_BLACK);
    tb_change_cell(game_state.margin_x + width - 1, y, 0x2524, TB_WHITE, TB_BLACK);

    for (usize i = 2; i < width; ++i)
    {
        tb_change_cell(game_state.margin_x + i - 1, y, 0x2500, TB_WHITE, TB_BLACK);
    }

    ++y;

    i32 offset = inf->command_index - inf->cursor_index;
    for (usize i = 0; i < game_state.command_box_height; ++i)
    {
        tb_change_cell(game_state.margin_x, y + i, 0x2502, TB_WHITE, TB_BLACK);
        tb_change_cell(game_state.margin_x + width - 1, y + i, 0x2502, TB_WHITE, TB_BLACK);

        if (offset + i >= 0 && offset + i < inf->commands_count)
        {
            Command* cmd = interface_get_command(inf, offset + i);
            render_string(game_state.margin_x + 4, y + i, cmd->name);
        }
    }

    render_string(game_state.margin_x + 2, y + inf->cursor_index, "> ");

    y = tb_height() - game_state.margin_y;
    tb_change_cell(game_state.margin_x, y, 0x2514, TB_WHITE, TB_BLACK);
    tb_change_cell(game_state.margin_x + width - 1, y, 0x2518, TB_WHITE, TB_BLACK);

    for (usize i = 2; i < width; ++i)
    {
        tb_change_cell(game_state.margin_x + i - 1, y, 0x2500, TB_WHITE, TB_BLACK);
    }

    mod_list_render_interface(&mod_list, inf);
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
    Interface* inf = termquest_peek_interface();

    if (inf->command_index > 0)
    {
        --inf->command_index;
    }

    if (inf->cursor_index > 0)
    {
        --inf->cursor_index;
    }
}

static void termquest_interface_move_down(void)
{
    Interface* inf = termquest_peek_interface();
    if (inf->command_index + 1 < inf->commands_count)
    {
        ++inf->command_index;

        if (inf->cursor_index + 1 < game_state.command_box_height)
        {
            ++inf->cursor_index;
        }
    }
}

void termquest_create_save_file(const char* name)
{
    // TODO: Create save file and initialize new save
}

u32 termquest_width(void)
{
    return tb_width() - 2 * game_state.margin_x;
}

u32 termquest_height(void)
{
    return tb_height() - 2 * game_state.margin_y - game_state.command_box_height - 5;
}

void termquest_render(i32 x, i32 y, const char* str)
{
    render_string(x + game_state.margin_x + 1, y + game_state.margin_y + 3, str);
}

void termquest_execute_command(void)
{
    Interface* inf = termquest_peek_interface();
    mod_list_execute_command(&mod_list, interface_get_command(inf, inf->command_index));
}

static void prompt_user(const char* message)
{
    tb_clear();
    render_string(0, 0, message);
    tb_present();

    memset(game_state.prompt_buffer, '\0', sizeof(char) * TQ_PROMPT_BUFFER_CAPACITY);

    usize offset = strlen(message);
    usize index = 0;
    b8 done = TQ_FALSE;
    while (!done)
    {
        struct tb_event event;
        if (tb_poll_event(&event) == TB_EVENT_KEY)
        {
            if (event.key == TB_KEY_ENTER)
            {
                done = TQ_TRUE;
                game_state.prompt_buffer[index] = '\0';
            }
            else if (event.key == TB_KEY_BACKSPACE || event.key == TB_KEY_BACKSPACE2)
            {
                if (index > 0)
                {
                    --index;
                }

                tb_change_cell(offset + index, 0, ' ', TB_WHITE, TB_BLACK);
                tb_present();
            }
            else
            {
                game_state.prompt_buffer[index] = event.ch;
                tb_change_cell(offset + index++, 0, event.ch, TB_WHITE, TB_BLACK);
                tb_present();
            }
        }
    }
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
