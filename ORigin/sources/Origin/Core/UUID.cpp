// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "UUID.h"
#include <random>

namespace origin {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	UUID::UUID()
		: m_UUID(s_UniformDistribution(s_Engine))
	{
		PROFILER_FUNCTION();
	}

	UUID::UUID(uint64_t uuid)
		: m_UUID(uuid)
	{
	}
}