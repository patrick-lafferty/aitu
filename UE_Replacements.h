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

#include "Math.h"
#include <string>
#include <vector>
#include <memory>
#include "IFixedTick.h"
#include "Utility.h"
#include "SoundMap.h"

namespace AI
{
    /*
    Replaces AActor
    */
    class Actor
    {
    public:

        Actor(class World* world, int id, std::string name = "nameless");

        World* getWorld();

        Math::Vector3 GetActorLocation() const;
        Math::Vector3 GetActorForwardVector() const;
        Math::Vector3 GetActorRightVector() const;
        Math::Vector3 GetActorUpVector() const;

        std::string getName() const;
        int getId() const {return id;}

        //takes ownership of the component
        class Component* addComponent(std::unique_ptr<Component> component);

    private:

        World* world;
        Math::Transform transform;
        int id;
        std::string name;
        std::vector<std::unique_ptr<Component>> components;
    };

    class Component
    {
    public:

        Component(Actor* owner);

        class World* getWorld();

        virtual void initializeComponent() = 0;

    protected:

        Actor* owner;
    };

    /*
    Replaces UWorld
    */
    class World
    {
    public:

        typedef std::vector<std::unique_ptr<Actor>>::iterator WorldIterator; 

        World(class GameMode* authGameMode);
        GameMode* getAuthGameMode();

        WorldIterator begin() {return actors.begin();}
        WorldIterator end() {return actors.end();}

        Actor* createActor();

        Actor* getPlayer() {return player;}

    private:

        GameMode* authGameMode;
        std::vector<std::unique_ptr<Actor>> actors;
        Actor* player;
        int nextActorId {0};
    };

    /*
    Replaces AGameMode
    */
    class GameMode
    {
    public:

        GameMode();

        void registerForFixedTicks(IFixedTickable* thing);
        TaskDatabase& getAvailableTasks();
        SoundMap& getSoundMap();
        std::string getBarkString(enum Bark bark);

        void tick();

    private:

        std::vector<IFixedTickable*> tickables;
        TaskDatabase taskDatabase;
        SoundMap soundMap;
    };

    /*
    Replaces UGameplayStatics
    */
    class GameplayStatics
    {
    public:
        static Actor* GetPlayerCharacter(World* world, int);
    };
}