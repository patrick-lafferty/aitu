/*
MIT License

Copyright (c) 2016 Patrick Lafferty

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <vector>

#include <stack>
#include <map>
#include "WorldState.h"
#include "Tasks.h"
#include "Utility.h"
#include "WorldQuerySystem/WorldQuerySystem.h"

/*
hierarhical task network planner
*/
namespace AI
{
	/*
	A Plan is a collection of tasks, and records any abstract implementations used by
	the plan incase the plan fails and a different implementation can be swapped in
	*/
	struct Plan
	{
		std::vector<Task> tasks;
		std::vector<Task>::iterator currentTask;
		std::vector<TaskIdentifier> planPath;
		std::vector<TaskIdentifier>::iterator currentPathVertex;
		std::map<int, std::vector<int>> implementationsUsed;
		bool failed{ false };
		bool finished{ false };
		bool hasCurrentTask {false};

		void start();
	};

	struct Planner
	{		
		Plan plan;
		TaskParameters parameters;		
	};

	Plan generatePlan(Task& initialTask, WorldState& currentState, const WorldQuerier& worldQuerySystem, 
		TaskParameters& parameters, TaskDatabase& taskDatabase);
	
	/*
	Checks if we can still continue with this plan, can we move on to the next task, did we fail or finish
	*/
	void evaluatePlan(Plan& plan, WorldState& currentState, const WorldQuerier& worldQuerySystem, 
		TaskParameters& parameters, std::vector<SatisfiablePredicate>& satisfiablePredicates);
	
	/*
	Try to fix a plan by seeing if we can replace one abstract task implementation with another
	*/
	void fixFailedPlan(Plan& plan, WorldState& currentState, TaskParameters& parameters, 
		TaskDatabase& taskDatabase);
	
	/*
	runs the finally function for any tasks that implement it
	*/
	void executeFinally(Plan& plan, WorldState& currentState);

	/*
	debug prints the tasks in a plan
	*/
	void printTasks(Plan& plan, int tabs = 0);
}