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

#include <array>
#include <drc/types.h>
#include <string>

namespace drc {

class DeviceConfig {
 public:
  struct LCDLevels {
    LCDLevels(): threshold(0) {
    }

    struct Level {
      Level(): level(0), frequency(0) {
      }

      uint8_t level;
      uint8_t frequency;
    } modes[6];
    uint8_t threshold;
  };

  enum WifiAuthMode {
    kWifiAuthModeDisabled = 0x0,
    kWifiAuthModeNone = 0x1,
    kWifiAuthModeWPA = 0x4,
    kWifiAuthModeWPA2 = 0x80,
    kWifiAuthModeUnknown = 0xffffffff,
  };

  enum WifiEncType {
    kWifiEncTypeNone = 0x0,
    kWifiEncTypeWEP = 0x1,
    kWifiEncTypeTKIP = 0x2,
    kWifiEncTypeAES = 0x3,
    kWifiEncTypeUnknown = 0xffffffff,
  };

  enum BoardMainVersion {
    kBoardMainVersionDK1 = 0x0,
    kBoardMainVersionEP_DK2 = 0x1,
    kBoardMainVersionDP1 = 0x2,
    kBoardMainVersionDP2 = 0x3,
    kBoardMainVersionDK3 = 0x4,
    kBoardMainVersionDK4 = 0x5,
    kBoardMainVersionDP3 = 0x6,
    kBoardMainVersionDK5 = 0x7,
    kBoardMainVersionDP4 = 0x8,
    kBoardMainVersionDKMP = 0x9,
    kBoardMainVersionDP5 = 0xa,
    kBoardMainVersionMASS = 0xb,
    kBoardMainVersionDKMP2 = 0xc,
    kBoardMainVersionDRC_I = 0xd,
    kBoardMainVersionUnknown = 0xffffffff,
  };

  enum BoardSubVersion {
    kBoardSubVersionDK1_EP_DK2 = 0x0,
    kBoardSubVersionDP1_DK3 = 0x1,
    kBoardSubVersionDK4 = 0x2,
    kBoardSubVersionDP3 = 0x3,
    kBoardSubVersionDK5 = 0x4,
    kBoardSubVersionDP4 = 0x5,
    kBoardSubVersionDKMP = 0x6,
    kBoardSubVersionDP5 = 0x7,
    kBoardSubVersionMASS = 0x8,
    kBoardSubVersionDKMP2 = 0x9,
    kBoardSubVersionDRC_I = 0xa,
    kBoardSubVersionUnknown = 0xffffffff,
  };


  enum Region {
    kRegionJapan = 0x0,
    kRegionNorthAmerica = 0x1,
    kRegionEurope = 0x2,
    kRegionChina = 0x3,
    kRegionSouthKorea = 0x4,
    kRegionTaiwan = 0x5,
    kRegionAustralia = 0x6,
    kRegionUnsupported = 0xffffffff,
  };

  enum Language {
    kLanguageEnglish = 0x0,
    kLanguageFrench = 0x1,
    kLanguageSpanish = 0x2,
    kLanguagePortuguese = 0x3,
    kLanguageEnglish2 = 0x4,
    kLanguageFrench2 = 0x5,
    kLanguageSpanish2 = 0x6,
    kLanguagePortuguese2 = 0x7,
    kLanguageDutch = 0x8,
    kLanguageItalian = 0x9,
    kLanguageGerman = 0xa,
    kLanguageRussian = 0xb,
    kLanguageJapanese = 0xc,
    kLanguageChineseSimpified = 0xd,
    kLanguageKorean = 0xe,
    kLanguageChineseTraditional = 0xf,
    kLanguageUnsupported = 0xffffffff,
  };

  DeviceConfig();
  void LoadFromBlob(const uint8_t *blob, size_t n);
  virtual ~DeviceConfig();

// Page 1
  inline const std::string& GetWifiSSID() const {
    return wifi_ssid_;
  }

  inline uint8_t GetWifiAuthType() const {
    return wifi_auth_type_;
  }

  inline WifiAuthMode GetWifiAuthMode() const {
    return wifi_auth_mode_;
  }

  inline WifiEncType GetWifiEncType() const {
    return wifi_enc_type_;
  }

  inline const std::string& GetWifiKey() const {
    return wifi_key_;
  }

  inline const std::array<uint8_t, 16> GetWowlKey() const {
    return wowl_key_;
  }

  inline const std::array<uint8_t, 6> &GetWowlMac() const {
    return wowl_mac_;
  }

  inline uint8_t GetWowlSetting() const {
    return wowl_setting_;
  }

  inline uint8_t GetLastChannel() const {
    return last_channel_;
  }

// Page 2
  inline BoardMainVersion GetBoardMainVersion() const {
    return board_main_version_;
  }

  inline BoardSubVersion GetBoardSubVersion() const {
    return board_sub_version_;
  }

  Region GetRegion() const {
    return region_;
  }

  // sku
  // mic gain
  inline uint8_t GetVolumeMin() const {
    return volume_min_;
  }

  inline uint8_t GetVolumeMax() const {
    return volume_max_;
  }

  inline const std::array<int32_t, 3> &GetGyroZer() const {
    return gyro_zer_;
  }

  inline const std::array<int32_t, 3> &GetGyroRot() const {
    return gyro_rot_;
  }

  inline uint8_t GetGyroSpd() const {
    return gyro_spd_;
  }

// Page 3

  inline Language GetLanguage() const {
    return language_;
  }

  inline uint8_t GetOpeningScreen() const {
    return opening_screen_;
  }

  inline uint8_t GetDevelopmentConfig() const {
    return development_config_;
  }

  // mic in out
  inline const std::array<int16_t, 3> &GetAccel0G() const {
    return accel_0g_;
  }

  inline const std::array<int16_t, 3> &GetAccel1G() const {
    return accel_1g_;
  }

  inline const std::array<int16_t, 2> &GetPanelRef1() const {
    return panel_ref1_;
  }

  inline const std::array<int16_t, 2> &GetPanelRef2() const {
    return panel_ref2_;
  }

  inline const std::array<int16_t, 2> &GetPanelRaw1() const {
    return panel_raw1_;
  }

  inline const std::array<int16_t, 2> &GetPanelRaw2() const {
    return panel_raw2_;
  }


  inline uint8_t GetLanguageBank() const {
    return language_bank_;
  }

  inline uint32_t GetLanguageVersion() const {
    return language_version_;
  }

  inline uint32_t GetTVRemoconVersion() const {
    return tv_remocon_version_;
  }

  inline uint32_t GetServiceVersion() const {
    return service_version_;
  }

  inline uint32_t GetOpeningScreen1Version() const {
    return opening_screen1_version_;
  }

  inline uint32_t GetOpeningScreen2Version() const {
    return opening_screen2_version_;
  }

  inline uint8_t GetInitialBootFlag() const {
    return initial_boot_flag_;
  }

  inline const LCDLevels &GetLCDLevels() const {
    return lcd_levels_;
  }

  inline uint8_t GetLCDSetting() const {
    return lcd_setting_;
  }

  inline const std::array<uint8_t, 5> &GetTVRemoconId1() const {
    return tv_remocon_id1_;
  }

  inline const std::array<uint8_t, 5> &GetTVRemoconId2() const {
    return tv_remocon_id2_;
  }

  inline uint8_t GetAirwaveSetting() const {
    return airwave_setting_;
  }

 private:
  std::string wifi_ssid_;
  uint8_t wifi_auth_type_;
  WifiAuthMode wifi_auth_mode_;
  WifiEncType wifi_enc_type_;
  std::string wifi_key_;
  std::array<uint8_t, 16> wowl_key_;
  std::array<uint8_t, 6> wowl_mac_;
  uint8_t wowl_setting_;
  uint8_t last_channel_;

  BoardMainVersion board_main_version_;
  BoardSubVersion board_sub_version_;
  Region region_;
  uint8_t volume_min_;
  uint8_t volume_max_;
  std::array<int32_t, 3> gyro_zer_;
  std::array<int32_t, 3> gyro_rot_;
  uint8_t gyro_spd_;

  Language language_;
  uint8_t opening_screen_;
  uint8_t development_config_;
  std::array<int16_t, 3> accel_0g_;
  std::array<int16_t, 3> accel_1g_;
  std::array<int16_t, 2> panel_ref1_;
  std::array<int16_t, 2> panel_ref2_;
  std::array<int16_t, 2> panel_raw1_;
  std::array<int16_t, 2> panel_raw2_;
  uint8_t language_bank_;
  uint32_t language_version_;
  uint32_t tv_remocon_version_;
  uint32_t service_version_;
  uint32_t opening_screen1_version_;
  uint32_t opening_screen2_version_;
  uint8_t initial_boot_flag_;
  LCDLevels lcd_levels_;
  uint8_t lcd_setting_;
  std::array<uint8_t, 5> tv_remocon_id1_;
  std::array<uint8_t, 5> tv_remocon_id2_;

  uint8_t airwave_setting_;
};


}  // namespace drc
