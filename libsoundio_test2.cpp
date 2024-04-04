#include "AudioLibSwitcher_libsoundio.h"


int main(int argc, char **argv)
{
  audio::AudioLibSwitcher_libsoundio libsoundio;
  
  // ////////
  
  std::vector<short> data;
  int sample_rate = 44100;
  float dur = 3.f;
  float freq = 440.f;
  size_t num_samples = static_cast<int>(dur * sample_rate);
  float dt = 1.f/sample_rate;
  
  data.resize(num_samples);
  for (size_t i = 0; i < num_samples; ++i)
  {
    float t = i * dt;
    float sample = std::sin(math::c_2pi * freq * t);
    data[i] = static_cast<short>(32767 * sample);
    //std::cout << data[i] << ", ";
  }
  
  // ////////
  
  libsoundio.init();
  
  auto* soundio = libsoundio.get_soundio();
  
  auto* device = libsoundio.get_device();
  
  unsigned int src_id = libsoundio.create_source();
  unsigned int buf_id = libsoundio.create_buffer();
  
  libsoundio.set_buffer_data_mono_16(buf_id, data, sample_rate);
  libsoundio.attach_buffer_to_source(src_id, buf_id);
  libsoundio.play_source(src_id);
  
#if 1
  //for (;;)
  while (libsoundio.is_source_playing(src_id))
  {
    soundio_flush_events(soundio);
    int c = getc(stdin);
    if (c == 'q')
      break;
  }
#else
  soundio_flush_events(soundio);
#endif
  
  libsoundio.destroy_buffer(buf_id);
  libsoundio.destroy_source(src_id);
  
  libsoundio.finish();
  
  return 0;
}
