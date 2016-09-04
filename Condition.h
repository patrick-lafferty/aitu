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
#include <functional>
#include "WorldState.h"

namespace AI
{
    /*
	There are a few predicates tasks can use to evaluate a condition, 
	these identifiers are how you refer to those
	*/
	enum class SatisfiablePredicateIdentifier
	{
		NearPlayer,
		NearDestination,
		NearActor,
		TimeElapsed,
		ParameterFlag,
		Blackboard_ValueNotNull,
		ReactionFinished
	};

	enum class ConditionOp
	{
		LessThan,
		LessEqual,
		EqualTo,
		NotEqualTo,
		GreaterThan,
		GreaterEqual,		
	};

    /*
    requires state.current.flags[id] == flag
    */
	struct RequiredFlag
	{
		WorldStateIdentifier id;
    	bool flag;
	};

    /*
    requires state.current.values[id] op value
    */
	struct RequiredValue
	{
		WorldStateIdentifier id;
		ConditionOp op;
		float value;
	};

	struct SatisfiablePredicateParams
	{
		SatisfiablePredicateIdentifier identifier;

		int index;	//index into task.parameters.vectors, if necessary the Z value will be an index to the next vector to take params from
	};

	struct MergedSatisfiablePredicateParams
	{
		SatisfiablePredicateIdentifier identifier;

		int index;	//index into task.parameters.vectors, if necessary the Z value will be an index to the next vector to take params from
		int taskIndex; //index into an array of task.parameters
	};

    struct Task;

    /*
    A predicate function that can examine the complete WorldState (as a particular AI character sees it)
    along with all the parameters in the task this predicate's condition belongs to. Much more flexible
    than the other Condition expressions as you can define exactly how its evaluated.
    */
	struct SatisfiablePredicate
	{
		SatisfiablePredicateIdentifier identifier;
		std::function<bool(const WorldState&, const Task&, int parameterIndex)> func;
	};

    /*
    A Condition is a set of expressions that must all be true for the condition to be
    considered true overall. Tasks have preconditions (a condition that must be true 
    in order for the task to be started) and postconditions (a condition that must be true
    in order for the task to complete sucessfully and not fail the plan).
    */
	struct Condition
	{
		//requiredFlag/Values: X must have value Y in order for tthis condition to be true
		std::vector<RequiredFlag> requiredFlags;
		std::vector<RequiredValue> requiredValues;

		std::vector<SatisfiablePredicateParams> satisfiedPredicates;

		std::vector<ConsumableFact> consumableFlags;
		std::vector<ConsumableFact> consumableValues;
		std::vector<ConsumableFact> consumableVectors;

		bool isTrue(const WorldState& state, const Task& task, const std::vector<SatisfiablePredicate>& satisfiablePredicates, bool isMerged = false);					
		bool isEmpty();
	};

}