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

/*
Tracks how long a float value was below some given threshold, up to a maximum duration
*/

#include <functional>

struct ValueOverTimeTracker
{
	ValueOverTimeTracker();

	//the threshold to compare subsequent values to
	float maxValue;

	//how long values provided in update() were below maxValue, up to maximumDurationToTrack
	float durationBelowValue;

	/*
	typically one wants to perform an action one time when durationBelowValue >= maximumDurationToTrack.a
	this keeps that bookkeeping here, and resets the flag when a new maxValue is provided via update()
	*/
	bool reacted;	

	static const float maximumDurationToTrack;

	std::function<bool(ValueOverTimeTracker&)> durationExceedsExtension;

	/*
	called once per frame with the current value of the thing a user wants to track.
	if value is >= maxValue, then the duration is reset and maxValue is set to value.
	otherwise, the duration is increased
	*/
	void update(float value, float dt);
};