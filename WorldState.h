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
#include <map>
#include "Memory.h"
#include "ValueOverTimeTracker.h"
#include "Barker.h"
#include "Math.h"

class UBarker;

enum class Stance
{
	Standing,
	Sitting
};

/*
Name for values stored in state.flag/value/vector maps
*/
enum class WorldStateIdentifier
{
	Alertness,		
	Boredom,
	Curiosity,
	PlayerIdentified,
	Destination,
	CurrentPosition,
	CurrentPosition_Feet,
	PlayerPosition,	
	Stance,	
	Invalid
};

//update Consideration.h::ConsumableFact when changing this!
enum class ConsumableFact
{
	Player_LastKnownLocation,
	Player_ForwardVector,
	NoiseDisturbance,
	Glimpse,
	HasLead,
	LostLead,
	Lead,
	PlayerRecentlyIdentified,
	HasPickedSeat,
	IsPositionedToSit
};

namespace AI
{	
	struct State
	{
		std::map<WorldStateIdentifier, bool> flags;
		std::map<WorldStateIdentifier, float> values;
		std::map<WorldStateIdentifier, Math::Vector3> vectors;
	};

	struct StateChanges
	{
		State state;
		std::map<WorldStateIdentifier, bool> valuesAddedOrRemoved; //true = added, false = removed
	};

	/*
	Facts are similar to State values, with the added constraint that facts can only be used/consumed once.
	After they are consumed, you need to produce the fact again.

	For example if you had a Fact HasClue, and the AI then investigates that clue (consuming it in the process),
	it no longer has a clue. (You're not going to think "Hey I should check out that thing over there",
	then after investigating that thing, "Hey I should check out this thing over here").
	*/
	struct FactFlag
	{
		bool flag;
		bool consumed {true}; //if true then flag can't be used anymore, its considered stale.
	};

	struct FactValue
	{
		float value;
		bool consumed {true};  //if true then value can't be used anymore, its considered stale.
	};

	struct FactVector
	{
		Math::Vector3 value;
		bool consumed {true}; //if true then value can't be used anymore, its considered stale.
	};

	struct Facts
	{
		std::map<ConsumableFact, FactFlag> flags;
		std::map<ConsumableFact, FactValue> values;
		std::map<ConsumableFact, FactVector> vectors;
	};

	struct WorldState
	{
		State current;
		Facts facts;
		Memory memory;
		Barker barker;

		/*
		consumeX gives you the value of the fact and sets its consumed value to true so it can't be used 
		until produced again.
		*/
		FactFlag consumeFactFlag(ConsumableFact fact);
		FactValue consumeFactValue(ConsumableFact fact);
		FactVector consumeFactVector(ConsumableFact fact);

		/*
		produceX stores a new value for a fact and setes its consumed to false, indicating it can
		be consumed
		*/
		void produceFactFlag(ConsumableFact fact, bool flag);
		void produceFactValue(ConsumableFact fact, float value);
		void produceFactVector(ConsumableFact fact, Math::Vector3 vector);

		void pushChanges(struct Task& task);
		void popChanges();

		std::vector<StateChanges> changes;
		std::map<WorldStateIdentifier, ValueOverTimeTracker> valueTrackers;
		std::vector<Actor*> actorsToIgnore;
		bool actorsToIgnoreChanged {false};
	};
}