#pragma once

#ifndef __METADATA_UTILS_H__
#define __METADATA_UTILS_H__

#include <stdint.h>
#include <string>
#include <fstream>
#include <map>

#include "Encoder\Common.h"
#include "mxml/mxml.h"
#include "mpeg/constants.h"

// Utilities for examining/injecting spatial media metadata in MP4/MOV files."""

static const char *MPEG_FILE_EXTENSIONS[2] = { ".mp4", ".mov" };

// "\xff\xcc\x82\x63\xf8\x55\x4a\x93\x88\x14\x58\x7a\x02\x52\x1f\xdd")
static const uint8_t SPHERICAL_UUID_ID[] = { 0xff, 0xcc, 0x82, 0x63, 0xf8, 0x55, 0x4a, 0x93, 0x88, 0x14, 0x58, 0x7a, 0x02, 0x52, 0x1f, 0xdd };

// XML contents.
static std::string RDF_PREFIX = " xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" ";

static std::string SPHERICAL_XML_HEADER = "<?xml version=\"1.0\"?>"\
"<rdf:SphericalVideo\n"\
"xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"\
"xmlns:GSpherical=\"http://ns.google.com/videos/1.0/spherical/\">";

static std::string SPHERICAL_XML_CONTENTS = "<GSpherical:Spherical>true</GSpherical:Spherical>"\
"<GSpherical:Stitched>true</GSpherical:Stitched>"\
"<GSpherical:StitchingSoftware>%s</GSpherical:StitchingSoftware>";

static std::string SPHERICAL_XML_CONTENTS_EQUIRECT =
"<GSpherical:ProjectionType>equirectangular</GSpherical:ProjectionType>";
static std::string SPHERICAL_XML_CONTENTS_CUBEMAP =
"<GSpherical:ProjectionType>cubemap</GSpherical:ProjectionType>";
static std::string SPHERICAL_XML_CONTENTS_SINGLE_FISHEYE =
"<GSpherical:ProjectionType>single_fisheye</GSpherical:ProjectionType>";

static std::string SPHERICAL_XML_CONTENTS_TOP_BOTTOM = "<GSpherical:StereoMode>top-bottom</GSpherical:StereoMode>";
static std::string SPHERICAL_XML_CONTENTS_LEFT_RIGHT = "<GSpherical:StereoMode>left-right</GSpherical:StereoMode>";

// Parameter order matches that of the crop option.
static std::string SPHERICAL_XML_CONTENTS_CROP_FORMAT = \
"<GSpherical:CroppedAreaImageWidthPixels>%d</GSpherical:CroppedAreaImageWidthPixels>"\
"<GSpherical:CroppedAreaImageHeightPixels>%d</GSpherical:CroppedAreaImageHeightPixels>"\
"<GSpherical:FullPanoWidthPixels>%d</GSpherical:FullPanoWidthPixels>"\
"<GSpherical:FullPanoHeightPixels>%d</GSpherical:FullPanoHeightPixels>"\
"<GSpherical:CroppedAreaLeftPixels>%d</GSpherical:CroppedAreaLeftPixels>"\
"<GSpherical:CroppedAreaTopPixels>%d</GSpherical:CroppedAreaTopPixels>";

static std::string SPHERICAL_XML_FOOTER = "</rdf:SphericalVideo>";
static std::string SPHERICAL_PREFIX = "{http://ns.google.com/videos/1.0/spherical/}";

static AudioMetadata g_DefAudioMetadata;

class Box;
class SA3DBox;
class Container;
class Mpeg4Container;

using namespace FBCapture::Common;

class Metadata {
public:
  Metadata();
  virtual ~Metadata();

  void setVideoXML(std::string &, mxml_node_t *);
  void setVideoXML(std::string &);
  void setAudio(AudioMetadata *);

  std::string &getVideoXML();
  AudioMetadata  *getAudio();

private:
  std::string m_strVideoXML;
  std::map<std::string, mxml_node_t *> m_mapVideo;
  AudioMetadata *m_pAudio;
};


class ParsedMetadata : public Metadata {
public:
  ParsedMetadata();
  virtual ~ParsedMetadata();

  uint32_t m_iNumAudioChannels;

  typedef std::map<std::string, std::string> videoEntry;
  std::map<std::string, videoEntry> m_video;
  SA3DBox *m_pAudio;
};


class Utils {
public:
  Utils();
  virtual ~Utils();

  Box *spherical_uuid(std::string &);
  bool mpeg4_add_spherical(Mpeg4Container *, std::fstream &, std::string &);
  bool mpeg4_add_spatial_audio(Mpeg4Container *, std::fstream &, AudioMetadata *);
  bool mpeg4_add_audio_metadata(Mpeg4Container *, std::fstream &, AudioMetadata *);
  bool inject_spatial_audio_atom(std::fstream &, Box *, AudioMetadata *);
  std::map<std::string, std::string> parse_spherical_xml(uint8_t *); // return sphericalDictionary
  ParsedMetadata *parse_spherical_mpeg4(Mpeg4Container *, std::fstream &); // return metadata
  void parse_mpeg4(std::string &);
  bool inject_mpeg4(std::string &, std::string &, Metadata *);
  void parse_metadata(std::string &);
  bool inject_metadata(std::string &, std::string &, Metadata *);
  std::string &generate_spherical_xml(projection, enMode, std::string, int *);
  uint8_t get_descriptor_length(std::fstream &);
  int32_t get_expected_num_audio_components(std::string &, uint32_t);
  int32_t  get_num_audio_channels(Container *, std::fstream &);
  uint32_t get_sample_description_num_channels(Container *, std::fstream &);
  int32_t  get_aac_num_channels(Container *, std::fstream &);
  uint32_t get_num_audio_tracks(Mpeg4Container *, std::fstream &);

private:
  std::string m_strSphericalXML;
  std::map<std::string, std::string> m_mapSphericalDictionary;
};

#endif // __METADATA_UTILS_H__
