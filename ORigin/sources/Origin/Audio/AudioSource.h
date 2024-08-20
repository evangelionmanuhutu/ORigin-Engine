// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include "AudioEngine.h"

namespace origin
{
	class AudioSource : public Asset
	{
	public:
		AudioSource() = default;
		~AudioSource();

		void LoadSource(const std::string &name, const std::filesystem::path &filepath, bool looping = false, bool spatializing = false);
		void LoadStreamingSource(const std::string &name, const std::filesystem::path &filepath, bool looping = false, bool spatializing = false);
		void ActivateOverlapping();
		void DeactivateOverlapping();

		void SetLoop(bool enable);
		void SetVolume(float value);
		void SetPitch(float value);
		void SetPaning(float pan);
		void SetPosition(const glm::vec3 &position,int index = -1, ma_positioning mode = ma_positioning_absolute);
		void SetName(const char* name);
		void SetSpatial(bool enable);
		void SetMinMaxDistance(float minVal, float maxVal);

		void Play();
		void Pause();
		void Stop();
		void PlayLooped();
		void PlayOverlapping();
		bool IsPlaying();
		bool IsPaused();
		bool IsLooping();
		bool IsSpatial();
		float GetVolume();
		float GetPitch();
		float GetMinDistance();
		float GetMaxDistance();
		bool IsLoaded = false;

		const glm::vec3 GetPosition(int index = 0) const;

		const std::string &GetName() { return m_Name; }
		const std::filesystem::path GetFilepath() { return m_Filepath; }

		static AssetType GetStaticType() { return AssetType::Audio; }
		virtual AssetType GetType() const { return GetStaticType(); }

		static std::shared_ptr<AudioSource> Create();
	private:
		std::filesystem::path m_Filepath;
		std::string m_Name;
		std::vector<ma_sound*> m_Sounds;
		int m_OverlapIndex = 0;
		int m_MaxOverlap = 50;
		bool m_IsSpatializing = false;
		bool m_OverlappingAllowed = false;
	};

}

#endif