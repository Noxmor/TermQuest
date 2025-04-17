#include "mod.h"
#include "prototype_registry.h"

#include <stdlib.h>
#include <string.h>

#include <lua5.4/lualib.h>
#include <lua5.4/lua.h>
#include <lua5.4/lauxlib.h>

static void mod_register_interface(Mod* mod, lua_State* L)
{
    lua_getfield(L, -2, "name");
    const char* internal_name = luaL_checkstring(L, -1);
    char* name = malloc(sizeof(char) * (strlen(mod->name) + 1 + strlen(internal_name) + 1));
    sprintf(name, "%s:%s", mod->name, internal_name);
    lua_pop(L, 1);

    Interface* inf = interface_create(name, 0);
    // TODO: Create commands
    interface_registry_insert(inf);
}

static int data_extend(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "mod");
    Mod* mod = (Mod*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!lua_istable(L, 2))
    {
        return luaL_error(L, "data:extend() requires a table");
    }

    lua_pushnil(L);
    while (lua_next(L, 2) != 0)
    {
        lua_getfield(L, -1, "type");
        const char* type = luaL_checkstring(L, -1);

        if (strcmp(type, "interface") == 0)
        {
            mod_register_interface(mod, L);
        }

        lua_pop(L, 1);

        lua_pop(L, 1);
    }

    return 0;
}

void mod_init(Mod* mod, const char* filepath)
{
    mod->filepath = filepath;
    mod->name = strrchr(mod->filepath, '/');
    if (mod->name != NULL)
    {
        ++mod->name;
    }
    else
    {
        mod->name = mod->filepath;
    }

    mod->active = TQ_FALSE;
}

void mod_load(Mod* mod)
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_newtable(L);
    lua_pushcfunction(L, &data_extend);
    lua_setfield(L, -2, "extend");
    lua_setglobal(L, "data");

    lua_pushlightuserdata(L, mod);
    lua_setfield(L, LUA_REGISTRYINDEX, "mod");

    char buffer[PATH_MAX];
    sprintf(buffer, "%s/data.lua", mod->filepath);

    if (luaL_dofile(L, buffer) != LUA_OK)
    {
        fprintf(stderr, "Error loading mod: %s\n", lua_tostring(L, -1));
        lua_close(L);
    }
}
