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

#include <drc/internal/device-config.h>
#include <drc/types.h>
#include <algorithm>
#include <string>

namespace drc {

namespace {

uint16_t GetCRC(const uint8_t *location) {
  return (location[0]) | (location[1] << 8);
}


uint16_t CRC16(const uint8_t *data, size_t n) {
  size_t i, j;
  uint16_t crc = 0xffff;
  uint16_t mask = 0x8408;
  for (i = 0; i < n; ++i) {
    crc ^= data[i];
    for (j = 0; j < 8; ++j) {
      if (crc&1) {
        crc >>= 1;
        crc ^= mask;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool CheckCRC16(const uint8_t *data, size_t n) {
  uint16_t stored_crc = GetCRC(&data[n]);
  return (stored_crc == CRC16(data, n));
}

template <typename Iterator>
bool CopyIfCorrectCRC16(const uint8_t *start, size_t length, Iterator out) {
  if (CheckCRC16(start, length)) {
    std::copy_n(start, length, out);
    return true;
  }
  return false;
}

inline int16_t ReadInt16(const uint8_t *data) {
  int16_t rv = (data[0]) | (data[1] << 8);
  return rv;
}

inline int32_t ReadInt24(const uint8_t *data) {
  int32_t rv = (data[0] << 8) | (data[1] << 16) | (data[2] << 24);
  return rv>>8;
}

}  // namespace


DeviceConfig::DeviceConfig():
    wifi_auth_type_(0), wifi_auth_mode_(kWifiAuthModeUnknown),
    wifi_enc_type_(kWifiEncTypeUnknown), wowl_mac_({0}), wowl_setting_(0),
    last_channel_(0), board_main_version_(kBoardMainVersionUnknown),
    board_sub_version_(kBoardSubVersionUnknown), region_(kRegionUnsupported),
    volume_min_(0), volume_max_(0), gyro_zer_({0}), gyro_rot_({0}),
    gyro_spd_(0), language_(kLanguageUnsupported), opening_screen_(0),
    development_config_(0), accel_0g_({0}), accel_1g_({0}),
    panel_ref1_({20, 20}), panel_ref2_({834, 460}), panel_raw1_({195, 3818}),
    panel_raw2_({3877, 373}), language_bank_(0), language_version_(0),
    tv_remocon_version_(0), service_version_(0), opening_screen1_version_(0),
    opening_screen2_version_(0), initial_boot_flag_(0), lcd_setting_(0),
    tv_remocon_id1_({0}), tv_remocon_id2_({0}), airwave_setting_(0) {
}

void DeviceConfig::LoadFromBlob(const uint8_t *blob, size_t n) {
  // check expected length
  if (n != 0x300) return;

  // layout of page1
  if (!CheckCRC16(&blob[0xfd], 1) ||
      (blob[0xfd] != 0x03)) return;

  // layout of page2
  if (!CheckCRC16(&blob[0x1fd], 1) ||
      (blob[0x1fd] != 0x05)) return;

  // layout of page3
  if (!CheckCRC16(&blob[0x2fd], 1) ||
      (blob[0x2fd] != 0x04)) return;

  // wifi
  if (CheckCRC16(&blob[1], 0x6a)) {
    size_t len = 0x20;
    if (blob[0x21] < len) len = blob[0x21];

    wifi_ssid_ = std::string(reinterpret_cast<const char*>(&blob[1]), len);
    wifi_auth_type_ = blob[0x22];

    switch (blob[0x23]) {
      case kWifiAuthModeDisabled:
      case kWifiAuthModeNone:
      case kWifiAuthModeWPA:
      case kWifiAuthModeWPA2: {
        wifi_auth_mode_ = (WifiAuthMode)blob[0x23];
        break;
      }

      default: {
        wifi_auth_mode_ = kWifiAuthModeUnknown;
      }
    }

    switch (blob[0x24]) {
      case kWifiEncTypeNone:
      case kWifiEncTypeWEP:
      case kWifiEncTypeTKIP:
      case kWifiEncTypeAES: {
        wifi_enc_type_ = (WifiEncType)blob[0x24];
        break;
      }

      default: {
        wifi_enc_type_ = kWifiEncTypeUnknown;
      }
    }

    len = 0x40;
    if (blob[0x65] < len) len = blob[0x65];
    wifi_key_ = std::string(reinterpret_cast<const char*>(&blob[0x025]), len);
  }

  // we ignore freq stuff atm

  // wowl key
  CopyIfCorrectCRC16(&blob[0x6d], 0x10, wowl_key_.begin());

  // wowl mac
  CopyIfCorrectCRC16(&blob[0x7f], 6, wowl_mac_.begin());

  // wowl setting
  CopyIfCorrectCRC16(&blob[0x87], 1, &wowl_setting_);

  // last channel
  CopyIfCorrectCRC16(&blob[0x8a], 1, &last_channel_);


  // board info
  if (CheckCRC16(&blob[0x100], 1)) {
    switch (blob[0x100] & 0xf) {
      case kBoardMainVersionDK1:
      case kBoardMainVersionEP_DK2:
      case kBoardMainVersionDP1:
      case kBoardMainVersionDP2:
      case kBoardMainVersionDK3:
      case kBoardMainVersionDK4:
      case kBoardMainVersionDP3:
      case kBoardMainVersionDK5:
      case kBoardMainVersionDP4:
      case kBoardMainVersionDKMP:
      case kBoardMainVersionDP5:
      case kBoardMainVersionMASS:
      case kBoardMainVersionDKMP2:
      case kBoardMainVersionDRC_I: {
        board_main_version_ = (BoardMainVersion)(blob[0x100] & 0xf);
        break;
      }
      default: {
        board_main_version_ = kBoardMainVersionUnknown;
      }
    }

    switch (blob[0x100] >> 4) {
      case kBoardSubVersionDK1_EP_DK2:
      case kBoardSubVersionDP1_DK3:
      case kBoardSubVersionDK4:
      case kBoardSubVersionDP3:
      case kBoardSubVersionDK5:
      case kBoardSubVersionDP4:
      case kBoardSubVersionDKMP:
      case kBoardSubVersionDP5:
      case kBoardSubVersionMASS:
      case kBoardSubVersionDKMP2:
      case kBoardSubVersionDRC_I: {
        board_sub_version_ = (BoardSubVersion)(blob[0x100] >> 4);
        break;
      }
      default: {
        board_sub_version_ = kBoardSubVersionUnknown;
      }
    }
  } else if (CheckCRC16(&blob[0x180], 1)) {
    switch (blob[0x180] & 0xf) {
      case kBoardMainVersionDK1:
      case kBoardMainVersionEP_DK2:
      case kBoardMainVersionDP1:
      case kBoardMainVersionDP2:
      case kBoardMainVersionDK3:
      case kBoardMainVersionDK4:
      case kBoardMainVersionDP3:
      case kBoardMainVersionDK5:
      case kBoardMainVersionDP4:
      case kBoardMainVersionDKMP:
      case kBoardMainVersionDP5:
      case kBoardMainVersionMASS:
      case kBoardMainVersionDKMP2:
      case kBoardMainVersionDRC_I: {
        board_main_version_ = (BoardMainVersion)(blob[0x180] & 0xf);
        break;
      }
      default: {
        board_main_version_ = kBoardMainVersionUnknown;
      }
    }

    switch (blob[0x180] >> 4) {
      case kBoardSubVersionDK1_EP_DK2:
      case kBoardSubVersionDP1_DK3:
      case kBoardSubVersionDK4:
      case kBoardSubVersionDP3:
      case kBoardSubVersionDK5:
      case kBoardSubVersionDP4:
      case kBoardSubVersionDKMP:
      case kBoardSubVersionDP5:
      case kBoardSubVersionMASS:
      case kBoardSubVersionDKMP2:
      case kBoardSubVersionDRC_I: {
        board_sub_version_ = (BoardSubVersion)(blob[0x180] >> 4);
        break;
      }
      default: {
        board_sub_version_ = kBoardSubVersionUnknown;
      }
    }
  }


  // region
  if (CheckCRC16(&blob[0x103], 1)) {
    switch (blob[0x103]) {
      case kRegionJapan:
      case kRegionNorthAmerica:
      case kRegionEurope:
      case kRegionChina:
      case kRegionSouthKorea:
      case kRegionTaiwan:
      case kRegionAustralia: {
        region_ = (Region)blob[0x103];
        break;
      }
      default: {
        region_ = kRegionUnsupported;
      }
    }
  } else if (CheckCRC16(&blob[0x183], 1)) {
    switch (blob[0x183]) {
      case kRegionJapan:
      case kRegionNorthAmerica:
      case kRegionEurope:
      case kRegionChina:
      case kRegionSouthKorea:
      case kRegionTaiwan:
      case kRegionAustralia: {
        region_ = (Region)blob[0x183];
        break;
      }
      default: {
        region_ = kRegionUnsupported;
      }
    }
  }

  // volume min, max
  if (CheckCRC16(&blob[0x11a], 2)) {
    volume_min_ = blob[0x11a];
    volume_max_ = blob[0x11b];
  } else if (CheckCRC16(&blob[0x19a], 2)) {
    volume_min_ = blob[0x19a];
    volume_max_ = blob[0x19b];
  }

  // gyro
  if (CheckCRC16(&blob[0x12c], 19)) {
    gyro_zer_[0] = ReadInt24(&blob[0x12c]);
    gyro_zer_[1] = ReadInt24(&blob[0x12f]);
    gyro_zer_[2] = ReadInt24(&blob[0x132]);

    gyro_rot_[0] = ReadInt24(&blob[0x135]);
    gyro_rot_[1] = ReadInt24(&blob[0x138]);
    gyro_rot_[2] = ReadInt24(&blob[0x13b]);

    gyro_spd_ = blob[0x13e];
  } else if (CheckCRC16(&blob[0x1ac], 19)) {
    gyro_zer_[0] = ReadInt24(&blob[0x1ac]);
    gyro_zer_[1] = ReadInt24(&blob[0x1af]);
    gyro_zer_[2] = ReadInt24(&blob[0x1b2]);

    gyro_rot_[0] = ReadInt24(&blob[0x1b5]);
    gyro_rot_[1] = ReadInt24(&blob[0x1b8]);
    gyro_rot_[2] = ReadInt24(&blob[0x1bb]);

    gyro_spd_ = blob[0x1be];
  }

  // language
  if (CheckCRC16(&blob[0x200], 1)) {
    switch (blob[0x200]) {
      case kLanguageEnglish:
      case kLanguageFrench:
      case kLanguageSpanish:
      case kLanguagePortuguese:
      case kLanguageEnglish2:
      case kLanguageFrench2:
      case kLanguageSpanish2:
      case kLanguagePortuguese2:
      case kLanguageDutch:
      case kLanguageItalian:
      case kLanguageGerman:
      case kLanguageRussian:
      case kLanguageJapanese:
      case kLanguageChineseSimpified:
      case kLanguageKorean:
      case kLanguageChineseTraditional: {
        language_ = (Language)blob[0x200];
        break;
      }
      default: {
        language_ = kLanguageUnsupported;
      }
    }
  }

  // opening screen
  CopyIfCorrectCRC16(&blob[0x203], 1, &opening_screen_);

  // development config
  CopyIfCorrectCRC16(&blob[0x206], 1, &development_config_);

  // accel
  if (CheckCRC16(&blob[0x213], 12)) {
    accel_0g_[0] = ReadInt16(&blob[0x213]);
    accel_0g_[1] = ReadInt16(&blob[0x213 + 2]);
    accel_0g_[2] = ReadInt16(&blob[0x213 + 4]);

    accel_1g_[0] = ReadInt16(&blob[0x213 + 6]);
    accel_1g_[1] = ReadInt16(&blob[0x213 + 8]);
    accel_1g_[2] = ReadInt16(&blob[0x213 + 10]);
  } else if (CheckCRC16(&blob[0x11e], 12)) {
    accel_0g_[0] = ReadInt16(&blob[0x11e]);
    accel_0g_[1] = ReadInt16(&blob[0x11e + 2]);
    accel_0g_[2] = ReadInt16(&blob[0x11e + 4]);

    accel_1g_[0] = ReadInt16(&blob[0x11e + 6]);
    accel_1g_[1] = ReadInt16(&blob[0x11e + 8]);
    accel_1g_[2] = ReadInt16(&blob[0x11e + 10]);
  } else if (CheckCRC16(&blob[0x19e], 12)) {
    accel_0g_[0] = ReadInt16(&blob[0x19e]);
    accel_0g_[1] = ReadInt16(&blob[0x19e + 2]);
    accel_0g_[2] = ReadInt16(&blob[0x19e + 4]);

    accel_1g_[0] = ReadInt16(&blob[0x19e + 6]);
    accel_1g_[1] = ReadInt16(&blob[0x19e + 8]);
    accel_1g_[2] = ReadInt16(&blob[0x19e + 10]);
  }

  // panel
  if (CheckCRC16(&blob[0x244], 16)) {
    panel_ref1_[0] = ReadInt16(&blob[0x244]);
    panel_ref1_[1] = ReadInt16(&blob[0x244 + 2]);
    panel_ref2_[0] = ReadInt16(&blob[0x244 + 4]);
    panel_ref2_[1] = ReadInt16(&blob[0x244 + 6]);
    panel_raw1_[0] = ReadInt16(&blob[0x244 + 8]);
    panel_raw1_[1] = ReadInt16(&blob[0x244 + 10]);
    panel_raw2_[0] = ReadInt16(&blob[0x244 + 12]);
    panel_raw2_[1] = ReadInt16(&blob[0x244 + 14]);
  } else if (CheckCRC16(&blob[0x153], 16)) {
    panel_ref1_[0] = ReadInt16(&blob[0x153]);
    panel_ref1_[1] = ReadInt16(&blob[0x153 + 2]);
    panel_ref2_[0] = ReadInt16(&blob[0x153 + 4]);
    panel_ref2_[1] = ReadInt16(&blob[0x153 + 6]);
    panel_raw1_[0] = ReadInt16(&blob[0x153 + 8]);
    panel_raw1_[1] = ReadInt16(&blob[0x153 + 10]);
    panel_raw2_[0] = ReadInt16(&blob[0x153 + 12]);
    panel_raw2_[1] = ReadInt16(&blob[0x153 + 14]);
  } else if (CheckCRC16(&blob[0x1d3], 16)) {
    panel_ref1_[0] = ReadInt16(&blob[0x1d3]);
    panel_ref1_[1] = ReadInt16(&blob[0x1d3 + 2]);
    panel_ref2_[0] = ReadInt16(&blob[0x1d3 + 4]);
    panel_ref2_[1] = ReadInt16(&blob[0x1d3 + 6]);
    panel_raw1_[0] = ReadInt16(&blob[0x1d3 + 8]);
    panel_raw1_[1] = ReadInt16(&blob[0x1d3 + 10]);
    panel_raw2_[0] = ReadInt16(&blob[0x1d3 + 12]);
    panel_raw2_[1] = ReadInt16(&blob[0x1d3 + 14]);
  }


  // language bank, ver
  if (CheckCRC16(&blob[0x256], 1)) {
    language_bank_ = blob[0x256];
    language_version_ = (blob[0x257] << 16) | (blob[0x258] << 8) |
                        (blob[0x259]);
  }

  // tv remocon version
  CopyIfCorrectCRC16(&blob[0x25c], 4,
                     reinterpret_cast<uint8_t*>(&tv_remocon_version_));

  // service version
  CopyIfCorrectCRC16(&blob[0x262], 4,
                     reinterpret_cast<uint8_t*>(&service_version_)) ||
    CopyIfCorrectCRC16(&blob[0x165], 4,
                       reinterpret_cast<uint8_t*>(&service_version_)) ||
    CopyIfCorrectCRC16(&blob[0x1e5], 4,
                       reinterpret_cast<uint8_t*>(&service_version_));

  // opening screen1 version
  CopyIfCorrectCRC16(&blob[0x268], 4,
                     reinterpret_cast<uint8_t*>(&opening_screen1_version_));

  // opening screen2 version
  CopyIfCorrectCRC16(&blob[0x26e], 4,
                     reinterpret_cast<uint8_t*>(&opening_screen2_version_));

  // initial boot flag
  CopyIfCorrectCRC16(&blob[0x274], 1, &initial_boot_flag_);

  // lcd levels
  if (CheckCRC16(&blob[0x277], 13)) {
    lcd_levels_.modes[0].level = blob[0x277];
    lcd_levels_.modes[0].frequency = blob[0x278];
    lcd_levels_.modes[1].level = blob[0x279];
    lcd_levels_.modes[1].frequency = blob[0x27a];
    lcd_levels_.modes[2].level = blob[0x27b];
    lcd_levels_.modes[2].frequency = blob[0x27c];
    lcd_levels_.modes[3].level = blob[0x27d];
    lcd_levels_.modes[3].frequency = blob[0x27e];
    lcd_levels_.modes[4].level = blob[0x27f];
    lcd_levels_.modes[4].frequency = blob[0x280];
    lcd_levels_.modes[5].level = blob[0x281];
    lcd_levels_.modes[5].frequency = blob[0x282];
    lcd_levels_.threshold = blob[0x283];
  }

  // lcd setting
  CopyIfCorrectCRC16(&blob[0x286], 1, &lcd_setting_);

  // tv remocon ids
  if (CheckCRC16(&blob[0x289], 10)) {
    std::copy_n(&blob[0x289], 5, tv_remocon_id1_.begin());
    std::copy_n(&blob[0x28e], 5, tv_remocon_id2_.begin());
  }

  // airwave setting
  CopyIfCorrectCRC16(&blob[0x296], 1, &airwave_setting_);
}

DeviceConfig::~DeviceConfig() {
}


}  // namespace drc
