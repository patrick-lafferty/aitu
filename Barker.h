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

#include <string>

namespace AI
{
	enum class Bark
	{
		LostSightOfPlayer,
		StartPlayerSearch,
		StaringAtPlayer,
		RegainedSightOfPlayer,
		Bother,

		//reactions
		Dismiss,
		FoundPlayer,
		LostPlayer,
		HeardSomething,
		SawSomething,

		BeginInvestigation,
		EndInvestigation,
		
		GreetFriend,
		ChitChat,

		GoingToSit
	};

	/*
	Barker displays a message onscreen for a certain duration
	*/
	class Barker
	{
	public:

		//sets the bark string to be shown along with how long to show it
		void bark(std::string s, float duration = 5.f);
		
		//called once per frame to determine whether to continue displaying the bark or not
		void update(float dt);

		bool isBarking();		
		
	private:

		//how much longer to display the bark
		float timeRemaining {0.f};
		//are we barking?
		bool barking {false};
		//the actual string to show
		std::string speech;
	};
}