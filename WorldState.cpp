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

#include "WorldState.h"
#include "Tasks.h"

namespace AI
{
	FactFlag WorldState::consumeFactFlag(ConsumableFact fact)
	{
		auto it = facts.flags.find(fact);

		if (it == end(facts.flags) || it->second.consumed)
		{
			return {};
		}
		else
		{
			it->second.consumed = true;
			return it->second;
		}
	}

	FactValue WorldState::consumeFactValue(ConsumableFact fact)
	{
		auto it = facts.values.find(fact);

		if (it == end(facts.values) || it->second.consumed)
		{
			return {};
		}
		else
		{
			it->second.consumed = true;
			return it->second;
		}
	}

	FactVector WorldState::consumeFactVector(ConsumableFact fact)
	{
		auto it = facts.vectors.find(fact);

		if (it == end(facts.vectors) || it->second.consumed)
		{
			return {};
		}
		else
		{
			it->second.consumed = true;
			return it->second;
		}
	}

	void WorldState::produceFactFlag(ConsumableFact fact, bool flag)
	{
		facts.flags[fact] = {flag, false};
	}
	
	void WorldState::produceFactValue(ConsumableFact fact, float value)
	{
		facts.values[fact] = {value, false};		
	}

	void WorldState::produceFactVector(ConsumableFact fact, Math::Vector3 vector)
	{
		facts.vectors[fact] = FactVector{vector, false};		
	}

	void WorldState::pushChanges(Task& task)
	{
		auto copiedFlags = task.modifiedFlags;
		auto copiedValues = task.modifiedValues;
		auto copiedVectors = task.modifiedVectors;

		for (auto& flag : task.modifiedFlags)
		{			
			std::swap(copiedFlags[flag.first], current.flags[flag.first]);
		}

		for (auto& value : task.modifiedValues)
		{			
			std::swap(copiedValues[value.first], current.values[value.first]);
		}

		for (auto& vector : task.modifiedVectors)
		{
			std::swap(copiedVectors[vector.first], current.vectors[vector.first]);
		}

		StateChanges changed;
		changed.state.flags = std::move(copiedFlags);
		changed.state.values = std::move(copiedValues);
		changed.state.vectors = std::move(copiedVectors);

		changes.emplace_back(changed);
	}

	void WorldState::popChanges()
	{
		auto poppedChanges = changes[changes.size() - 1];
		changes.pop_back();

		for (auto& flag : poppedChanges.state.flags)
		{
			current.flags[flag.first] = flag.second;
		}

		for (auto& value : poppedChanges.state.values)
		{
			current.values[value.first] = value.second;
		}

		for (auto& value : poppedChanges.valuesAddedOrRemoved)
		{
			if (value.second)
			{
				current.values[value.first];
			}
			else
			{
				current.values.erase(value.first);
			}
		}
	}
}