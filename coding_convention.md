linux kernel coding style.

Keep functions small with asserts.  
Only one layer of dereferencing. my\_var\* is allowed, my\_var\*\* is not allowed, more asterisks are not allowed.

## Naming

All functions are prefixed `voe_`.

The namespace is spelled out in the name. A folder is a namespace, and nested
folders spell out in order:

    voe_<namespace>_<subnamespace>_<name>

    voe_math_vec3_add
    voe_render_pipeline_new

Types follow the same rule. `voe_math_vec3` is a type, `voe_math_vec3_add` is a
function on it.

## Modules

A folder is a namespace. A module is a component plus its system. It may span
several files in that namespace, and they all share the module name.

    <module>_component.h   the data. Read by anyone, const.
    <module>_system.h      the intent types, and how to submit them.

Data is read by anyone. It is changed only by its own system, and only by
sending that system an intent. An intent is a datatype, not the data itself.

Compact pure data needs no system. `voe_math_vec3` is a struct exposed for use,
with support functions, used inline. It has no owner because it has no identity.
The component and system pairing is for state something owns and mutates over
time, not for value types.

**Systems do not depend on other systems.** A system depends on data — its own
component, other components it reads, and intent types. It never calls into
another system. Chains of `system -> system -> system` are what this rule
exists to prevent.

## Allocation

Two pairs. Which one you use depends on where the memory goes, and the name says
which, so a reader never has to guess who frees it.

`new` allocates and **returns** it to the caller. The caller owns it and calls
`destroy`:

    voe_render_pipeline_new
    voe_render_pipeline_destroy

`init` allocates for a module's or system's **own internal use** and returns
nothing. The module owns it and frees it in `deinit`:

    voe_render_system_init
    voe_render_system_deinit

The rule reads both ways, and the second direction is the useful one: a function
with none of these words in its name does not allocate.

Be explicit about implicit flows. Memory that appears without the name saying so
is the thing this rule exists to prevent.
