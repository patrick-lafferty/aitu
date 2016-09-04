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

#include "Planner.h"
#include "ConditionOpSatisfies.h"
#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include "UE_Replacements.h"
#include "log.h"

#define SHOW_PLANNER_INFORMATION_MESSAGES

namespace AI
{
	void Plan::start()
	{
		currentTask = begin(tasks);
		hasCurrentTask = true;

		currentPathVertex = begin(planPath);
	}	
	
	/*
	note: should take into account the effects one task will have on the world when deciding whether the next
	task's precondition is valid, even though we won't know if the world's state will actually change until
	the effects are performed by some controller and then observed by the sensors

	also have to undo the effects if that path in the network failed
	*/

	struct TaskNode
	{
		TaskIdentifier identifier;
		int priority {0};

		TaskNode(TaskIdentifier id, int p)
			: identifier{id}, priority{p} {}
	};

	bool operator>(const TaskNode& lhs, const TaskNode& rhs)
	{
		return lhs.priority > rhs.priority;
	}

	int heuristic(Condition& remaining)
	{
		return
			remaining.requiredFlags.size()
			+ remaining.requiredValues.size()
			+ remaining.consumableFlags.size()
			+ remaining.consumableValues.size()
			+ remaining.consumableVectors.size();
	}

	int heuristic(FMergedCondition& condition)
	{
		return heuristic(condition.condition) + condition.satisfiedPredicates.size();
	}

	typedef bool (*Satisfier)(float a, ConditionOp op, float value);
	static Satisfier satisfiers[] =
	{
		&canLessThanSatisfy,
		&canGreaterThanSatisfy,
		&canEqualToSatisfy,
		&canNotEqualToSatisfy,
		&canLessEqualSatisfy,
		&canGreaterEqualSatisfy
	};

	void mergeConsumables(std::vector<ConsumableFact>& current, std::vector<ConsumableFact>& nextPost,
		std::vector<ConsumableFact>& nextPre)
	{
		for (auto consumable : nextPost)
		{
			auto it = std::find(begin(current), end(current), consumable);
			
			if (it != end(current))
			{
				auto index = it - begin(current);
				std::swap(current[index], current.back());
				current.pop_back();
			}
		}

		current.reserve(current.size() + nextPre.size());
		current.insert(end(current), begin(nextPre), end(nextPre));
	}

	Condition mergeConditions(Task& next, Condition current)
	{
		for (auto flag : next.postconditions.requiredFlags)
		{
			auto it = std::find_if(begin(current.requiredFlags), end(current.requiredFlags),
				[flag](auto& f){return f.id == flag.id;});
			auto index = it - begin(current.requiredFlags);

			if (it != end(current.requiredFlags)
				&& current.requiredFlags[index].flag == flag.flag)
			{
				current.requiredFlags.erase(begin(current.requiredFlags) + index);
			}
		}

		current.requiredFlags.insert(end(next.preconditions.requiredFlags), begin(next.preconditions.requiredFlags),
			end(next.preconditions.requiredFlags));

		for (auto value : next.postconditions.requiredValues)
		{
			auto it = std::find_if(begin(current.requiredValues), end(current.requiredValues),
				[value](auto& v){return v.id == value.id;});

			if (it != end(current.requiredValues))
			{
				auto index = it - begin(current.requiredValues);
				bool canSatisfy = satisfiers[static_cast<int>(value.op)](value.value, current.requiredValues[index].op, current.requiredValues[index].value);

				if (canSatisfy)
				{
					current.requiredValues.erase(begin(current.requiredValues) + index);
				}
			}
		}

		current.requiredValues.insert(end(current.requiredValues), begin(next.preconditions.requiredValues),
			end(next.preconditions.requiredValues));
		
		mergeConsumables(current.consumableFlags, next.postconditions.consumableFlags, next.preconditions.consumableFlags);
		mergeConsumables(current.consumableValues, next.postconditions.consumableValues, next.preconditions.consumableValues);
		mergeConsumables(current.consumableVectors, next.postconditions.consumableVectors, next.preconditions.consumableVectors);

		return current;
	}

	FMergedCondition mergeConditions(Task& next, FMergedCondition current)
	{
		current.condition = mergeConditions(next, current.condition);

		for (auto satisfied : next.postconditions.satisfiedPredicates)
		{
			auto it = std::find_if(begin(current.satisfiedPredicates), end(current.satisfiedPredicates), 
				[satisfied](auto& sp){return sp.identifier == satisfied.identifier;});
			
			if (it != end(current.satisfiedPredicates))
			{
				auto index = it - begin(current.satisfiedPredicates);
				std::swap(current.satisfiedPredicates[index], current.satisfiedPredicates.back());
				current.satisfiedPredicates.pop_back();
			}
		}

		current.satisfiedPredicates.reserve(current.satisfiedPredicates.size() + next.preconditions.satisfiedPredicates.size());
		
		int size = current.tasks.size();
		for(auto& sp : next.preconditions.satisfiedPredicates)
		{
			current.satisfiedPredicates.push_back({sp.identifier, sp.index, size});
		}

		current.tasks.push_back(next);

		return current;
	}

	std::vector<TaskIdentifier> constructPath(std::unordered_map<TaskIdentifier, TaskIdentifier> cameFrom, 
		TaskIdentifier start, TaskIdentifier goal)
	{
		std::vector<TaskIdentifier> path;

		path.push_back(start);

		while(start != goal)
		{
			start = cameFrom[start];
			path.push_back(start);
		}

		return path;
	}	

	int finalizePlanImpl(Plan& plan, int index, Task& task)
	{	
		if (task.type == TaskType::Compound
			|| task.type == TaskType::Recursive)
		{
			plan.tasks.insert(begin(plan.tasks) + index, task);

			auto parentIndex = plan.tasks.size() - 1;
			index++;

			for (auto& subtask : task.subtasks)
			{
				subtask.parentTaskIndex = parentIndex;
				index = finalizePlanImpl(plan, index, subtask);
			}
		}
		else if (task.type == TaskType::Abstract)
		{
			if (!plan.tasks.empty())
			{
				auto parentIndex = plan.tasks.size();
				plan.tasks.back().parentTaskIndex = parentIndex;			
			}

			plan.tasks.insert(begin(plan.tasks) + index, task);
		}
		else
		{
			plan.tasks.insert(begin(plan.tasks) + index, task);
		}

		return index + 1;
	}

	void finalizePlanImpl(Plan& plan, Task& task, std::unordered_map<TaskIdentifier, int>& implementationIndex)
	{	
		if (task.type == TaskType::Compound
			|| task.type == TaskType::Recursive)
		{
			plan.tasks.push_back(task);

			auto parentIndex = plan.tasks.size() - 1;

			for (auto& subtask : task.subtasks)
			{
				subtask.parentTaskIndex = parentIndex;
				finalizePlanImpl(plan, subtask, implementationIndex);
			}
		}
		else if (task.type == TaskType::Abstract)
		{
			if (!plan.tasks.empty())
			{
				auto parentIndex = plan.tasks.size();
				plan.tasks.back().parentTaskIndex = parentIndex;			

				plan.implementationsUsed[parentIndex].push_back(implementationIndex[task.identifier]);
			}

			plan.tasks.push_back(task);
		}
		else
		{
			plan.tasks.push_back(task);
		}
	}

	void finalizePlan(Plan& plan, std::vector<TaskIdentifier>& identifiers, TaskDatabase& taskDatabase, 
		std::unordered_map<TaskIdentifier, int>& implementationIndex)
	{
		for(auto identifier : identifiers)
		{
			finalizePlanImpl(plan, taskDatabase.tasks[identifier], implementationIndex);
		}	

		plan.planPath = identifiers;
	}

	FMergedCondition toMergedCondition(Condition condition, Task task)
	{
		FMergedCondition merged;

		merged.condition = condition;

		for(auto& sp : condition.satisfiedPredicates)
		{
			merged.satisfiedPredicates.push_back({sp.identifier, sp.index, 0});
		}

		merged.tasks.push_back(task);

		return merged;
	}
	
	void generatePlanImpl(Plan& plan, Task& initialTask, WorldState& currentState, TaskParameters& parameters, 
		TaskDatabase& taskDatabase, int parentTaskIndex)
	{
		auto start = initialTask.identifier;

		std::priority_queue<TaskNode, std::vector<TaskNode>, std::greater<TaskNode>> frontier;
		std::unordered_map<TaskIdentifier, TaskIdentifier> cameFrom;
		std::unordered_map<TaskIdentifier, int> costSoFar;
		std::unordered_map<TaskIdentifier, FMergedCondition> remainingPreconditions;
		std::unordered_map<TaskIdentifier, int> implementationIndex;

		int remainingSatisfiableStateCount = 0;
		auto taskImplementations = taskDatabase.abstractTaskImplementations.find(initialTask.identifier);
		bool generatingForAbstractGoal {false};
		TaskIdentifier currentAbstractImplementation {TaskIdentifier::Null};

		if (taskImplementations != end(taskDatabase.abstractTaskImplementations))
		{
			//the goal is an abstract task, add all the implementations to the queue
			//instead of the abstract task itself
			
			for(auto& task : taskImplementations->second)
			{
				frontier.emplace(task.identifier, 0);
				cameFrom[task.identifier] = initialTask.identifier;
				costSoFar[task.identifier] = 0;
				remainingPreconditions[task.identifier] = toMergedCondition(task.preconditions, task);
			}

			start = taskImplementations->second[0].identifier;	
			
			generatingForAbstractGoal = true;
			currentAbstractImplementation = start;
		}
		else
		{
			frontier.emplace(start, 0);
			remainingPreconditions[start] = toMergedCondition(initialTask.preconditions, initialTask);
		}		

		cameFrom[start] = start;
		costSoFar[start] = 0;	

		TaskNode current = frontier.top();
		while (!frontier.empty())
		{
			current = frontier.top();
			frontier.pop();

			if (generatingForAbstractGoal)
			{
				auto it = std::find_if(begin(taskImplementations->second), end(taskImplementations->second),
					[=](const Task& task) {return task.identifier == current.identifier;});

				if (it != end(taskImplementations->second))
				{
					currentAbstractImplementation = current.identifier;
				}
			}
		
			//is this task abstract && not the first one?
			if (current.identifier != initialTask.identifier)
			{
				auto implementations = taskDatabase.abstractTaskImplementations.find(current.identifier);
				if (implementations != end(taskDatabase.abstractTaskImplementations))
				{
					int i = 0;
					for(auto& implementation : implementations->second)
					{
						frontier.emplace(implementation.identifier, 0);
						cameFrom[implementation.identifier] = current.identifier;
						costSoFar[implementation.identifier] = 0;
						remainingPreconditions[implementation.identifier] = toMergedCondition(implementation.preconditions, implementation);
						implementationIndex[implementation.identifier] = i;
						i++;
					}

					continue;
				}
			}

			auto preconditions = remainingPreconditions[current.identifier];
			remainingSatisfiableStateCount = heuristic(preconditions);					

			if (remainingSatisfiableStateCount == 0
				|| preconditions.isTrue(currentState, taskDatabase.tasks[current.identifier], taskDatabase.satisfiablePredicates))
			{
				break;
			}

			auto vertex = std::find(begin(taskDatabase.taskGraph), end(taskDatabase.taskGraph), TaskVertex{current.identifier});

			for (auto& adjacentVertex : vertex->adjacentTasks)
			{
				auto& next = taskDatabase.tasks[adjacentVertex];

				int new_cost = costSoFar[current.identifier] + 1;

				if (!costSoFar.count(adjacentVertex) || new_cost < costSoFar[adjacentVertex])
				{					
					auto mergedConditions = mergeConditions(next, remainingPreconditions[current.identifier]);					
					int priority = heuristic(mergedConditions);

					auto adjacent = std::find(begin(taskDatabase.taskGraph), end(taskDatabase.taskGraph), TaskVertex{adjacentVertex});

					if (adjacent->adjacentTasks.empty()
						&& priority > 0
						&& !mergedConditions.isTrue(currentState, next, taskDatabase.satisfiablePredicates))
					{
						//this is a dead end, don't follow
						continue;
					}

					priority += new_cost;
					//this cameFrom stuff isnt going to work if there are multiple instances of the TaskIdentifier in the graph
					//eg lots of MoveTo or PlayAnimation. instead of taskIdentifier it should be an index or something
					costSoFar[adjacentVertex] = new_cost;
					frontier.emplace(adjacentVertex, priority);
					cameFrom[adjacentVertex] = current.identifier;
					remainingPreconditions[adjacentVertex] = mergedConditions;
				}
			}
		}

		if (remainingSatisfiableStateCount == 0
			|| remainingPreconditions[current.identifier].isTrue(currentState, taskDatabase.tasks[current.identifier], taskDatabase.satisfiablePredicates))
		{
			if (generatingForAbstractGoal)
			{
				start = currentAbstractImplementation;
			}

			auto path = constructPath(cameFrom, current.identifier, start);
			finalizePlan(plan, path, taskDatabase, implementationIndex);
		}
		else
		{
			plan.failed = true;
		}
	}	

	Plan generatePlan(Task& initialTask, WorldState& currentState, const WorldQuerier& worldQuerySystem, TaskParameters& parameters, TaskDatabase& taskDatabase)
	{
		Plan plan;
		generatePlanImpl(plan, initialTask, currentState, parameters, taskDatabase, -1);

		if (plan.failed)
			return plan;

		plan.currentTask = begin(plan.tasks);

		if (plan.currentTask->setup)
		{
			plan.currentTask->setup(*plan.currentTask, currentState, worldQuerySystem);
		}

		return plan;
	}

	void evaluatePlan(Plan& plan, WorldState& currentState, WorldQuerier const& worldQuerySystem, TaskParameters& parameters, std::vector<SatisfiablePredicate>& satisfiablePredicates)
	{
		//is this an abstract or compound/.recursive task? they don't have any actions, so we can skip ahead		
		if (plan.currentTask->type != TaskType::Simple)			
		{
			if (!plan.currentTask->preconditions.isTrue(currentState, *plan.currentTask, satisfiablePredicates))
			{
				log("[", worldQuerySystem.getName(), "] ", "Compound task: ", plan.currentTask->debugName, " preconditions were not met");
				plan.failed = true;
				return;
			}

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
			log("[", worldQuerySystem.getName(), "] ", "skipping non-simple task: ", plan.currentTask->debugName);
#endif

			if (plan.currentTask->setup)
			{
				plan.currentTask->setup(*plan.currentTask, currentState, worldQuerySystem);
			}

			if (plan.currentTask->loop)
			{
				plan.currentTask->loop(currentState, worldQuerySystem, *plan.currentTask);
			}

			plan.currentTask++;	

			if (plan.currentTask->identifier != TaskIdentifier::Null
				&& *plan.currentPathVertex != plan.currentTask->identifier)
			{
				plan.currentPathVertex++;
			}

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
			log("[", worldQuerySystem.getName(), "] ", "Started task: ", plan.currentTask->debugName);
#endif

			if (plan.currentTask->setup)
			{
				plan.currentTask->setup(*plan.currentTask, currentState, worldQuerySystem);
			}

			return;
		}

		//are we done with the current task?
		if ((plan.currentTask->postconditions.isTrue(currentState, *plan.currentTask, satisfiablePredicates)
			|| (!plan.currentTask->breakConditions.isEmpty() && plan.currentTask->breakConditions.isTrue(currentState, *plan.currentTask, satisfiablePredicates)))
			&& !plan.currentTask->failed)
		{	
#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
			log("[", worldQuerySystem.getName(), "] ", "Finished task: ", plan.currentTask->debugName);
#endif

			if (plan.currentTask->finish)
			{
				plan.currentTask->finish(currentState, *plan.currentTask);
			}

			//is this the last task in an abstract implementation or a compound task?
			if (plan.currentTask->parentTaskIndex != -1)
			{		
				auto& parent = plan.tasks[plan.currentTask->parentTaskIndex];		

				if (parent.type == TaskType::Abstract) 
				{
					if (!parent.postconditions.isTrue(currentState, parent, satisfiablePredicates))
					{
						log("[", worldQuerySystem.getName(), "] ", "Abstract task: ", parent.debugName, " finished but its postconditions are not met");
						plan.failed = true;

						return;
					}
				}
				else if (parent.type == TaskType::Compound && plan.currentTask->isLastInCompound) 
				{
					if (!parent.postconditions.isTrue(currentState, parent, satisfiablePredicates)) 
					{
						log("[", worldQuerySystem.getName(), "] ", "Compound task: ", parent.debugName, " finished but its postconditions were not met");
						plan.failed = true;

						return;
					}
				}
				else if (parent.type == TaskType::Recursive && plan.currentTask->isLastInCompound)
				{
					if (!parent.postconditions.isTrue(currentState, parent, satisfiablePredicates)) 
					{
						if (parent.remainingRepeats > 0)
						{
							parent.remainingRepeats--;

							log("[", worldQuerySystem.getName(), "] ", "Recursive task: ", parent.debugName, " finished, starting over since postconditions were not met");
							plan.currentTask = begin(plan.tasks) + plan.currentTask->parentTaskIndex;
							evaluatePlan(plan, currentState, worldQuerySystem, parameters, satisfiablePredicates);
						}
						else
						{
							log("[", worldQuerySystem.getName(), "] ", "Recursive task: ", parent.debugName, " failed, postconditions not met and remainingRepeats");
							plan.failed = true;
						}

						return;
					}
					else
					{
						log("[", worldQuerySystem.getName(), "] ", "Recursive task: ", parent.debugName, " has finished");
					}
				}				
			}

			//is there a next task?
			if ((plan.currentTask + 1) < end(plan.tasks))
			{
				//can we move to the next task?
				plan.currentTask++;

				if (plan.currentTask->identifier != TaskIdentifier::Null
					&& *plan.currentPathVertex != plan.currentTask->identifier)
				{
					plan.currentPathVertex++;
				}

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
				log("[", worldQuerySystem.getName(), "] ", "Started task: ", plan.currentTask->debugName);
#endif

				if (plan.currentTask->setup)
				{
					plan.currentTask->setup(*plan.currentTask, currentState, worldQuerySystem);
				}

				if (plan.currentTask->preconditions.isTrue(currentState, *plan.currentTask, satisfiablePredicates)) 
				{
					return;
				}
				else
				{
					log("[", worldQuerySystem.getName(), "] ", plan.currentTask->debugName, " preconditions false, plan failed");
					plan.failed = true;					
				}
			}
			else
			{

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
				log("[", worldQuerySystem.getName(), "] ", "Plan finished successfully");
#endif

				plan.failed = false;
				plan.finished = true;			
			}			
		}
		else
		{
			//can we still do this task?
			if (plan.currentTask->preconditions.isTrue(currentState, *plan.currentTask, satisfiablePredicates) 
				&& !plan.currentTask->isImmediate && !plan.currentTask->failed)
			{
				return;
			}
			else
			{
				log("[", worldQuerySystem.getName(), "] ", plan.currentTask->debugName, " preconditions false, plan failed");
				plan.failed = true;
			}
		}
	}

	void executeFinally(Plan& plan, WorldState& currentState) 
	{
		if (plan.finished && !plan.failed)
		{
			for (auto it = rbegin(plan.tasks); it != rend(plan.tasks); ++it)
			{
				if (it->finally)
				{
					it->finally(currentState);					
				}
			}

			return;
		}

		if (!plan.hasCurrentTask)
			return;					

		for(auto it = plan.currentTask; it != begin(plan.tasks); --it)
		{
			if (it->finally)
			{
				it->finally(currentState);
			}
		}
	}

	void fixFailedPlan(Plan& plan, WorldState& currentState, TaskParameters& parameters, TaskDatabase& taskDatabase)
	{
		//walk forwards through the plan to try to find an abstract task
		if (!plan.hasCurrentTask)
			return;

		auto task = *plan.currentTask;

		if (plan.tasks[task.parentTaskIndex].type == TaskType::Abstract)
		{
			//try another implementation
			auto& implementationsUsed = plan.implementationsUsed[task.parentTaskIndex];

			int implementationIndex = 0;
			auto abstractTask = plan.tasks[task.parentTaskIndex];

			for (auto& implementation : taskDatabase.abstractTaskImplementations[abstractTask.identifier])
			{
				auto it = std::find(begin(implementationsUsed), end(implementationsUsed), implementationIndex);

				if (it == end(implementationsUsed))
				{
					//we havent used this implementation yet, check if its usable
					if (implementation.preconditions.isTrue(currentState, implementation, taskDatabase.satisfiablePredicates))
					{
						plan.failed = false;
						implementationsUsed.push_back(implementationIndex);
						auto parentTaskIndex = task.parentTaskIndex;

						auto currentTaskIndex = plan.currentTask - begin(plan.tasks);
						plan.tasks.erase(begin(plan.tasks) + currentTaskIndex);
						auto index = finalizePlanImpl(plan, currentTaskIndex, implementation);
						plan.tasks[currentTaskIndex].parentTaskIndex = parentTaskIndex + (index - currentTaskIndex - 1) - 1;
						plan.currentTask = begin(plan.tasks) + currentTaskIndex;

						//generatePlanImpl(plan, implementation, currentState, parameters, taskDatabase, task.parentTaskIndex);
						break;
					}
				}

				implementationIndex++;
			}
		}

		if (!plan.failed)
		{
			plan.hasCurrentTask = true;
		}
	}	

	void printTasksImpl(Plan& plan, int taskIndex, int currentParentIndex, std::stack<int> tabs)
	{
		if (taskIndex < static_cast<int>(plan.tasks.size()))
		{
			auto& task = plan.tasks[taskIndex];

			if (task.parentTaskIndex > tabs.top())
			{
				tabs.push(task.parentTaskIndex);
			}
			else if (task.parentTaskIndex < currentParentIndex)
			{				
				while (!tabs.empty() && tabs.top() != task.parentTaskIndex)
				{
					tabs.pop();
				}
			}
	
			printTasksImpl(plan, taskIndex + 1, task.parentTaskIndex, tabs);
		}
	}

	void printTasks(Plan& plan, int tabs)
	{
		std::stack<int> tab;
		tab.push(-1);
		printTasksImpl(plan, 0, 0, tab);
	}
}

#ifdef SHOW_PLANNER_INFORMATION_MESSAGES
#undef SHOW_PLANNER_INFORMATION_MESSAGES
#endif