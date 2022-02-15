// Copyright (c) 2013, Mema Hacking, All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

/*
 * A very small C wrapper for libdrc in order to make it usable in C
 * applications. It only exposes a very small portion of the libdrc API. For
 * full functionality, please use the C++ library instead.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct drc_streamer;

enum drc_pixel_format {
  DRC_RGB,
  DRC_RGBA,
  DRC_BGR,
  DRC_BGRA,
  DRC_RGB565,
};

enum drc_flipping_mode {
  DRC_NO_FLIP,
  DRC_FLIP_VERTICALLY,
};

struct drc_streamer* drc_new_streamer();
void drc_delete_streamer(struct drc_streamer* self);
int drc_start_streamer(struct drc_streamer* self);
void drc_stop_streamer(struct drc_streamer* self);
void drc_push_vid_frame(struct drc_streamer* self, const unsigned char* buffer,
                        unsigned int size, unsigned short width,
                        unsigned short height, enum drc_pixel_format pixfmt,
                        enum drc_flipping_mode flipmode);
void drc_enable_system_input_feeder(struct drc_streamer* self);
void drc_shutdown_pad(struct drc_streamer* self);

#ifdef __cplusplus
}
#endif
