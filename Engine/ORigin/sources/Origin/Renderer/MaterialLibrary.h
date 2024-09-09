// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef MATERIAL_LIBRARY_H
#define MATERIAL_LIBRARY_H

#include "Material.h"

namespace origin
{
	class MaterialLibrary
	{
	public:
		MaterialLibrary() = default;

		void Add(const std::string &name, const std::shared_ptr<Material> &material);
		bool Exist(const std::string &name);

		std::shared_ptr<Material> Get(const std::string &name);
		std::unordered_map<std::string, std::shared_ptr<Material>> GetMap() const { return m_MaterialMap; }
		inline size_t GetSize() const { return m_MaterialMap.size(); }

	private:
		std::unordered_map<std::string, std::shared_ptr<Material>> m_MaterialMap;
	};
}

#endif