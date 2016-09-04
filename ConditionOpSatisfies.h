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

#include "Tasks.h"

namespace AI
{
	/*
	canXSatisfy tests whether the given condition op and values can satisfy
	a different condition given the same values. Used to check if two different
	conditions are compatible.

	Given tasks A and B, we want to check if one of B's postconditions can
	satisfy one of A's preconditions. Say B has a postcondition NotEqual(x, y).
	If A has a precondition Equal(x, z), then if y != z its possible that NotEqual(x, y)
	could satisfy Equal(x, z).

	eg if NotEqual(x, 5) is true, Equal(x, any value other than 5) could be true,
	but Equal(x, 5) would be false.

	*/
	bool canLessThanSatisfy(float a, ConditionOp op, float value);
	bool canGreaterThanSatisfy(float a, ConditionOp op, float value);
	bool canEqualToSatisfy(float a, ConditionOp op, float value);
	bool canNotEqualToSatisfy(float a, ConditionOp op, float value);
	bool canLessEqualSatisfy(float a, ConditionOp op, float value);
	bool canGreaterEqualSatisfy(float a, ConditionOp op, float value);
}