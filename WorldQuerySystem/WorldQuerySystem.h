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

#include "EnvironmentQueries.h"
#include "Math.h"
#include <vector>
#include <algorithm>
#include "../UE_Replacements.h"

namespace AI
{
	/*
	Used by Tasks to query all the actors in the world, and of the task's parent Actor
	for specific values.
	*/
	class WorldQuerier
	{
	public:

		void setup(World* world, Actor* owner);

		/*
		the 5 getX()s return the X properties from the Actor owner of this querier
		*/
		Math::Vector3 getForwardVector() const;	
		Math::Vector3 getRightVector() const;
		Math::Vector3 getUpVector() const;
		World* getWorld() const;
		std::string getName() const;

		/*
		Scores all of the actors in the world by passing them to the query function,
		culling any actors with a negative score, then sorts the remaining actors.

		Example uses could be finding all actors that are chairs, all actors within
		x units of some location, the closest actor to some point/other actor.
		*/
		template<class Query>
		QueryResult queryEnvironment(Query query, QueryRunMode runMode)
		{
			QueryResult result;

			std::vector<std::pair<float, Actor*>> scoredActors;

			for(auto& actor : *world)
			{
				auto score = query(actor);

				if (score >= 0.f) 
				{
					scoredActors.emplace_back(score, actor);
				}
			}

			result.actors.reserve(scoredActors.size());

			std::sort(begin(scoredActors), end(scoredActors),
				[](const auto& lhs, const auto& rhs)
				{return lhs.first < rhs.first;});

			for(auto& score : scoredActors)
			{
				result.actors.push_back(score.second);
			}

			return result;
		}

		//tuple elements: dot(direction, right), dot(direction, forward) < 0, direction
		std::tuple<float, bool, Math::Vector3> calculateRelativeRotationTo(float x, float y) const;

	private:
		
		World* world;
		Actor* owner;
	};
}