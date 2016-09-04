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

#include <map>
#include "WorldState.h"
#include <vector>
#include <functional>
#include "Condition.h"

namespace AI
{

	enum class TaskIdentifier
	{		
		Browse,
		Wander,
		Chase,
		ChaseSight,
		ChaseLastKnownPosition,
		ChaseLastKnownHeading,
		Bother,
		Search,
		Stare,
		FaceSound,
		InvestigateSound,
		FollowPath,
		Relax,
		Null,
		FindSeat,
		FindSeat_PickNearbySeat,
		FindSeat_SearchAreaForSeat,
		StartConversation,
		JoinConversation,
		React_Dismiss,
		React_FoundPlayer,
		React_LostPlayer,
		React_Sight,
		React_Sound,
		TrackHead
	};

	enum class Action
	{		
		SelectDestination = 1,
		MoveToPlayer,
		MoveToDestination,
		MoveToActor,
		StopMoving,		
		Wait,
		BotherPlayer,
		Bark,
		PlayMontage,
		PlayAnimation,
		FocusPlayer,
		LookAt,		
	};

	enum class Bark;	

	enum class Montage
	{
		Stare,
		LookAround
	};

	enum class DestinationType
	{
		RandomLocation,
		RandomStack,
		AnyNearbySeat
	};

	/*
	Tasks can be primitive, compound or abstract

	Primitive tasks have an action
	Compound tasks have a sequence of sub-tasks, executed in order
	Abstract tasks have one or more Primitive/Compound implementations, which get selected based on first-come preconditions

	Only abstract tasks need TaskIdentifiers, since its used to look up implementations,
	or top-level tasks that will be reused in other tasks
	*/
	enum class TaskType
	{
		Simple,
		Compound,
		Abstract,
		Recursive
	};

	//stores instance specific parameters for Tasks
	//eg you can define a Walk task, and create two copies of it with different speed parameters
	struct TaskParameters
	{
		std::vector<Math::Vector3> vectors;
		std::vector<float> values;
		std::vector<bool> flags;
	};	

	struct Task
	{		
		Task(std::string debugName, TaskType type = TaskType::Simple)
			: type{type}, debugName{debugName}
			{}
		
		Task() : Task("unnamed") {}

		//all preconditions must be true in order to start the task
		Condition preconditions;
		//all postconditions must be true in order to sucessfully complete the task
		Condition postconditions;
		//provides an alternate set of postconditions to enable an early-out,
		//so a task can successfuly complete if (all postconditions are true) or (break conditions are true)
		Condition breakConditions;
				
		TaskType type;
		std::vector<Task> subtasks;
		
		int parentTaskIndex {-1};
		//how many times we can repeat this task(and its subtasks) in a loop
		int maxRepeats{ 0 };		
		int remainingRepeats{ 0 };

		std::map<WorldStateIdentifier, bool> modifiedFlags;
		std::map<WorldStateIdentifier, float> modifiedValues;
		std::map<WorldStateIdentifier, Math::Vector3> modifiedVectors;

		Action action;
		TaskIdentifier identifier {TaskIdentifier::Null};

		//called once when starting this task		
		std::function<void(Task&, WorldState&, class WorldQuerier const&)> setup;
		
		//called once when the task successfully finishes
		std::function<void(WorldState&, Task&)> finish;	

		//guaranteed to called once when switching away from this task, either
		//because it finished or failed
		std::function<void(WorldState&)> finally;

		//called once per frame
		std::function<void(WorldState&, WorldQuerier const&, Task&)> loop;

		std::string debugName;

		bool isImmediate{ false };
		bool isLastInCompound{ false };
		bool failed {false};

		void addSubtask(Task& task);

		//store instance specitic task params here.
		//eg: for a walk task, speed could be stored here
		TaskParameters parameters;
	};

	//used to determine task adjacency
	//tasks A and B are considered adjacent if A's postconditions meet one of B's preconditions
	struct TaskVertex
	{
		TaskIdentifier identifier;
		std::vector<TaskIdentifier> adjacentTasks;
	};

	bool operator==(const TaskVertex& lhs, const TaskVertex& rhs);

	struct FMergedCondition
	{
		Condition condition;

		std::vector<MergedSatisfiablePredicateParams> satisfiedPredicates;
		std::vector<Task> tasks;

		bool isTrue(const WorldState& state, const Task& task, const std::vector<SatisfiablePredicate>& satisfiablePredicates);
	};

}