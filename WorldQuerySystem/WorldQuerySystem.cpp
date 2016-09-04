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

#include "WorldQuerySystem.h"

namespace AI
{
	void WorldQuerier::setup(World* world, Actor* owner)
	{
		this->world = world;
		this->owner = owner;
	}

	Math::Vector3 WorldQuerier::getForwardVector() const
	{
		return owner->GetActorForwardVector();
	}

	Math::Vector3 WorldQuerier::getRightVector() const
	{
		return owner->GetActorRightVector();
	}

	Math::Vector3 WorldQuerier::getUpVector() const
	{
		return owner->GetActorUpVector();
	}

	World* WorldQuerier::getWorld() const
	{
		return world;
	}

	std::string WorldQuerier::getName() const
	{
		return owner->getName();
	}
	
	//tuple elements: dot(direction, right), dot(direction, forward) < 0, direction
	std::tuple<float, bool, Math::Vector3> WorldQuerier::calculateRelativeRotationTo(float x, float y) const
	{
		const auto& actorLocation = owner->GetActorLocation();
		auto right = owner->GetActorRightVector();
		Math::Vector3 direction{x, y, actorLocation.z};
		direction -= actorLocation;
		direction.normalize();
		auto angle = dot(direction, owner->GetActorForwardVector());

		return std::make_tuple(dot(direction, right), angle < 0.f, direction);
	}
}