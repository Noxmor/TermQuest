#ifndef TQ_INTERFACE_H
#define TQ_INTERFACE_H

#include "types.h"
#include "command.h"

typedef struct Interface
{
    const char* name;
    Command* commands;
    usize commands_count;
} Interface;

Interface* interface_create(const char* name);

#endif
