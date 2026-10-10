#include "Vector.h"

FLinearColor FVector4::ToLinearColor() const
{
	return FLinearColor(this->x, this->y, this->z, this->w);
}
