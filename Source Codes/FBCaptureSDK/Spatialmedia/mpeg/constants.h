#pragma once

#include <stdint.h>

typedef enum {
  SM_NONE, SM_TOP_BOTTOM, SM_LEFT_RIGHT
} enMode;
typedef enum {
  NONE, EQUIRECT, CUBEMAP, SINGLE_FISHEYE
} projection;

struct AudioMetadata {
  AudioMetadata() {
    ambisonic_order = 1;
    ambisonic_type = "periphonic";
    ambisonic_channel_ordering = "ACN";
    ambisonic_normalization = "SN3D";
    for (uint32_t t = 0; t < 4; t++)
      channel_map[t] = t;
  };
  uint32_t ambisonic_order;
  std::string ambisonic_type;
  std::string ambisonic_channel_ordering;
  std::string ambisonic_normalization;
  uint32_t channel_map[4];
};

// MPEG-4 constants
namespace constants {

  static const char *TRAK_TYPE_VIDE = "vide";

  // Leaf types.
  static const char *TAG_STCO = "stco";
  static const char *TAG_CO64 = "co64";
  static const char *TAG_FREE = "free";
  static const char *TAG_MDAT = "mdat";
  static const char *TAG_XML = "xml ";
  static const char *TAG_HDLR = "hdlr";
  static const char *TAG_FTYP = "ftyp";
  static const char *TAG_ESDS = "esds";
  static const char *TAG_SOUN = "soun";
  static const char *TAG_SA3D = "SA3D";

  // Container types.
  static const char *TAG_MOOV = "moov";
  static const char *TAG_UDTA = "udta";
  static const char *TAG_META = "meta";
  static const char *TAG_TRAK = "trak";
  static const char *TAG_MDIA = "mdia";
  static const char *TAG_MINF = "minf";
  static const char *TAG_STBL = "stbl";
  static const char *TAG_STSD = "stsd";
  static const char *TAG_UUID = "uuid";
  static const char *TAG_WAVE = "wave";

  // Sound sample descriptions.
  static const char *TAG_NONE = "NONE";
  static const char *TAG_RAW_ = "raw ";
  static const char *TAG_TWOS = "twos";
  static const char *TAG_SOWT = "sowt";
  static const char *TAG_FL32 = "fl32";
  static const char *TAG_FL64 = "fl64";
  static const char *TAG_IN24 = "in24";
  static const char *TAG_IN32 = "in32";
  static const char *TAG_ULAW = "ulaw";
  static const char *TAG_ALAW = "alaw";
  static const char *TAG_LPCM = "lpcm";
  static const char *TAG_MP4A = "mp4a";

  static const char * SOUND_SAMPLE_DESCRIPTIONS[12] = {
    TAG_NONE,
    TAG_RAW_,
    TAG_TWOS,
    TAG_SOWT,
    TAG_FL32,
    TAG_FL64,
    TAG_IN24,
    TAG_IN32,
    TAG_ULAW,
    TAG_ALAW,
    TAG_LPCM,
    TAG_MP4A
  };

  static const char * CONTAINERS_LIST[20] = {
    TAG_MDIA,
    TAG_MINF,
    TAG_MOOV,
    TAG_STBL,
    TAG_STSD,
    TAG_TRAK,
    TAG_UDTA,
    TAG_WAVE,

    TAG_NONE,
    TAG_RAW_,
    TAG_TWOS,
    TAG_SOWT,
    TAG_FL32,
    TAG_FL64,
    TAG_IN24,
    TAG_IN32,
    TAG_ULAW,
    TAG_ALAW,
    TAG_LPCM,
    TAG_MP4A
  };

  enum Type {
    Box = 0,
    Container,
    ContainerLeaf,
    None
  };

};  // End of namespace constants

