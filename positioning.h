/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __POSITIONING_H_
#define __POSITIONING_H_

/* The main purpose of this class is to store the position and angle of a piece of bridge (pin/slab).
   Other objects simply obtain the position for e.g., drawing by calling X(), Y() and Angle().

   The idea is to initialise an instance with a position and angle, this being regarded as the edit time position,
   then when the physics engine does its stuff, it can set the position to its new temporary position.

   This means that the X(), Y() and Angle() calls always return the CURRENT values, regardless of whether it
   is during editing or simulation.

   A call to Reset() sets the values back to their edit-time/original values.

   Usage:
   10 call Initialise() with the edit-time information (drawing calls X(), Y() and Angle())
   20 change to simulation mode
   30 the physics engine calls Set() with the new temporary values (drawing still calls X(), Y() and Angle())
   40 changing to edit mode
   50 call Reset()
   60 goto 10
   */
class Positioning
{
protected:
	enum
	{
		Original = 0,
		Current,
	};

protected:
	float x[2];
	float y[2];
	float angle[2];

public:
	Positioning()
	{
		x[0] = x[1] = y[0] = y[1] = angle[0] = angle[1] = 0;
	}

	void Initialise(float X, float Y, float Angle)
	{
		x[Original]     = X;
		x[Current]      = x[Original];
		y[Original]     = Y;
		y[Current]      = y[Original];
		angle[Original] = Angle;
		angle[Current]  = angle[Original];
	}

	void Set(float X, float Y, float Angle)
	{
		x[Current]     = X;
		y[Current]     = Y;
		angle[Current] = Angle;
	}

	void Reset()
	{
		x[Current]     = x[Original];
		y[Current]     = y[Original];
		angle[Current] = angle[Original];
	}

	float X()
	{
		return x[Current];
	}

	float Y()
	{
		return y[Current];
	}

	float Angle()
	{
		return angle[Current];
	}
};

#endif
