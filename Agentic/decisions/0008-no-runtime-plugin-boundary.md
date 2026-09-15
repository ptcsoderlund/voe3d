# 0008. No runtime plugin boundary; folders link statically

**Rule:** Engine folders are linked statically and call each other directly.
There is no plugin registry, no runtime interface lookup, no loading of engine
code from shared libraries, and no ABI-stability obligation between folders.

**Status:** Accepted · 2026-08-28

**Why:** The Machinery's API registry — subsystems publishing structs of
function pointers, discovered by name and version at runtime — is the most
complete answer anyone shipped to C's missing module system. It exists to serve
**hot-reloading code and third-party plugins.** VOE3D has no plugin author, and
live reloading is tiered *later*.

Adopting the mechanism without the requirement is premature generality, and it
runs the trade of ADR-0005 backwards: there we chose Clang-only specifically so
the dialect would be compiler-checked rather than convention-enforced. A runtime
registry replaces compiler-checked calls with unchecked lookups, in exchange for
a capability nothing asks for.

**Cost accepted:** Adding hot reload later means introducing an indirection
boundary where there is none, at whichever seam needs it. ADR-0001 keeps that
cheap — every folder already configures standalone — so this is a change of
mechanism at one seam, not a rewrite. Note also that reload would then be
constrained by ADR-0007's ownership rule and by the `**` ban, which forbid the
pointer tables such a registry usually wants.

**Rejected:** an API registry now, in anticipation. Rejected as premature
generality, by name.
