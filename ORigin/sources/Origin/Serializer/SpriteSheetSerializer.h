// Copyright (c) Evangelion Manuhutu | ORigin Engine
#pragma once
#include "Origin\Scene\SpriteSheet.h"
#include <filesystem>

namespace origin
{
	class SpriteSheetSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path &filepath, const std::shared_ptr<SpriteSheet> &spriteSheet);
		static bool Deserialize(const std::filesystem::path &filepath, std::shared_ptr<SpriteSheet> &spriteSheet);
	};
}

