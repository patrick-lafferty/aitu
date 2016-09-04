cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_EXE_LINKER_FLAGS "ld")

project(aitu)

set(CATCH_INCLUDE_DIR "catch" CACHE PATH "path to include folder for Catch (github.com/philsquared/Catch)")

include_directories(${CATCH_INCLUDE_DIR})

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

add_library(aitu_objs OBJECT
	Tasks.cpp
	Condition.cpp	
	Planner.cpp
	ConditionOpSatisfies.cpp
	Math.cpp
	WorldState.cpp
	Barker.cpp
	Memory.cpp
	SoundMap.cpp
	ValueOverTimeTracker.cpp
	WorldQuerySystem/WorldQuerySystem.cpp
	UE_Replacements.cpp
	HierarchicalTaskNetworkComponent.cpp
)

target_compile_options(aitu_objs PUBLIC -std=c++1z -stdlib=libstdc++ -Wall)

add_executable(aitu
	main.cpp
	$<TARGET_OBJECTS:aitu_objs>
)

target_compile_options(aitu PUBLIC -std=c++1z -stdlib=libstdc++ -Wall)

add_executable(aitu_test 	
	tests/catch_main.cpp
	tests/math.cpp
	$<TARGET_OBJECTS:aitu_objs>
)

target_compile_options(aitu_test PUBLIC -std=c++1z -stdlib=libstdc++ -Wall)

target_link_libraries(aitu_test Catch)

enable_testing()
add_test(NAME AituTest COMMAND aitu_test)
