/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of libsoundio, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */
//#include <soundio/soundio.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdint.h>
//#include <math.h>
#include "AudioLibSwitcher_libsoundio.h"

/*
static void write_sample_s16ne(char *ptr, double sample)
{
  int16_t *buf = (int16_t *)ptr;
  double range = (double)INT16_MAX - (double)INT16_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}
static void write_sample_s32ne(char *ptr, double sample)
{
  int32_t *buf = (int32_t *)ptr;
  double range = (double)INT32_MAX - (double)INT32_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}
static void write_sample_float32ne(char *ptr, double sample)
{
  float *buf = (float *)ptr;
  *buf = sample;
}
static void write_sample_float64ne(char *ptr, double sample)
{
  double *buf = (double *)ptr;
  *buf = sample;
}
static void (*write_sample)(char *ptr, double sample);
static const double PI = 3.14159265358979323846264338328;
static double seconds_offset = 0.0;
static volatile bool want_pause = false;
static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
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
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    double pitch = 440.0;
    double radians_per_second = pitch * 2.0 * PI;
    for (int frame = 0; frame < frame_count; ++frame)
    {
      double sample = sinf((seconds_offset + frame * seconds_per_frame) * radians_per_second);
      for (int channel = 0; channel < layout->channel_count; ++channel)
      {
        write_sample(areas[channel].ptr, sample);
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
*/

int main(int argc, char **argv)
{
  audio::AudioLibSwitcher_libsoundio libsoundio;
  
  enum SoundIoBackend backend = SoundIoBackendNone;
  bool raw = false;
  char *stream_name = NULL;
  double latency = 0.0;
  int sample_rate = 0;
  int err = 0;
  
  libsoundio.init();
  
  auto* soundio = libsoundio.get_soundio();
  
  auto* device = libsoundio.get_device();
  
  unsigned int src_id = libsoundio.create_source();
  unsigned int buf_id = libsoundio.create_buffer();
  
  
  
  libsoundio.destroy_buffer(buf_id);
  libsoundio.destroy_source(src_id);
  
  libsoundio.finish();
  
  return 0;
}
