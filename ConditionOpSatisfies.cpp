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

#include "ConditionOpSatisfies.h"

namespace AI
{
	bool canLessThanSatisfy(float a, ConditionOp op, float value)
	{
		switch(op)
		{
			case ConditionOp::LessThan
				:
			{	
				return true;
			}
			case ConditionOp::GreaterThan
				:
			{
				return value < a;
			}
			case ConditionOp::EqualTo
				:
			{
				return value < a;
			}
			case ConditionOp::NotEqualTo
				:
			{
				return true;
			}
			case ConditionOp::LessEqual
				:
			{
				return true;
			}
			case ConditionOp::GreaterEqual
				:
			{
				return value < a;
			}
		}

		return false;
	}

	bool canGreaterThanSatisfy(float a, ConditionOp op, float value)	
	{
		switch(op)
		{
			case ConditionOp::LessThan
				:
			{				
				return value < a;
			}
			case ConditionOp::GreaterThan
				:
			{
				return true;
			}
			case ConditionOp::EqualTo
				:
			{
				return value > a;
			}
			case ConditionOp::NotEqualTo
				:
			{
				return true;
			}
			case ConditionOp::LessEqual
				:
			{
				return value > a;
			}
			case ConditionOp::GreaterEqual
				:
			{
				return true;
			}
		}

		return false;
	}

	bool canEqualToSatisfy(float a, ConditionOp op, float value)	
	{
		switch(op)
		{
			case ConditionOp::LessThan
				:
			{				
				return value > a;
			}
			case ConditionOp::GreaterThan
				:
			{
				return value < a;
			}
			case ConditionOp::EqualTo
				:
			{
				return value == a;
			}
			case ConditionOp::NotEqualTo
				:
			{
				return value != a;
			}
			case ConditionOp::LessEqual
				:
			{
				return value >= a;
			}
			case ConditionOp::GreaterEqual
				:
			{
				return value <= a;
			}
		}

		return false;
	}

	bool canNotEqualToSatisfy(float a, ConditionOp op, float value)
	{
		switch(op)
		{
			case ConditionOp::LessThan
				:
			{				
				return true;
			}
			case ConditionOp::GreaterThan
				:
			{
				return true;
			}
			case ConditionOp::EqualTo
				:
			{
				return value != a;
			}
			case ConditionOp::NotEqualTo
				:
			{
				return true;
			}
			case ConditionOp::LessEqual
				:
			{
				return true;
			}
			case ConditionOp::GreaterEqual
				:
			{
				return true;
			}
		}

		return false;
	}

	bool canLessEqualSatisfy(float a, ConditionOp op, float value)	
	{
		switch(op)
		{
			case ConditionOp::LessThan
				:
			{				
				return true;
			}
			case ConditionOp::GreaterThan
				:
			{
				return value < a;
			}
			case ConditionOp::EqualTo
				:
			{
				return value <= a;
			}
			case ConditionOp::NotEqualTo
				:
			{
				return true;
			}
			case ConditionOp::LessEqual
				:
			{
				return true;
			}
			case ConditionOp::GreaterEqual
				:
			{
				return value <= a;
			}
		}

		return false;
	}

	bool canGreaterEqualSatisfy(float a, ConditionOp op, float value)
	{
		switch(op)
		{
			case ConditionOp::LessThan
				:
			{				
				return value > a;
			}
			case ConditionOp::GreaterThan
				:
			{
				return true;
			}
			case ConditionOp::EqualTo
				:
			{
				return value >= a;
			}
			case ConditionOp::NotEqualTo
				:
			{
				return true;
			}
			case ConditionOp::LessEqual
				:
			{
				return value >= a;
			}
			case ConditionOp::GreaterEqual
				:
			{
				return true;	
			}
		}

		return false;
	}
}