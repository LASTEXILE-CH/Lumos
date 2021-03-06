#pragma once
#include "lmpch.h"

namespace Lumos
{
	LUMOS_EXPORT u8* LoadImageFromFile(const char* filename, u32* width = nullptr, u32* height = nullptr, u32* bits = nullptr, bool* isHDR = nullptr, bool flipY = false);
	LUMOS_EXPORT u8* LoadImageFromFile(const String& filename, u32* width = nullptr, u32* height = nullptr, u32* bits = nullptr, bool* isHDR = nullptr, bool flipY = false);
}
