//
//  AudioLibSwitcher_libsoundio.h
//  AudioLibSwitcher_libsoundio
//
//  Created by Rasmus Anthin on 2024-03-28.
//

#pragma once
#include "AudioLibSwitcher/IAudioLibSwitcher.h"

#include <iostream>
#include <vector>
#include <soundio/soundio.h>


namespace audio
{
  
  class AudioLibSwitcher_libsoundio final : IAudioLibSwitcher
  {
    SoundIo* m_soundio = nullptr;
    SoundIoOutStream* m_outstream = nullptr;
        
    // Source_libsoundio class
    struct Source_libsoundio
    {
      float volume = 1.f;
      float* data = nullptr;
      int sample_count = 0;
      int position = 0;
      bool is_playing = false;
      bool looping = false;
      float pitch = 0.f;
      
      Source_libsoundio(float* audio_data, int sample_count)
      : data(audio_data)
      , sample_count(sample_count)
      {}
    };
    
    // Buffer_libsoundio class
    struct Buffer_libsoundio
    {
      // Define your buffer structure here
      // For example:
      std::vector<float> data;
      // Other necessary members
    };
    
    class SourceManager_libsoundio
    {
    private:
      std::vector<Source_libsoundio> m_sources;
      
    public:
      size_t add_source(const Source_libsoundio& source)
      {
        auto src_id = m_sources.size();
        m_sources.emplace_back(source);
        return src_id;
      }
      
      bool remove_source(size_t source_id)
      {
        return stlutils::erase_at(m_sources, source_id);
      }
      
      void mix_audio(float* buffer, int frameCount)
      {
        // Clear the buffer
        std::fill(buffer, buffer + frameCount, 0.0f);
        
        // Mix audio data from each active source
        for (auto& source : m_sources)
          if (source.is_playing)
            for (int i = 0; i < frameCount && source.position < source.sample_count; ++i)
            {
              buffer[i] += source.data[source.position] * source.volume;
              ++source.position;
            }
      }
      
      bool is_playing(size_t source_id) const {
        return source_id < m_sources.size() ? m_sources[source_id].is_playing : false;
      }
      
      void play(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id].is_playing = true;
          // Code to start playing the source using libsoundio
        }
      }
      
      void pause(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id].is_playing = false;
          // Code to pause the source using libsoundio
        }
      }
      
      void stop(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id].is_playing = false;
          // Code to stop the source using libsoundio
        }
      }
      
      void set_volume(size_t source_id, float volume)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id].volume = volume;
          // Code to set the volume of the source using libsoundio
        }
      }
      
      void set_pitch(size_t source_id, float pitch)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id].pitch = pitch;
          // Code to set the pitch of the source using libsoundio
        }
      }
      
      void set_looping(size_t source_id, bool looping)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id].looping = looping;
          // Code to set the looping behavior of the source using libsoundio
        }
      }
    };
    
    class BufferManager_libsoundio
    {
      std::vector<Buffer_libsoundio> m_buffers;
      
    public:
      size_t add_buffer(const Buffer_libsoundio& buffer)
      {
        auto buf_id = m_buffers.size();
        m_buffers.push_back(buffer);
        return buf_id;
      }
      
      bool remove_buffer(size_t buffer_id)
      {
        return stlutils::erase_at(m_buffers, buffer_id);
      }
      
      // Additional functions for buffer management can be added here
    };
    
    SourceManager_libsoundio m_source_manager;
    BufferManager_libsoundio m_buffer_manager;
    
  public:
    virtual void init() override
    {
      // Initialize libsoundio
      m_soundio = soundio_create();
      if (m_soundio == nullptr)
        throw std::runtime_error("Failed to initialize libsoundio");
      
      soundio_connect(m_soundio);
      soundio_flush_events(m_soundio);
    }
    
    virtual void finish() override
    {
      // Clean up libsoundio resources
      if (m_outstream != nullptr)
        soundio_outstream_destroy(m_outstream);
      if (m_soundio != nullptr)
        soundio_destroy(m_soundio);
    }
    
    unsigned int create_source() override
    {
      // Create a new source and return its ID
      Source_libsoundio source(nullptr, 0);
      auto src_id = m_source_manager.add_source(source);
      return static_cast<unsigned int>(src_id);
    }
    
    void destroy_source(unsigned int src_id) override
    {
      m_source_manager.remove_source(static_cast<size_t>(src_id));
    }
    
    unsigned int create_buffer() override
    {
      // Create a new buffer and return its ID
      Buffer_libsoundio buffer;
      auto buf_id = m_buffer_manager.add_buffer(buffer);
      return static_cast<unsigned int>(buf_id);
    }
    
    void destroy_buffer(unsigned int buf_id) override
    {
      m_buffer_manager.remove_buffer(static_cast<size_t>(buf_id));
    }
    
    virtual void play_source(unsigned int src_id) override
    {
      m_source_manager.play(src_id);
    }
    
    virtual bool is_source_playing(unsigned int src_id) override
    {
      return m_source_manager.is_playing(src_id);
    }
    
    virtual void pause_source(unsigned int src_id) override
    {
      m_source_manager.pause(src_id);
    }
    
    virtual void stop_source(unsigned int src_id) override
    {
      m_source_manager.stop(src_id);
    }
    
    virtual void set_source_volume(unsigned int src_id, float vol) override
    {
      m_source_manager.set_volume(src_id, vol);
    }
    
    virtual void set_source_pitch(unsigned int src_id, float pitch) override
    {
      m_source_manager.set_pitch(src_id, pitch);
    }
    
    virtual void set_source_looping(unsigned int src_id, bool loop) override
    {
      m_source_manager.set_looping(src_id, loop);
    }
    
    virtual void detach_source(unsigned int src_id) override
    {
    
    }
    
    virtual void set_source_standard_params(unsigned int src_id) override
    {
    
    }
    
    virtual void set_buffer_data_mono_16(unsigned int buf_id, const std::vector<short>& buffer, int sample_rate) override
    {
    
    }
    
    virtual void attach_buffer_to_source(unsigned int src_id, unsigned int buf_id) override
    {
    
    }
    
    virtual std::string check_error() override
    {
      return "";
    }
    
  };
  
}
