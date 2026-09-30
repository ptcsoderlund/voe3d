# 0303 — A project type names its need with a call of its own
date: 2026-09-30
by: planner

## Decision
Replaces 0302 point 5's member. `voe_game_project_type` keeps the six members it had before 043; a
project names what a type needs by a separate call after registering it,
`voe_game_project_component_needs(world, key, needed)`, which sets it with
`voe_ecs_component_needs_set`. A refusal (a key not registered through game, a needed key not
registered, a need already set) is a line on stderr and false, as registration's are. A type that
makes no such call needs nothing. The tank game's placed types make the call; Breakable does not.

## Reasoning
A project's code builds with `-Wall -Wextra -Werror`, and Clang's `-Wmissing-field-initializers`
turns every positional initialiser written before 043 into a build error once the struct grows, so
any existing project stopped building (bug 01 of 043). A call of its own leaves old code untouched
and says the need where a reader looks for it. Rejected: dropping the warning in the project build
(it hides the same mistake everywhere else); requiring designated initialisers (still breaks every
existing project).

## Replaces
0302 point 5, its `needs` member only.
