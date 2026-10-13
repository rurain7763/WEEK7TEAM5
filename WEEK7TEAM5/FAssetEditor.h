#pragma once

#include "Core.h"
#include "FGuid.h"

class FAssetEditor
{
public:
	virtual ~FAssetEditor() = default;

	virtual void Render() = 0;
	virtual void Save() = 0;

	inline bool IsOpen() const { return bIsOpen; }

protected:
	bool bIsOpen = false;
};
