\# Guidelines



This document is valid for the project. Not for linked or consumed libraries.

Where an external library requires a shape this document forbids — a callback,

a base type, a mutable struct — conform to the library.



An explicit instruction in the task overrides this document. Say when it does.



\## Scope



Work is scoped by folder. Where a language has its own `module` — C++20, F# —

that is a language construct, not a boundary in this document.



The task names one folder. Edit the files in it. A subfolder is a neighbour,

not a part of it. A task naming no folder is unclear — see the end of this

section.



Read this document, the target folder, and its `<folder>.md`.  

Read another folder's `<folder>.md` on demand, when the task requires

understanding it from outside. Its table of contents is a map, not an

explanation: to learn how one of the files named there behaves, read the comment

at the top of that file, and nothing more of it.

If a depended-on folder has no `<folder>.md`, ask for permission to create it.



Report an `.md` that contradicts its code. Do not read further to compensate.



An edit that seems to require another folder is reported, not made:

`BLOCKED: <folder>, <why>`.



Unclear which rule applies: ask once and wait. Where no answer can arrive — a

non-interactive run — take the narrowest reading, continue, and mark it in the

code: `DEVIATION: <rule>, <reading>, <why>`. Never take a reading silently.

List every marker left behind in the final answer.



\## Structure



Group by what the code does, not by what it is — `orders/`, not `interfaces/`.

Everything named — file, folder, namespace, type — is named for its function.

Folders and namespaces stay in sync. A language-level module stays inside one

folder.



Every folder has a table of contents, named `<folder>.md`. It exists to help a

reader navigate with as little cognitive load as possible, and for nothing else.



The real documentation is the comment at the start of every code file. A rule, a

constraint, a gotcha or an example lives there, in exactly one place. The `.md`

may point at it by name. The `.md` never restates it.



The shape is fixed:



\- One or two sentences on what the folder is for, and what it is not.

\- One line per file and per subfolder: the name, then one sentence.

\- Nothing else. The `# <folder>` title is the only heading; no sections below it,

&#x20; no code blocks, no tables, no lists of rules, no scene diagrams. A `.md` that

&#x20; has grown any of those has started documenting, and documenting is the file

&#x20; headers' job.



The whole form:



```markdown

\# motion



Everything that moves a `Node3D` over time. Not what decides where to go.



\- `CharacterMovement.cs` — a `CharacterBody3D` that moves itself towards a posted

&#x20; direction. Two entry points, both required — see its header.

\- `VelocityStep.cs` — the accelerate-or-decelerate rule, shared by both movers.

```



Longer is not more helpful. Longer is a second copy of the headers, and the copy

goes stale while the headers stay right.





\## Coupling



Classes and folders own their data and state. Mutation from outside is

forbidden. Calling a method that mutates is fine — the owner decides.



Where possible, data is exposed as immutable, read-only or as a copy (C# -> struct from {get;}).



A type the `.md` does not name is internal. Naming is a line in the table of

contents — not an explanation, and not a promise that the `.md` describes how the

type behaves. Dependencies point one way. No cycles between folders.



\## Workflow



Humans are responsible for the project, building, planning, testing.



Agents keep documentation and comments updated. A change to a folder's public

surface updates its `.md` in the same change — which means removing lines that no

longer earn their place as much as adding lines that do. A `.md` that only ever

grows is not being maintained.



\## Preference



When writing code, we prefer doing so with composition.  

Data driven design is #1 choice.  

Events, Interface and inheritance comes as second choice.

