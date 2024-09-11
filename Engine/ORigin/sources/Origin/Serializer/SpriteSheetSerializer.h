// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef SPRITE_SHEET_SERIALIZER_H
#define SPRITE_SHEET_SERIALIZER_H

#include "Origin/Scene/SpriteSheet.h"
#include <filesystem>

namespace origin
{
	class OGN_API SpriteSheetSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path &filepath, const std::shared_ptr<SpriteSheet> &spriteSheet);
		static bool Deserialize(const std::filesystem::path &filepath, std::shared_ptr<SpriteSheet> &spriteSheet);
	};
}

#endif