# Interpreting FindMUS output

## Setup
```
FznSubProblem:	hard cons: 1483	soft cons: 755	leaves: 755	branches: 889	Built tree in 0.03854 seconds.
```

The first line describes the "Checking solver" view of the problem. It sees
1483 "hard" (background) constraints, 755 "soft" (foreground) constraints (this
is the set that will be searched). Branches refers to the branching points in
the full constraint hierarchy.

```
SubsetMap:	nleaves:	222	nbranches:	244
```

This second line describes how the "Map Solver" sees the problem.
The map solver is used for proposing candidate subsets for the checking
solver to check.  It has grouped the 755 constraints of the problem into
222 leaves. Since findMUS was executed with "--paramset mzn" it treats
decompositions of global constraints called from the model as a single leaf.

## Sanity checks
```
FznSubProblem:	Solve: 2(U:U) ncons:      755	took: 0.01031 seconds
FznSubProblem:	Solve: 1(S:S) ncons:        0	took: 0.00578 seconds
```
These first two checks verify that the entire problem is unsat and that the
background is satisfiable. The `ncons` field shows how many foreground FlatZinc
constraints were included in the subset being checked. Since this problem has
755 foreground constraints the check of the entire problem includes the full
set and the background check includes 0 constraints from the foreground.


# Enumeration

## Checks

```
FznSubProblem:	Solve: 2(U:U) ncons:      755	took: 0.00481 seconds
FznSubProblem:	Solve: 1(S:S) ncons:      587	took: 0.00608 seconds
FznSubProblem:	Solve: 5(?:S) ncons:      109	took: 1.00410 seconds
```

Here we see three subset checks that report different statuses.  The status
is listed in the `Solve` field between parenthesis.  The status shows two
values separated by a colon: `(A:B)`.  `A` here represents the result we
received from the underlying solver.  `U` means UNSAT, `S` means SAT, `?` means
UNKNOWN. Typically `?` is the result of a check exceeding its timelimit (This
can be seen in the third example case where the timelimit was set to 1 second
and the subset could not be solved in this time).

## Interpreting an MUS

```
MUS: 23 688 680 0
```
This line lists the constraint indices in the FlatZinc file.  The numbers are
0-indexed with 0 being the first constraint listed in the FlatZinc (not the
first Item).

```
Brief: clause;:(i=1,p=1)
clause;:(i=2)
int_eq;:(i=11)
int_le;:()
```

The "brief" output lists the constraint type (`clause`, `int_eq`, etc...),
constraint/expression name if available, and the values of loop index
variables.  As an example of a case where constraint/expresison names are
present consider this listing.

```
all_different_int;@{ADRows@AD(row 2)}:(i=2)
```

The constraint and expression names associated with this constraint are
listed in the field delimited by: `@{}`. In this case the constraint item that
this constraint came from was annotated with `:: "ADRows"`, and the specific
`all_different` constraint was annotated with `:: "AD(row \(i))"` Constraint
and expression annotations are useful for quickly interpreting the MUSes.


```
Traces:
/mnt/vault/research/cglol/coreguidedlol/exploration/models/seat-moving/seat-moving-core.mzn|112|2|117|2|ca|forall;/mnt/vault/research/cglol/coreguidedlol/exploration/models/seat-moving/seat-moving-core.mzn|112|2|117|2|ac;/mnt/vault/research/cglol/coreguidedlol/exploration/models/seat-moving/seat-moving-core.mzn|112|9|112|9|i=1;/mnt/vault/research/cglol/coreguidedlol/exploration/models/seat-moving/seat-moving-core.mzn|112|31|112|31|p=1;/mnt/vault/research/cglol/coreguidedlol/exploration/models/seat-moving/seat-moving-core.mzn|113|6|116|9|bin|'->';/mnt/vault/research/cglol/coreguidedlol/exploration/models/seat-moving/seat-moving-core.mzn|113|6|116|9|ca|clause
```

The `Traces` section lists the constraint paths of the constraints in the MUS.
Paths are constructed from the stack-trace of the compiler when a constraint is
being added to the FlatZinc.  Stack-frames are separated by a semi-colon `;`
and typically take the form: `path|sl|sc|el|ec|type|name` where `path` is the
path to a file; `sl`, `sc`, `el`, and `ec` are the start-line, start-column,
end-line, and end-column respectively; `type` refers to the type of expression,
typically this will be `ca` for `Call`, `bin` for `BinOp`; `name` is the name
of the call or operation (e.g. `all_different`, `->`, etc...). Another special
`type` to be aware of is the `ac` type. This represents an array comprehensions
(loop construct).  These frames do not have a `name` field but the following
frames will have a type field of `x=v` representing the assignments to loop
index variables.

