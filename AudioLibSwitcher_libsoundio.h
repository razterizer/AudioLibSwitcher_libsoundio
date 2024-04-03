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
    SoundIoDevice* m_device = nullptr;
    
    // Buffer class
    struct Buffer
    {
      // Define your buffer structure here
      // For example:
      std::vector<short> data;
      // Other necessary members
      int sample_rate = 44100;
    };
    
    // Source class
    struct Source
    {
      float volume = 1.f;
      Buffer* buffer = nullptr;
      int sample_count = 0;
      int position = 0;
      bool is_playing = false;
      bool looping = false;
      float pitch = 0.f;
      bool want_pause = false;
      double seconds_offset = 0.0;
      SoundIoOutStream* outstream = nullptr;
      std::string m_stream_name;
      
      void write_sample_s16ne(char *ptr, short sample)
      {
        int16_t *buf = (int16_t *)ptr;
        //double range = (double)INT16_MAX - (double)INT16_MIN;
        //double val = sample * range / 2.0;
        //*buf = val;
        *buf = sample;
      }
      
      static void write_func_proxy(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
      {
        write_callback_static(outstream, frame_count_min, frame_count_max, static_cast<Source*>(outstream->userdata));
      };
      
      static void write_callback_static(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max, Source* source)
      {
        //std::cout << "source: " << source << std::endl;
        if (source != nullptr)
          source->write_callback(outstream, frame_count_min, frame_count_max);
      }
      
      void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
      {
        //std::cout << "buffer: " << buffer << std::endl;
        if (buffer == nullptr)
          return;
        double float_sample_rate = outstream->sample_rate;
        double seconds_per_frame = 1.0 / float_sample_rate;
        struct SoundIoChannelArea *areas;
        int err;
        int frames_left = frame_count_max;
        for (;;)
        {
          int frame_count = frames_left;
          if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count)))
          {
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
          }
          if (!frame_count)
            break;
          const SoundIoChannelLayout* layout = &outstream->layout;
          for (int frame = 0; frame < frame_count; ++frame)
          {
            short sample = buffer->data[frame];
            for (int channel = 0; channel < layout->channel_count; ++channel)
            {
              write_sample_s16ne(areas[channel].ptr, sample);
              areas[channel].ptr += areas[channel].step;
            }
          }
          seconds_offset += seconds_per_frame * frame_count;
          if ((err = soundio_outstream_end_write(outstream)))
          {
            if (err == SoundIoErrorUnderflow)
              return;
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
          }
          frames_left -= frame_count;
          if (frames_left <= 0)
            break;
        }
        soundio_outstream_pause(outstream, want_pause);
      }
      
      static void underflow_callback(struct SoundIoOutStream *outstream)
      {
        static int count = 0;
        fprintf(stderr, "underflow %d\n", count++);
      }
      
      Source(SoundIoDevice* device, std::string_view stream_name)
        : m_stream_name(stream_name)
      {
        outstream = soundio_outstream_create(device);
      }
      
      ~Source()
      {
        if (outstream != nullptr)
          soundio_outstream_destroy(outstream);
      }
      
      void init()
      {
        //std::cout << "name: " << m_stream_name << std::endl;
        outstream->underflow_callback = underflow_callback;
        outstream->name = m_stream_name.c_str();
        outstream->userdata = this; // Store pointer to this Source object
        outstream->write_callback = write_func_proxy;
      }
      
      void set_buffer_data_mono_16(SoundIoDevice* device)
      {
        if (soundio_device_supports_format(device, SoundIoFormatS16NE))
        {
          outstream->format = SoundIoFormatS16NE;
          //write_sample = write_sample_s16ne; // #FIXME: (?) Reimplement function pointer write_sample?
        }
        //else if (soundio_device_supports_format(device, SoundIoFormatFloat32NE))
        //{
        //  outstream->format = SoundIoFormatFloat32NE;
        //  write_sample = write_sample_float32ne;
        //}
        //else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE))
        //{
        //  outstream->format = SoundIoFormatFloat64NE;
        //  write_sample = write_sample_float64ne;
        //}
        //else if (soundio_device_supports_format(device, SoundIoFormatS32NE))
        //{
        //  outstream->format = SoundIoFormatS32NE;
        //  write_sample = write_sample_s32ne;
        //}
        else
        {
          throw std::runtime_error("No suitable device format available.");
        }
      }
    };
    
    class SourceManager
    {
    private:
      std::vector<std::unique_ptr<Source>> m_sources;
      SoundIo* m_soundio = nullptr;
      
    public:
      SourceManager(SoundIo* soundio)
        : m_soundio(soundio)
      {}
      
      size_t add_source(SoundIoDevice* device)
      {
        auto src_id = m_sources.size();
        m_sources.emplace_back(std::make_unique<Source>(device, "source-" + std::to_string(src_id)));
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
              buffer[i] += source->buffer->data[source->position] * source->volume;
              ++source->position;
            }
      }
      
      bool is_playing(size_t source_id) const
      {
        if (source_id < m_sources.size())
        {
          const auto& source = m_sources[source_id];
          // Check if the playback handle is valid and if the associated task is still running
          //return source->is_playing && source->playback_handle.valid() && source->playback_handle.wait_for(std::chrono::seconds(0)) == std::future_status::timeout;
          return false;
        }
        else
          return false;
      }
      
      void play(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          auto& source = m_sources[source_id];
          source->is_playing = true;
          
          if (int err = soundio_outstream_start(source->outstream); err != 0)
            throw std::runtime_error("unable to start device: " + std::string(soundio_strerror(err)));
        }
      }
      
      void pause(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          m_sources[source_id]->want_pause = true;
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
      
      void set_buffer_data_mono_16(SoundIoDevice* device, size_t source_id)
      {
        if (source_id < m_sources.size())
          m_sources[source_id]->set_buffer_data_mono_16(device);
      }
      
      void attach_buffer_to_source(size_t source_id, Buffer* buffer)
      {
        if (source_id < m_sources.size())
        {
          auto& source = m_sources[source_id];
          source->buffer = buffer;
          source->outstream->sample_rate = buffer->sample_rate;
        }
      }
      
      void open_stream(size_t source_id)
      {
        if (source_id < m_sources.size())
        {
          auto* source = m_sources[source_id].get();
          auto* outstream = source->outstream;
          if (int err = soundio_outstream_open(outstream); err != 0)
            throw std::runtime_error("unable to open device: " + std::string(soundio_strerror(err)));
          std::cout << "Software latency: " + std::to_string(outstream->software_latency) << std::endl;
          if (outstream->layout_error)
            throw std::runtime_error("unable to set channel layout: " + std::string(soundio_strerror(outstream->layout_error)));
          source->init();
        }
      }
      
      void clear_buffer(size_t source_id)
      {
        if (source_id < m_sources.size())
          m_sources[source_id]->buffer = nullptr;
      }
      
      void set_standard_params(size_t source_id)
      {
      
      }
    };
    
    class BufferManager
    {
      std::vector<std::unique_ptr<Buffer>> m_buffers;
      
    public:
      size_t add_buffer()
      {
        auto buf_id = m_buffers.size();
        m_buffers.emplace_back(std::make_unique<Buffer>());
        return buf_id;
      }
      
      bool remove_buffer(size_t buffer_id)
      {
        return stlutils::erase_at(m_buffers, buffer_id);
      }
      
      Buffer* get_buffer(size_t buffer_id)
      {
        if (buffer_id < m_buffers.size())
          return m_buffers[buffer_id].get();
        return nullptr;
      }
      
      void set_buffer_data_mono_16(size_t buffer_id, const std::vector<short>& short_buffer, int sample_rate)
      {
        if (buffer_id < m_buffers.size())
        {
          auto* buffer = m_buffers[buffer_id].get();
          buffer->data = short_buffer;
          buffer->sample_rate = sample_rate;
        }
      }
      
      // Additional functions for buffer management can be added here
    };
    
    std::unique_ptr<SourceManager> m_source_manager;
    std::unique_ptr<BufferManager> m_buffer_manager;
    
  public:
    virtual void init() override
    {
      m_source_manager = std::make_unique<SourceManager>(m_soundio);
      m_buffer_manager = std::make_unique<BufferManager>();
    
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
      if (m_device != nullptr)
        soundio_device_unref(m_device);
      if (m_soundio != nullptr)
        soundio_destroy(m_soundio);
    }
    
    unsigned int create_source() override
    {
      // Create a new source and return its ID
      auto src_id = m_source_manager->add_source(m_device);
      return static_cast<unsigned int>(src_id);
    }
    
    void destroy_source(unsigned int src_id) override
    {
      m_source_manager->remove_source(static_cast<size_t>(src_id));
    }
    
    unsigned int create_buffer() override
    {
      // Create a new buffer and return its ID
      auto buf_id = m_buffer_manager->add_buffer();
      return static_cast<unsigned int>(buf_id);
    }
    
    void destroy_buffer(unsigned int buf_id) override
    {
      m_buffer_manager->remove_buffer(static_cast<size_t>(buf_id));
    }
    
    virtual void play_source(unsigned int src_id) override
    {
      //fprintf(stderr, "Backend: %s\n", soundio_backend_name(m_soundio->current_backend));
      m_source_manager->play(src_id);
    }
    
    virtual bool is_source_playing(unsigned int src_id) override
    {
      return m_source_manager->is_playing(src_id);
    }
    
    virtual void pause_source(unsigned int src_id) override
    {
      m_source_manager->pause(src_id);
    }
    
    virtual void stop_source(unsigned int src_id) override
    {
      m_source_manager->stop(src_id);
    }
    
    virtual void set_source_volume(unsigned int src_id, float vol) override
    {
      m_source_manager->set_volume(src_id, vol);
    }
    
    virtual void set_source_pitch(unsigned int src_id, float pitch) override
    {
      m_source_manager->set_pitch(src_id, pitch);
    }
    
    virtual void set_source_looping(unsigned int src_id, bool loop) override
    {
      m_source_manager->set_looping(src_id, loop);
    }
    
    virtual void detach_source(unsigned int src_id) override
    {
      m_source_manager->clear_buffer(src_id);
    }
    
    virtual void set_source_standard_params(unsigned int src_id) override
    {
      m_source_manager->set_standard_params(src_id);
    }
    
    virtual void set_buffer_data_mono_16(unsigned int buf_id, const std::vector<short>& buffer, int sample_rate) override
    {
      m_buffer_manager->set_buffer_data_mono_16(buf_id, buffer, sample_rate);
    }
    
    virtual void attach_buffer_to_source(unsigned int src_id, unsigned int buf_id) override
    {
      m_source_manager->set_buffer_data_mono_16(m_device, src_id);
      
      auto* buffer = m_buffer_manager->get_buffer(buf_id);
      m_source_manager->attach_buffer_to_source(src_id, buffer);
      
      m_source_manager->open_stream(src_id);
    }
    
    virtual std::string check_error() override
    {
      return "";
    }
    
    SoundIo* get_soundio() const { return m_soundio; }
    
    SoundIoDevice* get_device() const { return m_device; }
  };
  
}
