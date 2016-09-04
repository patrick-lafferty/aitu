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

#include "UE_Replacements.h"
#include "log.h"

using namespace Math;

namespace AI
{
    Actor::Actor(World* world, int id, std::string name)
        : world{world}, id{id}, name{name}
    {
        log("Created Actor: ", name);
    }

    World* Actor::getWorld()
    {
        return world;
    }

    Vector3 Actor::GetActorLocation() const
    {
        return {transform.m[12], transform.m[13], transform.m[14]};
    }

    Vector3 Actor::GetActorForwardVector() const
    {
        return {transform.m[7], transform.m[8], transform.m[9]};
    }

    Vector3 Actor::GetActorRightVector() const
    {
        return {transform.m[0], transform.m[1], transform.m[2]};
    }

    Vector3 Actor::GetActorUpVector() const
    {
        return {transform.m[4], transform.m[5], transform.m[6]};
    }

    std::string Actor::getName() const
    {
        return name;
    }

    class Component* Actor::addComponent(std::unique_ptr<Component> component)
    {
        auto raw = component.get();
        components.push_back(std::move(component));
        return raw;
    }

    Component::Component(Actor* owner)
        : owner{owner}
        {}

    World* Component::getWorld()
    {
        return owner->getWorld();
    }

    World::World(GameMode* mode)
        : authGameMode{mode}
        {}
    
    GameMode* World::getAuthGameMode()
    {
        return authGameMode;
    }

    Actor* World::createActor()
    {
        auto actor = std::make_unique<Actor>(this, nextActorId++);
        auto raw = actor.get();

        actors.push_back(std::move(actor));

        return raw;
    }

    GameMode::GameMode()
    {
        taskDatabase = setupTasks();
    }

    void GameMode::registerForFixedTicks(IFixedTickable* tickable)
    {
        tickables.push_back(tickable);
    }

    TaskDatabase& GameMode::getAvailableTasks()
    {
        return taskDatabase;
    }
    
    SoundMap& GameMode::getSoundMap()
    {
        return soundMap;
    }
        
    std::string GameMode::getBarkString(enum Bark bark)
    {
        return "";
    }

    void GameMode::tick()
    {
        //TODO: fixed ticking

        for(auto& tickable : tickables)
        {
            tickable->fixedTick(1.f/60.f);
        }
    }

    Actor* GameplayStatics::GetPlayerCharacter(World* world, int)
    {
        return world->getPlayer();
    }
}