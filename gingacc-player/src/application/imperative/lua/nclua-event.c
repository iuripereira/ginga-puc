/* nclua.c -- The NCLua Event library.
   Copyright (C) 2012 PUC-Rio/Laboratorio TeleMidia

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 51
Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <limits.h>

#include <lua.h>
#include <lauxlib.h>
#include <pthread.h>

#include "nclua.h"
#include "nclua-private.h"
#include "nclua-luax-private.h"

/* Function prototypes:  */
static int l_get_handler_list (lua_State *L);
static int l_post (lua_State *L);
static int l_register (lua_State *L);
static int l_unregister (lua_State *L);
static int l_uptime (lua_State *L);

/* List of Lua functions registered by the NCLua Event library.  */
static const struct luaL_Reg __nclua_event_function_list[] =
{
  { "get_handler_list", l_get_handler_list },
  { "post",             l_post             },
  { "register",         l_register         },
  { "unregister",       l_unregister       },
  { "uptime",           l_uptime           },
  { NULL,               NULL               }
};


/* Exported private functions.  */

/* Opens the NCLua Event library.  Returns NCLUA_STATUS_SUCCESS or
   NCLUA_STATUS_NO_MEMORY if memory could not be allocated.  */

nclua_status_t
_nclua_event_open (lua_State *L)
{
  lua_newtable (L);
  _nclua_set_registry_data (L, _NCLUA_REGISTRY_INPUT_QUEUE);

  lua_newtable (L);
  _nclua_set_registry_data (L, _NCLUA_REGISTRY_OUTPUT_QUEUE);

  lua_newtable (L);
  _nclua_set_registry_data (L, _NCLUA_REGISTRY_HANDLER_LIST);

  lua_newtable (L);
  _nclua_set_registry_data (L, _NCLUA_REGISTRY_TIMER_LIST);

  luaL_register (L, "event", __nclua_event_function_list);
  lua_pop (L, 1);

  return NCLUA_STATUS_SUCCESS;
}

/* Closes the NCLua Event library.  */

void
_nclua_event_close (lua_State *L)
{
  _nclua_unset_registry_data (L, _NCLUA_REGISTRY_INPUT_QUEUE);
  _nclua_unset_registry_data (L, _NCLUA_REGISTRY_OUTPUT_QUEUE);
  _nclua_unset_registry_data (L, _NCLUA_REGISTRY_HANDLER_LIST);
  _nclua_unset_registry_data (L, _NCLUA_REGISTRY_TIMER_LIST);
}

/* Returns true if event at index EVENT matches filter at index FILTER.
   Otherwise, returns false.  */

static inline nclua_bool_t
match (lua_State *L, int event, int filter)
{
  nclua_bool_t status = TRUE;
  int top;

  top = lua_gettop (L);
  event = ncluax_abs (L, event);
  filter = ncluax_abs (L, filter);

  lua_pushnil (L);
  while (lua_next (L, filter) != 0)
    {
      lua_pushvalue (L, -2);
      lua_gettable (L, event);

      if (lua_isnil (L, -1) || !lua_equal (L, -1, -2))
        {
          status = FALSE;
          break;                /* mismatch */
        }

      lua_pop (L, 2);
    }

  lua_settop (L, top);
  return status;
}

/* For each function in handler list, if its filter matches event at top of
   stack, call it passing the event as argument.  If the function returns
   true, stop processing (event handled).  Otherwise, continue until all
   functions have been called.

   Returns the number of handlers called.  */

int
_nclua_notify (lua_State *L)
{
  int event;                    /* event parameter */
  int list;                     /* handler list */
  int size;                     /* handler list size */
  int n_called;                 /* number of handlers called */

  int saved_top;
  int i;

  saved_top = lua_gettop (L);
  event = ncluax_abs (L, -1);

  _nclua_get_registry_data (L, _NCLUA_REGISTRY_HANDLER_LIST);
  list = ncluax_abs (L, -1);
  size = lua_objlen (L, list);

  n_called = 0;
  for (i = 1; i <= size; i++)
    {
      lua_rawgeti (L, list, i);
      lua_rawgeti (L, -1, 2);
      if (match (L, event, -1))
        {
          lua_rawgeti (L, -2, 1);
          lua_pushvalue (L, event);
          lua_call (L, 1, 1);
          n_called++;
          if (lua_toboolean (L, -1))
            break;
        }
    }

  lua_settop (L, saved_top);

  return n_called;
}


/* The NCLua Event API -- From Lua to C.  */

/* event.get_handler_list () -> list:table

   Returns the handler list.  */

static int
l_get_handler_list (lua_State *L)
{
  _nclua_get_registry_data (L, _NCLUA_REGISTRY_HANDLER_LIST);
  return 1;
}

/* event.post ([destination:string], event:table)
     -> status:boolean, error_message:string

   Posts event into one of the state queues.  If DESTINATION != "in"
   or if DESTINATION is not given, inserts event EVENT into output queue.
   Otherwise, inserts event into input queue.

   Returns true if successful.
   Otherwise, returns false plus error message.  */

static int
l_post (lua_State *L)
{
  const char *dest;             /* destination parameter */
  int queue;                    /* input/output queue */
  int size;                     /* queue size */

  if (lua_istable (L, 1))
    {
      lua_pushstring (L, "out");
      lua_insert (L, 1);
    }
  dest = luaL_optstring (L, 1, "out");
  luaL_checktype (L, 2, LUA_TTABLE);
  lua_settop (L, 2);

  /* Post event to input queue.  */

  if (streq (dest, "in"))
    {
      _nclua_get_registry_data (L, _NCLUA_REGISTRY_INPUT_QUEUE);
      queue = ncluax_abs (L, -1);
      size = lua_objlen (L, -1);
    }

  /* Post event to output queue.  */

  else
    {
      _nclua_get_registry_data (L, _NCLUA_REGISTRY_OUTPUT_QUEUE);
      queue = ncluax_abs (L, -1);
      size = lua_objlen (L, -1);
    }

  lua_pushvalue (L, 2);
  ncluax_rawinsert (L, queue, size + 1);

  lua_pushboolean (L, TRUE);
  return 1;
}

/* event.register ([position:number], function:function, [filter:table])

   Appends the function FUNCTION to the handler list.
   If POSITION is given, registers FUNCTION at position POSITION.
   If FILTER is given, uses FILTER as event filter for FUNCTION.  */

static int
l_register (lua_State *L)
{
  int position;                 /* position parameter */
  int list;                     /* handler list */
  int size;                     /* handler list size */

  if (lua_isfunction (L, 1))
    {
      lua_pushinteger (L, INT_MAX); /* default position */
      lua_insert (L, 1);
    }
  position = luaL_optint (L, 1, INT_MAX);

  luaL_checktype (L, 2, LUA_TFUNCTION);

  if (lua_isnoneornil (L, 3))
    {
      _nclua_get_registry_data (L, _NCLUA_REGISTRY_EMPTY_TABLE);
      lua_insert (L, 3);
    }

  lua_settop (L, 3);

  /* Insert {f, filter} into the given position in handler list.  */

  _nclua_get_registry_data (L, _NCLUA_REGISTRY_HANDLER_LIST);
  list = ncluax_abs (L, -1);
  size = lua_objlen (L, -1);
  position = range (position, 1, size + 1);

  lua_createtable (L, 2, 0);
  lua_pushvalue (L, 2);
  lua_rawseti (L, -2, 1);
  lua_pushvalue (L, 3);
  lua_rawseti (L, -2, 2);

  ncluax_rawinsert (L, list, position);
  return 0;
}

/* event.unregister (function:function)
   event.unregister (function1:function, function2:function, ...)
   event.unregister ()
     -> n_removed:number

   Removes all entries indexed by function FUNCTION from handler list.
   In the second form, removes the given functions from handler list.
   In the third form, removes all functions from handler list.
   Returns the number of entries removed from handler list.  */

static int
l_unregister (lua_State *L)
{
  int list;                     /* handler list */
  int size;                     /* handler list size */
  int n_removed;                /* number of functions removed */

  int top;
  int i;

  /* 3rd form: remove all entries from queue.  */

  if (lua_gettop (L) == 0)
    {
      _nclua_get_registry_data (L, _NCLUA_REGISTRY_HANDLER_LIST);
      list = ncluax_abs (L, -1);
      size = lua_objlen (L, -1);

      for (i = 1; i <= size; i++)
        {
          lua_pushnil (L);
          lua_rawseti (L, list, i);
        }

      lua_pushinteger (L, size);
      return 1;
    }

  /* 1st and 2nd forms: process each argument.  */

  n_removed = 0;
  while ((top = lua_gettop (L)) > 0)
    {
      int saved_size;
      int i;

      luaL_checktype (L, 1, LUA_TFUNCTION);

      _nclua_get_registry_data (L, _NCLUA_REGISTRY_HANDLER_LIST);
      list = ncluax_abs (L, -1);
      size = lua_objlen (L, -1);
      saved_size = size;

      i = 1;
      while (i <= size)
        {
          lua_rawgeti (L, list, i);
          lua_rawgeti (L, -1, 1);

          if (lua_equal (L, -1, 1))
            {
              ncluax_rawremove (L, list, i);
              size--;
              continue;
            }

          i++;
        }

      n_removed += saved_size - size;
      lua_settop (L, top);
      lua_remove (L, 1);
    }

  lua_pushinteger (L, n_removed);
  return 1;
}

/* event.uptime () -> ms:number

   Returns the number of milliseconds elapsed since the first cycle.  */

static int
l_uptime (lua_State *L)
{
  nclua_t *nc = nclua_get_nclua_state (L);
  unsigned int uptime = _nclua_get_uptime (nc);
  lua_pushinteger (L, max (((int) uptime), (int) 0));
  return 1;
}
