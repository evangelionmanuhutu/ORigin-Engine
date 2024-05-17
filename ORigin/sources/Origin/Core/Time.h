// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#pragma once
#include <chrono>

namespace origin
{
	class Timestep
	{
	public:
		Timestep(float time = 0.0)
			: m_Time(time)
		{
			s_Instance = this;
		}

		operator float() const { return m_Time; }
		static float Seconds() { return s_Instance->m_Time; }
		static float MilliSeconds() { return s_Instance->m_Time * 1000.0f; }
		static Timestep &Get() { return *s_Instance; }

	private:
		static Timestep *s_Instance;
		float m_Time;
	};

	class Timer
	{
	public:
		Timer() {
			Reset();
		}

		void Timer::Reset()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		float Timer::Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
		}

		float Timer::ElapsedMillis()
		{
			return Elapsed() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

}