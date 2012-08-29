/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __PIN_H_
#define __PIN_H_

#include "positioning.h"

/* A "Pin" is a join between two bridge segments, usually drawn as a dot in bridge building games.
   It uses joints to connect multiple segments together, and can be "Fixed" aka static. */
class Pin
{
public:
	Pin         *Next;         /* The next Pin instance in the linked-list. */
	Positioning  Transform;    /* The position of this pin while at rest, and the current position. */
	bool         Fixed;        /* Set this to true if the Pin must not move, e.g., attached to ground. */
	void        *PhysicBody;   /* Points to the object instance within the Physics world instance. */

protected:
	void initialise(float X, float Y, bool Fixed)
	{
		this->Fixed = Fixed;
		Next        = NULL;
		PhysicBody  = NULL;
		Transform.Initialise(X, Y, 0);
	}

	/* Hide the default constructor as a Pin should always be created with a position. */
	Pin()
	{
	}

public:
	Pin(float X, float Y, bool Fixed)
	{
		initialise(X,Y, Fixed);
	}
};

#endif
