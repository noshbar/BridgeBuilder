/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __SLAB_STRUCTURE_H_
#define __SLAB_STRUCTURE_H_

#include "slab.h"

/* This slab type is meant for solid items that the objects in the world can collide with, e.g., a road segment. */
class SlabStructure : public Slab
{
protected:
	/* Hide the default constructor as one shouldn't be able to exist without existing pins to attach to. */
	SlabStructure()
	{
	}

public:
	SlabStructure(Pin *Left, Pin *Right) : Slab(Left, Right)
	{
		Purpose = Slab_Purpose_Structure;
	}
};

#endif
