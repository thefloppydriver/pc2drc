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

#include <cassert>
#include <cstdio>
#include <drc/internal/h264-encoder.h>
#include <drc/screen.h>
#include <drc/types.h>
#include <tuple>
#include <vector>

extern "C" {
#include <x264.h>
}

namespace drc {

namespace {
const char* const kEncoderQuality = "ultrafast"; //"slow";

// Enable intra prediction to flush decoding errors away. In practice we
// shouldn't need this (the DRC will send libdrc a message when a frame fails
// to be decoded), but at the moment it helps make decoding errors better.
const bool kEnableIntraRefresh = true;

// Debugging feature to dump the encoded h264 stream to a .h264 file. If
// enabled, a file called "dump.h264" will be created in the current directory.
const bool kDebugDumpToFile = false;

void LogEncoderMessages(void* data, int i_level, const char* format,
                        va_list va) {
}

void DumpH264Headers(FILE* fp) {
  u8 nal_start_code[] = { 0x00, 0x00, 0x00, 0x01 };
  u8 gamepad_sps[] = { 0x67, 0x64, 0x00, 0x20, 0xac, 0x2b, 0x40,
                       0x6c, 0x1e, 0xf3, 0x68 };
  u8 gamepad_pps[] = { 0x68, 0xee, 0x06, 0x0c, 0xe8 };

  fwrite(nal_start_code, sizeof (nal_start_code), 1, fp);
  fwrite(gamepad_sps, sizeof (gamepad_sps), 1, fp);
  fwrite(nal_start_code, sizeof (nal_start_code), 1, fp);
  fwrite(gamepad_pps, sizeof (gamepad_pps), 1, fp);
}

size_t NalEscape(byte* dst, const byte* src, size_t size) {
  byte* dst_start = dst;
  const byte* end = src + size;
  if (src < end) {
    *dst++ = *src++;
  }
  if (src < end) {
    *dst++ = *src++;
  }

  while (src < end) {
    if (src[0] <= 0x03 && !dst[-2] && !dst[-1]) {
      *dst++ = 0x03;
    }
    *dst++ = *src++;
  }

  return dst - dst_start;
}

void DumpH264Frame(FILE* fp, const H264ChunkArray& chunks, bool is_idr) {
  // TODO(delroth): make this a property of the encoder.
  static u32 frame_number = 0;

  u8 nal_start_code[] = { 0x00, 0x00, 0x00, 0x01 };
  u8 nal_idr_frame[] = { 0x25, 0xb8, 0x04, 0xff };
  u8 nal_p_frame[] = { 0x21, 0xe0, 0x03, 0xff };

  fwrite(nal_start_code, sizeof (nal_start_code), 1, fp);

  if (is_idr) {
    frame_number = 0;
    fwrite(nal_idr_frame, sizeof (nal_idr_frame), 1, fp);
  } else {
    frame_number = (frame_number + 1) & 0xFF;
    nal_p_frame[1] |= frame_number >> 3;
    nal_p_frame[2] |= frame_number << 5;
    fwrite(nal_p_frame, sizeof (nal_p_frame), 1, fp);
  }

  size_t nal_length = 0;
  for (const auto& chunk : chunks) {
    nal_length += std::get<1>(chunk);
  }

  std::vector<byte> unescaped;
  unescaped.reserve(nal_length);
  for (const auto& chunk : chunks) {
    unescaped.insert(unescaped.end(), std::get<0>(chunk),
                     std::get<0>(chunk) + std::get<1>(chunk));
  }

  std::vector<byte> escaped(2 * nal_length);
  size_t escaped_size = NalEscape(escaped.data(), unescaped.data(),
                                  unescaped.size());
  fwrite(escaped.data(), escaped_size, 1, fp);
}

}  // namespace

H264Encoder::H264Encoder()
    : encoder_(nullptr), dump_file_(nullptr) {
  CreateEncoder();
}

H264Encoder::~H264Encoder() {
  DestroyEncoder();
}

void H264Encoder::CreateEncoder() {
  x264_param_t param;

  x264_param_default_preset(&param, kEncoderQuality, "zerolatency");
  param.i_width = kScreenWidth;
  param.i_height = kScreenHeight;

  param.analyse.inter &= ~X264_ANALYSE_PSUB16x16;

  if (kEnableIntraRefresh) {
    param.i_keyint_min = 10;
    param.i_keyint_max = 30;
  } else {
    param.i_keyint_min = param.i_keyint_max = X264_KEYINT_MAX_INFINITE;
  }

  param.i_scenecut_threshold = -1;
  param.i_csp = X264_CSP_I420;
  param.b_cabac = 1;
  param.b_interlaced = 0;
  param.i_bframe = 0;
  param.i_bframe_pyramid = 0;
  param.i_frame_reference = 1;
  param.b_constrained_intra = 1;
  param.b_intra_refresh = kEnableIntraRefresh;
  param.analyse.i_weighted_pred = 0;
  param.analyse.b_weighted_bipred = 0;
  param.analyse.b_transform_8x8 = 0; //maybe change this to 1..?
  param.analyse.i_chroma_qp_offset = 0;

  // Set QP = 32 for all frames.
  param.rc.i_rc_method = X264_RC_CQP;
  param.rc.i_qp_constant = param.rc.i_qp_min = param.rc.i_qp_max = 32;
  param.rc.f_ip_factor = 1.0;

  // Do not output SPS/PPS/SEI/unit delimeters.
  param.b_repeat_headers = 0;
  param.b_aud = 0;

  // return macroblock rows intead of NAL units
  // XXX x264 must also be modified to not produce macroblocks utilizing
  // planar prediction. this isn't toggleable at runtime for now...
  param.b_drh_mode = 1;

  // Yield one complete frame serially.
  param.i_threads = 1;
  param.b_sliced_threads = 0;
  param.i_slice_count = 1;
  param.nalu_process = H264Encoder::ProcessNalUnitTrampoline;

  // Logging parameters.
  param.i_log_level = X264_LOG_INFO;
  param.pf_log = LogEncoderMessages;

  x264_param_apply_profile(&param, "main");

  encoder_ = x264_encoder_open(&param);
  assert(x264_encoder_maximum_delayed_frames(encoder_) == 0 ||
             "Encoder parameters will cause frames to be delayed");

  // If dumping to file, open the dump file here.
  if (kDebugDumpToFile) {
    dump_file_ = fopen("dump.h264", "wb");
    if (dump_file_) {
      DumpH264Headers(dump_file_);
    }
  }
}

void H264Encoder::DestroyEncoder() {
  if (encoder_) {
    x264_encoder_close(encoder_);
    encoder_ = nullptr;
  }
  if (dump_file_) {
    fclose(dump_file_);
    dump_file_ = nullptr;
  }
}

const H264ChunkArray& H264Encoder::Encode(const std::vector<byte>& frame,
                                          bool idr) {
  x264_picture_t input;
  x264_picture_init(&input);

  // x264 requires us to pass a non-const pointer even though it does not
  // modify the input data. Cheat a bit to keep a clean interface.
  byte* data = const_cast<byte*>(frame.data());

  // Used to find ourselves back from x264 callbacks.
  input.opaque = this;

  // Input image parameters.
  input.img.i_csp = X264_CSP_I420;
  input.img.i_plane = 3;

  // Plane 0: Y, offset 0, 1 byte per horizontal pixel
  input.img.i_stride[0] = kScreenWidth;
  input.img.plane[0] = data;

  // Plane 1: U, offset sizeof(Y), 0.5 byte per horizontal pixel
  input.img.i_stride[1] = kScreenWidth / 2;
  input.img.plane[1] = input.img.plane[0] + (kScreenWidth * kScreenHeight);

  // Plane 2: V, offset sizeof(Y)+sizeof(U), 0.5 byte per horizontal pixel
  input.img.i_stride[2] = kScreenWidth / 2;
  input.img.plane[2] = input.img.plane[1] + (kScreenWidth * kScreenHeight) / 4;

  if (idr) {
    input.i_type = X264_TYPE_IDR;
  } else {
    input.i_type = X264_TYPE_P;
  }

  // Since we use a nalu_process callback, these will/might not contain correct
  // data (especially since we do not behave well and NAL encode the NAL units
  // sent to us by x264).
  x264_nal_t* nals;
  int nals_count;
  x264_picture_t output;

  // Reinitialize frame-related state.
  num_chunks_encoded_ = 0;
  curr_frame_idr_ = false;

  // Encode!
  x264_encoder_encode(encoder_, &nals, &nals_count, &input, &output);

  // If debug dumping is enabled, create a frame from the generated chunks.
  if (dump_file_) {
    DumpH264Frame(dump_file_, chunks_, idr);
  }

  return chunks_;
}

void H264Encoder::ProcessNalUnit(x264_nal_t* nal) {
  if (nal->i_type == NAL_SEI) {
    return;
  }
  int mb_per_frame = ((kScreenWidth + 15) / 16) * ((kScreenHeight + 15) / 16);
  int mb_per_chunk = mb_per_frame / kH264ChunksPerFrame;
  int chunk_idx = nal->i_first_mb / mb_per_chunk;
  chunks_[chunk_idx] = std::make_tuple(nal->p_payload, nal->i_payload);

  num_chunks_encoded_++;
  assert(num_chunks_encoded_ <= 5);
  if (num_chunks_encoded_ == 5) {
    curr_frame_idr_ = nal->i_ref_idc != NAL_PRIORITY_DISPOSABLE &&
                      nal->i_type == NAL_SLICE_IDR;
  }
}

void H264Encoder::ProcessNalUnitTrampoline(x264_t* h, x264_nal_t* nal,
                                           void* opaque) {
  H264Encoder* enc = static_cast<H264Encoder*>(opaque);
  enc->ProcessNalUnit(nal);
}

}  // namespace drc
