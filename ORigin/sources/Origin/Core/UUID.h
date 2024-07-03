// Copyright (c) Evangelion Manuhutu | ORigin Engine

#pragma once

#ifdef OGN_PLATFORM_WINDOWS
	#include <xhash>
#else
	#include <functional>
#endif

#include <cstdint>

namespace origin
{
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID &uuid) = default;

		operator uint64_t() const { return m_UUID; }

		static const UUID NullID;

	private:
		uint64_t m_UUID;
	};

	const UUID UUID::NullID = UUID(0);
}

namespace std
{
	template<>
	struct hash<origin::UUID>
	{
		std::size_t operator() (const origin::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
}