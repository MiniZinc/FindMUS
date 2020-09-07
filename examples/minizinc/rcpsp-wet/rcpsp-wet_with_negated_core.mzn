%-----------------------------------------------------------------------------%
% vim: ts=4 sw=4 et wm=0 tw=0
%-----------------------------------------------------------------------------%
% Copyright (C) 2009-2016 The University of Melbourne and NICTA.
% See the file COPYING for license information.
%-----------------------------------------------------------------------------%
% Model example for Resource-Constrained Project Scheduling Problems with
% Weighted Earliness/Tardiness objective (RCPSP/WET)
% 
% A RCPSP consists of resources, tasks, and precedences between some tasks 
% where resources have of a specific capacity and tasks need some capacity of 
% some resource to be executed.
% Here, we consider resources with a constant discrete capacity over time and
% tasks with a constant discrete duration and resource requirements.
% The objective is to find a optimal schedule so that tasks start as close as
% possible to the given start time for each task, penalizing earliness or 
% tardiness according to the given weight for earliness and tardiness per task.
%
%-----------------------------------------------------------------------------%

include "cumulative.mzn";

%-----------------------------------------------------------------------------%
% Model parameters.


    % Resources
    %
int: n_res;                     % The number of resources
set of int: Res = 1..n_res;     % The set of all resources
array [Res] of int: rc;         % The resource capabilities

    % Tasks
    %
int: n_tasks;                           % The number of tasks
set of int: Tasks = 1..n_tasks;         % The set of all tasks
array [Tasks]      of int       : d  ;  % The task durations
array [Res, Tasks] of int       : rr ;  % The resource requirements
array [Tasks]      of set of int: suc;  % The task successors

    % Deadlines
    %
    % deadline[i, 1] is the desired start time for task i,
    % deadline[i, 2] is the earliness cost per time unit of earliness,
    % deadline[i, 3] is the tardiness cost per time unit of tardiness.
array [Tasks, 1..3] of int: deadline;

    % Planning horizon
    %
    % Note that our RCPSP/WET instance generator requires a solution to the
    % equivalent RCPSP problem in order to generate the instances, so it gives 
    % us a planning horizon = the makespan of the RCPSP problem, plus 20% slop
int: t_max; %= sum(i in Tasks)(d[i]);     % End time of the planning horizon
set of int: Times = 0..(t_max - 1);     % Possible start times

%-----------------------------------------------------------------------------%
% Model variables.

array [Tasks] of var Times: s;  % The start times
var 0..sum(i in Tasks) (
    max(
        deadline[i, 2] * deadline[i, 1],
        deadline[i, 3] * (t_max - deadline[i, 1])
    )
): objective;

%-----------------------------------------------------------------------------%
% Constraints.

    % Precedence constraints
    %
constraint :: "Precedence constraints"
   forall ( i in Tasks, j in suc[i] )
   (
         ( s[i] + d[i] <= s[j] )
         :: "Task \(i) must finish before task \(j) starts"
   );

% Redundant non-overlapping constraints
%
%constraint :: "Redundant constraint"
%    redundant_constraint(
%        forall ( i, j in Tasks where i < j )
%        (
%            if exists(r in Res)(rr[r, i] + rr[r, j] > rc[r]) then
%                (   s[i] + d[i] <= s[j]
%                 \/ s[j] + d[j] <= s[i] )
%            else
%                true
%            endif
%        )
%    );

array[Res, Tasks] of var 0 .. max(rr[..,..])+1: orr :: add_to_output;

constraint :: "Resource requirements"
  forall (r in Res, i in Tasks) ((orr[r, i] >= rr[r, i] )
  ::"Task \(i) requires resource \(r)");

    % Cumulative resource constraints
    %
constraint :: "Cumulative resource requirements"
    forall ( r in Res )
    (
        let {
            set of int: RTasks = 
                            { i | i in Tasks 
                            where rr[r, i] > 0 /\ d[i] > 0 },
            int: sum_rr = sum(i in RTasks)(rr[r, i])
        } in (
            if RTasks != {} /\ sum_rr > rc[r] then
                cumulative(
                    [ s[i] | i in RTasks ],
                    [ d[i] | i in RTasks ],
                    [ orr[r,i] | i in RTasks ],
                    rc[r]
                )
                :: "Cumulative constraint for tasks requiring resource \(r)"
            else
                true
            endif
        )
    );

    % Weighted Earliness/Tardiness objective
constraint
    objective = sum (i in Tasks) (
        % earliness
        deadline[i, 2] * max(0, deadline[i, 1] - s[i]) +
        % tardiness
        deadline[i, 3] * max(0, s[i] - deadline[i, 1])
    );

%-----------------------------------------------------------------------------%
% Objective.

solve
   :: int_search( s, first_fail, indomain_min, complete )
   minimize objective;

%-----------------------------------------------------------------------------%

output [
    "s = \(s);\n",
    "objective = \(objective);\n",
];

%-----------------------------------------------------------------------------%
%-----------------------------------------------------------------------------%



constraint :: "Core from solver (Negated)"
   (max(0, deadline[8, 1] - s[8])   + max(0, s[21] - deadline[21, 1]) < 5)
   :: "Penalty for scheduling task 8 earlier, and task 21 later can be less than 5"
/\ (max(0, s[27] - deadline[27, 1]) + max(0, deadline[17, 1] - s[17]) < 2)
   :: "Penalty for scheduling task 17 earlier, and task 27 later can be less than 2"
/\ (max(0, s[32] - deadline[32, 1]) < 1)
   :: "Task 32 can be scheduled without incurring a penalty"
;
