# aitu [![Build Status](https://travis-ci.org/patrick-lafferty/aitu.svg?branch=master)](https://travis-ci.org/patrick-lafferty/aitu) [![Packagist](https://img.shields.io/packagist/l/doctrine/orm.svg?maxAge=2592000)]()
aitu is the AI library from my game Wooden Sphere that uses hierarchical task networks and utility planners. By defining Tasks, giving tasks Considerations, and defining world state for an AI character, that character can then decide on what it wants to do and forms a plan on how to achieve that.

# Tasks and Plans

Tasks are the building blocks of this system. A Task is something that an AI character can do. They can have an Action enum which your code can switch on and perform some action. They can also have four std::function objects (setup, finish, finally, loop - defined in Tasks.h line 155) which get run at different times and can manipulate a WorldState object belonging to some character. Tasks have pre and post conditions which look at some WorldState property to determine their truthiness.

Plans are a list of tasks to be completed in sequential order. When creating a plan for some goal task, the planner looks at the preconditions for that task and see if any other tasks can satisfy those conditions. That is, for a goal task G it looks for any task T where T’s postconditions match G’s preconditions. Then if T has preconditions it looks for tasks that can satisfy those, etc. A plan then is a chain of tasks starting with something that has no other preconditions/can be started right away, and each successive task does something that allows the next task to be run, until finally reaching the goal task.

I chose this design because I wanted a flexible AI with the possibility of emergent gameplay. Since I never explicitly define dependencies between tasks but instead just expose their requirements, its easy to add in a new independent task which the planner can then pick up and use so long as it meets their requirements. No need to modify other tasks to make use of the new one, just define the task and the planner figures it out. And because of that you can get scenarios where the planner chose something unexpectely, since it just looks for anything that satisfies some condition. So some task that you added for one reason might find itself being used in a way you (or the player) didn’t predict.

# Utility and considerations

So that covers what plans are and how they get created, but how does the AI choose a goal in the first place? Enter considerations. A Consideration is something that looks at some aspect of the character’s WorldState and gives it a score. Tasks can have multiple considerations associated with them, each one looking at one particular aspect of the world state. The task with the highest score becomes the goal. To calculate a consideration’s score you pick some function (linear, exponential, log etc), define the constants for that function (the ABC’s in ax^2 + bx + c) and select the input source that gets evaluated by that function. Currently you can pick any value in WorldState, as well as other sources such as the number of tasks run since some target task (to limit repeats). To make it simple to edit considerations I made the [ConsiderationEditor](https://patrick-lafferty.github.io/projects/woodensphere/#considerationEditor) for Unreal Editor.

# Building

CMake 3.0 or higher is required to build. aitu uses [Catch](https://github.com/philsquared/Catch) for testing, so if you want to build the tests then get a copy.

Download aitu and run cmake. 

If you want to build the tests:
 - if using cmake-gui, fill out the text box for CATCH_INCLUDE_DIR to point to catch/single_include
 - if using the console version, pass CATCH_INCLUDE_DIR as a parameter:

    ```cmake -G "MSYS Makefiles" -DCATCH_INCLUDE_DIR:PATH=/path/to/catch/single_include```
    
    while replacing "MSYS Makefiles" with your generator of choice

# Usage

### Defining your own tasks

The short version:
 - Edit WorldState.h:45 and add new entries to WorldStateIdentifier
 - Edit Tasks.h:36 and add your own TaskIdentifier
 - Edit Tasks.cpp and 
    - write a function defining your task
    - add your Task to the TaskDatabase in setupTasks()
    - fill out a Consideration object (see Consideration.h) and add that to the TaskDatabase
  

