# aitu
aitu is the AI library from my game Wooden Sphere that uses hierarchical task networks and utility planners. By defining Tasks, giving tasks Considerations, and defining world state for an AI character, that character can then decide on what it wants to do and forms a plan on how to achieve that.

#Building

CMake 3.0 or higher is required to build. aitu uses [Catch](https://github.com/philsquared/Catch) for testing, so if you want to build the tests then get a copy.

Download aitu and run cmake. 

If you want to build the tests:
 - if using cmake-gui, fill out the text box for CATCH_INCLUDE_DIR to point to catch/single_include
 - if using the console version, pass CATCH_INCLUDE_DIR as a parameter:

    ```cmake -G "MSYS Makefiles" -DCATCH_INCLUDE_DIR:PATH=/path/to/catch/single_include```
    
    while replacing "MSYS Makefiles" with your generator of choice

#Usage

###Defining your own tasks

The short version:
 - Edit WorldState.h:45 and add new entries to WorldStateIdentifier
 - Edit Tasks.h:36 and add your own TaskIdentifier
 - Edit Tasks.cpp and 
    - write a function defining your task
    - add your Task to the TaskDatabase in setupTasks()
    - fill out a Consideration object (see Consideration.h) and add that to the TaskDatabase
  

