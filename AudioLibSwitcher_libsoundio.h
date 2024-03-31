//
//  AudioLibSwitcher_libsoundio.h
//  AudioLibSwitcher_libsoundio
//
//  Created by Rasmus Anthin on 2024-03-28.
//

#pragma once
#include "../Core/Math.h"
#include "../Core/StlUtils.h"
#include "AudioLibSwitcher/IAudioLibSwitcher.h"
#include <iostream>
#include <vector>
#include <soundio/soundio.h>
#include <future>
#include <memory>


namespace audio
{
  
  class AudioLibSwitcher_libsoundio final : IAudioLibSwitcher
  {
    SoundIo* m_soundio = nullptr;
    SoundIoOutStream* m_outstream = nullptr;
    SoundIoDevice* m_device = nullptr;
        
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
      int sample_rate = 44100;
      std::future<void> playback_handle;
      
      Source_libsoundio(float* audio_data, int sample_count)
      : data(audio_data)
      , sample_count(sample_count)
      {}
      
      void copy_from(const Source_libsoundio& other)
      {
        this->volume = other.volume;
        std::memcpy(this->data, other.data, other.sample_count*sizeof(float));
        this->sample_count = other.sample_count;
        this->position = other.position;
        this->is_playing = other.is_playing;
        this->looping = other.looping;
        this->pitch = other.pitch;
        this->sample_rate = other.sample_rate;
        //this->playback_handle = other.playback_handle;
      }
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
      std::vector<std::unique_ptr<Source_libsoundio>> m_sources;
      SoundIo* m_soundio = nullptr;
      SoundIoOutStream* m_outstream = nullptr;
      
    public:
      SourceManager_libsoundio(SoundIo* soundio, SoundIoOutStream* outstream)
        : m_soundio(soundio)
        , m_outstream(outstream)
      {}
      
      size_t add_source(const Source_libsoundio& source)
      {
        auto src_id = m_sources.size();
        auto& src_ptr = m_sources.emplace_back(std::make_unique<Source_libsoundio>(nullptr, 0));
        src_ptr->copy_from(source);
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
          if (source->is_playing)
            for (int i = 0; i < frameCount && source->position < source->sample_count; ++i)
            {
              buffer[i] += source->data[source->position] * source->volume;
              ++source->position;
            }
      }
      
      bool is_playing(size_t source_id) const
      {
        if (source_id < m_sources.size())
        {
          const auto& source = m_sources[source_id];
          // Check if the playback handle is valid and if the associated task is still running
          return source->is_playing && source->playback_handle.valid() && source->playback_handle.wait_for(std::chrono::seconds(0)) == std::future_status::timeout;
        }
        else
        {
          // Return false if source_id is out of range
          return false;
        }
      }
      
      void play(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          auto& source = m_sources[source_id];
          source->is_playing = true;
          
          // Assuming source data and sample count are properly initialized
          
          // Configure libsoundio output stream
          //m_outstream = soundio_outstream_create(m_soundio);
          soundio_outstream_open(m_outstream); //, nullptr, nullptr, SoundIoFormatFloat32NE, source.sample_rate, nullptr, nullptr);
          soundio_outstream_start(m_outstream);
          
          // Write audio data to output stream (simplified example)
          //soundio_outstream_write(m_outstream, source.data, source.sample_count);
          
          // Store the playback handle for later use
          source->playback_handle = std::async(std::launch::async, [&]()
          {
            // Wait for playback to complete (simplified example)
            soundio_flush_events(m_soundio); // This is a blocking call, replace it with proper event handling if needed
            
            // Once playback is finished, set is_playing to false
            source->is_playing = false;
            
            // Cleanup libsoundio resources
            soundio_outstream_destroy(m_outstream);
          });
        }
      }
      
      void pause(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id]->is_playing = false;
          // Code to pause the source using libsoundio
        }
      }
      
      void stop(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id]->is_playing = false;
          // Code to stop the source using libsoundio
        }
      }
      
      void set_volume(size_t source_id, float volume)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id]->volume = volume;
          // Code to set the volume of the source using libsoundio
        }
      }
      
      void set_pitch(size_t source_id, float pitch)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id]->pitch = pitch;
          // Code to set the pitch of the source using libsoundio
        }
      }
      
      void set_looping(size_t source_id, bool looping)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id]->looping = looping;
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
    
    std::unique_ptr<SourceManager_libsoundio> m_source_manager;
    std::unique_ptr<BufferManager_libsoundio> m_buffer_manager;
    
  public:
    virtual void init() override
    {
      m_source_manager = std::make_unique<SourceManager_libsoundio>(m_soundio, m_outstream);
      m_buffer_manager = std::make_unique<BufferManager_libsoundio>();
    
      // Initialize libsoundio
      m_soundio = soundio_create();
      if (m_soundio == nullptr)
        throw std::runtime_error("Failed to initialize libsoundio: Out of memory.");
      
      int err = soundio_connect(m_soundio);
      if (err)
        throw std::runtime_error("Unable to connect to backend: " + std::string(soundio_strerror(err)));
      
      //fprintf(stderr, "Backend: %s\n", soundio_backend_name(soundio->current_backend));
      soundio_flush_events(m_soundio);
      
      // Find and init device
      
      char *device_id = NULL;
      int selected_device_index = -1;
      if (device_id)
      {
        int device_count = soundio_output_device_count(m_soundio);
        for (int i = 0; i < device_count; i += 1)
        {
          auto* device = soundio_get_output_device(m_soundio, i);
          if (strcmp(device->id, device_id) == 0 && device->is_raw == false)
          {
            selected_device_index = i;
            break;
          }
        }
      }
      else
        selected_device_index = soundio_default_output_device_index(m_soundio);
      if (selected_device_index < 0)
        throw std::runtime_error("Output device not found.");
      m_device = soundio_get_output_device(m_soundio, selected_device_index);
      if (m_device == nullptr)
        throw std::runtime_error("Out of memory.");
      //fprintf(stderr, "Output device: %s\n", device->name);
      if (m_device->probe_error)
        throw std::runtime_error("Cannot probe device: " + std::string(soundio_strerror(m_device->probe_error)));
    }
    
    virtual void finish() override
    {
      // Clean up libsoundio resources
      if (m_outstream != nullptr)
        soundio_outstream_destroy(m_outstream);
      if (m_device != nullptr)
        soundio_device_unref(m_device);
      if (m_soundio != nullptr)
        soundio_destroy(m_soundio);
    }
    
    unsigned int create_source() override
    {
      // Create a new source and return its ID
      Source_libsoundio source(nullptr, 0);
      auto src_id = m_source_manager->add_source(source);
      return static_cast<unsigned int>(src_id);
    }
    
    void destroy_source(unsigned int src_id) override
    {
      m_source_manager->remove_source(static_cast<size_t>(src_id));
    }
    
    unsigned int create_buffer() override
    {
      // Create a new buffer and return its ID
      Buffer_libsoundio buffer;
      auto buf_id = m_buffer_manager->add_buffer(buffer);
      return static_cast<unsigned int>(buf_id);
    }
    
    void destroy_buffer(unsigned int buf_id) override
    {
      m_buffer_manager->remove_buffer(static_cast<size_t>(buf_id));
    }
    
    virtual void play_source(unsigned int src_id) override
    {
      m_source_manager->play(src_id);
    }
    
    virtual bool is_source_playing(unsigned int src_id) override
    {
      return m_source_manager->is_playing(src_id);
    }
    
    virtual void pause_source(unsigned int src_id) override
    {
    #if 0
      m_source_manager->pause(src_id);
    #endif
    }
    
    virtual void stop_source(unsigned int src_id) override
    {
    #if 0
      m_source_manager->stop(src_id);
    #endif
    }
    
    virtual void set_source_volume(unsigned int src_id, float vol) override
    {
    #if 0
      m_source_manager->set_volume(src_id, vol);
    #endif
    }
    
    virtual void set_source_pitch(unsigned int src_id, float pitch) override
    {
    #if 0
      m_source_manager->set_pitch(src_id, pitch);
    #endif
    }
    
    virtual void set_source_looping(unsigned int src_id, bool loop) override
    {
    #if 0
      m_source_manager->set_looping(src_id, loop);
    #endif
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
    
    SoundIo* get_soundio() const { return m_soundio; }
    
    SoundIoDevice* get_device() const { return m_device; }
  };
  
}
