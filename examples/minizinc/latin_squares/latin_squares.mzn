include "globals.mzn";

int: n;
set of int: N = 1..n;
array[N, N] of var N: X;

constraint forall (i in N) (alldifferent(row(X, i)) :: "elements of row \(i) should be different");
constraint forall (j in N) (alldifferent(col(X, j)) :: "elements of col \(j) should be different");

constraint forall (i in 1..n-1) (lex_less(row(X, i), row(X, i+1)) :: "row \(i) < row \(i+1)");
constraint forall (j in 1..n-1) (lex_greater(col(X, j), col(X, j+1)) :: "col \(j) > col \(j+1)");

solve satisfy;

output [ show2d(X) ];
