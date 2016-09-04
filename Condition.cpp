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

#include "Condition.h"
#include "Tasks.h"
#include "log.h"
#include <algorithm>

namespace AI
{
    bool Condition::isTrue(const WorldState& state, const Task& task, const std::vector<SatisfiablePredicate>& satisfiablePredicates, bool isMerged)
	{
		for (auto& requiredFlag : requiredFlags)
		{
			auto it = state.current.flags.find(requiredFlag.id);

			if (it == end(state.current.flags))
			{
				log("[Condition] ", "ERROR: flag missing");
			}

			if (it->second != requiredFlag.flag)
				return false;
		}

		for (auto& requiredValue : requiredValues)
		{
			auto it = state.current.values.find(requiredValue.id);

			if (it == end(state.current.values))
			{
				log("[Condition] ", "ERROR: flag missing");
			}

			switch (requiredValue.op)
			{
				case ConditionOp::LessThan
					:
				{					
					if (it->second >= requiredValue.value)
						return false;

					break;
				}
				case ConditionOp::LessEqual
					:
				{					
					if (it->second > requiredValue.value)
						return false;

					break;
				}
				case ConditionOp::EqualTo
					:
				{					
					if (it->second != requiredValue.value)
						return false;
					break;
				}
				case ConditionOp::NotEqualTo
					:
				{
					if (it->second == requiredValue.value)
						return false;
					break;
				}
				case ConditionOp::GreaterThan
					:
				{					
					if (it->second <= requiredValue.value)
						return false;

					break;
				}
				case ConditionOp::GreaterEqual
					:
				{					
					if (it->second < requiredValue.value)
						return false;

					break;
				}				
			}
		}		

		if (!isMerged)
		{
			for (auto satisfiable : satisfiedPredicates)
			{			
				const auto& pred = std::find_if(begin(satisfiablePredicates), end(satisfiablePredicates), 
					[satisfiable](const auto& sp) {return sp.identifier == satisfiable.identifier;});
				
				if (!pred->func(state, task, satisfiable.index))
				{
					return false;
				}
			}
		}

		for (auto consumable : consumableFlags)
		{
			auto it = state.facts.flags.find(consumable);

			if (it->second.consumed)
			{
				return false;
			}
		}

		for (auto consumable : consumableValues)
		{
			auto it = state.facts.values.find(consumable);

			if (it->second.consumed)
			{
				return false;
			}
		}

		for (auto consumable : consumableVectors)
		{
			auto it = state.facts.vectors.find(consumable);

			if (it->second.consumed)
			{
				return false;
			}
		}

		return true;
	}

	bool FMergedCondition::isTrue(const WorldState& state, const Task& task, const std::vector<SatisfiablePredicate>& satisfiablePredicates)
	{
		if (!condition.isTrue(state, task, satisfiablePredicates, true))
			return false;

		for (auto satisfiedPredicate : satisfiedPredicates)
		{
			const auto& pred = std::find_if(begin(satisfiablePredicates), end(satisfiablePredicates), 
				[satisfiedPredicate](const auto& sp) {return sp.identifier == satisfiedPredicate.identifier;});
			
			if (!pred->func(state, tasks[satisfiedPredicate.taskIndex], satisfiedPredicate.index))
			{
				return false;
			}
		}

		return true;
	}

	bool Condition::isEmpty()
	{
		return requiredFlags.size() == 0 
			&& requiredValues.size() == 0
			&& satisfiedPredicates.size() == 0
			&& consumableFlags.size() == 0
			&& consumableValues.size() == 0
			&& consumableVectors.size() == 0;
	}

}