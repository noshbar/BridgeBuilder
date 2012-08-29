/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __SLAB_H_
#define __SLAB_H_

#include "pin.h"
#include "positioning.h"

/* What follows are the different types of slab possible. Derived classes should set their "Purpose" to the value
   appropriate to their function. */
typedef enum Slab_Purpose
{
	Slab_Purpose_Invalid = 0,
	Slab_Purpose_Support,  /* A support structure, like those wire cables on bridges. Cars do not collide with these. */
	Slab_Purpose_Structure,    /* A solid piece of e.g., road. Cars collide with these slabs. */
}
Slab_Purpose;

class Slab
{
public:
	Slab         *Next;         /* The next slab in the linked-list. */
	Slab_Purpose  Purpose;      /* The purpose/material of this slab in the bridge, gets set by derived class constructors. */
	Pin          *Left;         /* The first pin this slab is connected to, for simplicity sake, the "left" pin. */
	Pin          *Right;        /* The second pin, referred to as the "right" pin. */
	Positioning   Transform;    /* Describes the position and rotation of this slab, both at rest, and the current state. */
	void         *PhysicBody;   /* Points to the object instance within the Physics world instance. */
	float         Length;       /* The length of the slab, we need this for rendering. */

protected:
	void initialise(Pin *Left, Pin *Right, void *Body)
	{
		this->Left  = Left;
		this->Right = Right;
		PhysicBody  = Body;
		if (Left != NULL && Right != NULL)
		{
			float differenceX = Right->Transform.X() - Left->Transform.X();
			float differenceY = Right->Transform.Y() - Left->Transform.Y();
			float angle       = atan2(differenceY, differenceX);
			Length            = sqrt( (differenceX * differenceX) + (differenceY * differenceY) );
			Transform.Initialise(Left->Transform.X() + differenceX / 2.0f, Left->Transform.Y() + differenceY / 2.0f, angle);
		}
	}

	/* Hide the default constructor as one shouldn't be able to exist without existing pins to attach to. */
	Slab()
	{
	}

public:
	Slab(Pin *Left, Pin *Right)
	{
		Next    = NULL;
		Purpose = Slab_Purpose_Invalid;
		initialise(Left, Right, NULL);
	}

	void Recalculate()
	{
		initialise(Left, Right, PhysicBody);
	}
};

#endif
