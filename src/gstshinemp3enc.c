/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2013 Zhaoxiu Zeng <zhaoxiu.zeng@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstshinemp3enc.h"

GST_DEBUG_CATEGORY_STATIC (gst_shinemp3enc_debug);
#define GST_CAT_DEFAULT gst_shinemp3enc_debug

/* elementfactory information */

static GstStaticPadTemplate gst_shinemp3enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) " G_STRINGIFY (G_BYTE_ORDER) ", "
        "signed = (boolean) true, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 }, "
        "channels = (int) [ 1, 2 ]")
    );

static GstStaticPadTemplate gst_shinemp3enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, "
        "mpegversion = (int) 1, "
        "layer = (int) 3, "
        "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 }, "
        "channels = (int) [ 1, 2 ]")
    );


/********** Define useful types for non-programmatic interfaces **********/
#define GST_TYPE_SHINEMP3ENC_MODE (gst_shinemp3enc_mode_get_type())
static GType
gst_shinemp3enc_mode_get_type (void)
{
  static GType shinemp3enc_mode_get_type = 0;
  static GEnumValue shinemp3enc_modes[] = {
    {STEREO, "Stereo", "stereo"},
    {JOINT_STEREO, "Joint Stereo", "joint"},
    {DUAL_CHANNEL, "Dual Channel", "dual"},
    {MONO, "Mono", "mono"},
    {0, NULL, NULL}
  };

  if (!shinemp3enc_mode_get_type) {
    shinemp3enc_mode_get_type =
        g_enum_register_static ("GstShineMP3EncMode", shinemp3enc_modes);
  }
  return shinemp3enc_mode_get_type;
}

#define GST_TYPE_SHINEMP3ENC_EMPHASIS (gst_shinemp3enc_emphasis_get_type())
static GType
gst_shinemp3enc_emphasis_get_type (void)
{
  static GType shinemp3enc_emphasis_type = 0;
  static GEnumValue shinemp3enc_emphasis[] = {
    {NONE, "No emphasis", "none"},
    {MU50_15, "50/15 ms", "5"},
    {CITT, "CCIT J.17", "ccit"},
    {0, NULL, NULL}
  };

  if (!shinemp3enc_emphasis_type) {
    shinemp3enc_emphasis_type =
        g_enum_register_static ("GstShineMP3EncEmphasis", shinemp3enc_emphasis);
  }

  return shinemp3enc_emphasis_type;
}

/********** Standard stuff for signals and arguments **********/

enum
{
  ARG_0,
  ARG_MODE,
  ARG_BITRATE,
  ARG_EMPHASIS,
};

#define DEFAULT_MODE STEREO
#define DEFAULT_BITRATE 128
#define DEFAULT_EMPHASIS NONE

static gboolean gst_shinemp3enc_start (GstAudioEncoder * enc);
static gboolean gst_shinemp3enc_stop (GstAudioEncoder * enc);
static gboolean gst_shinemp3enc_set_format (GstAudioEncoder * enc,
    GstAudioInfo * info);
static GstFlowReturn gst_shinemp3enc_handle_frame (GstAudioEncoder * enc,
    GstBuffer * in_buf);
static void gst_shinemp3enc_flush (GstAudioEncoder * enc);

static void gst_shinemp3enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_shinemp3enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_shinemp3enc_setup (GstShineMP3Enc* shine);

GST_BOILERPLATE (GstShineMP3Enc, gst_shinemp3enc, GstAudioEncoder,
    GST_TYPE_AUDIO_ENCODER);

static void
gst_shinemp3enc_release_memory (GstShineMP3Enc* shine)
{
  if (shine->shine) {
    shine_close (shine->shine);
    shine->shine = NULL;
  }
}

static void
gst_shinemp3enc_finalize (GObject * obj)
{
  gst_shinemp3enc_release_memory (GST_SHINEMP3ENC(obj));

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_shinemp3enc_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class,
      &gst_shinemp3enc_src_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_shinemp3enc_sink_template);
  gst_element_class_set_details_simple(element_class,
      "shine mp3 encoder",
      "Codec/Encoder/Audio",
      "fixed point free MP3 encoder",
      "Zhaoxiu Zeng <zhaoxiu.zeng@gmail.com>");
}

static void
gst_shinemp3enc_class_init (GstShineMP3EncClass * klass)
{
  GObjectClass *gobject_class;
  GstAudioEncoderClass *base_class;

  gobject_class = (GObjectClass *) klass;
  base_class = (GstAudioEncoderClass *) klass;

  gobject_class->set_property = gst_shinemp3enc_set_property;
  gobject_class->get_property = gst_shinemp3enc_get_property;
  gobject_class->finalize = gst_shinemp3enc_finalize;

  base_class->start = GST_DEBUG_FUNCPTR (gst_shinemp3enc_start);
  base_class->stop = GST_DEBUG_FUNCPTR (gst_shinemp3enc_stop);
  base_class->set_format = GST_DEBUG_FUNCPTR (gst_shinemp3enc_set_format);
  base_class->handle_frame = GST_DEBUG_FUNCPTR (gst_shinemp3enc_handle_frame);
  base_class->flush = GST_DEBUG_FUNCPTR (gst_shinemp3enc_flush);

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_MODE,
      g_param_spec_enum ("mode", "Mode", "Encoding mode",
          GST_TYPE_SHINEMP3ENC_MODE, DEFAULT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_BITRATE,
      g_param_spec_int ("bitrate", "Bitrate (kb/s)",
          "Bitrate in kbit/sec (Only valid if target is bitrate, for CBR one "
          "of 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, "
          "256 or 320)", 8, 320, DEFAULT_BITRATE,
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_EMPHASIS,
      g_param_spec_enum ("emphasis", "Emphasis",
          "Pre-emphasis to apply to the decoded audio",
          GST_TYPE_SHINEMP3ENC_EMPHASIS, DEFAULT_EMPHASIS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_shinemp3enc_init (GstShineMP3Enc * shine, GstShineMP3EncClass * klass)
{
}

static gboolean
gst_shinemp3enc_start (GstAudioEncoder * enc)
{
  GstShineMP3Enc *shine = GST_SHINEMP3ENC (enc);

  GST_DEBUG_OBJECT (shine, "start");

  return TRUE;
}

static gboolean
gst_shinemp3enc_stop (GstAudioEncoder * enc)
{
  GstShineMP3Enc *shine = GST_SHINEMP3ENC (enc);

  GST_DEBUG_OBJECT (shine, "stop");

  gst_shinemp3enc_release_memory (shine);

  return TRUE;
}

static gboolean
gst_shinemp3enc_set_format (GstAudioEncoder * enc, GstAudioInfo * info)
{
  GstShineMP3Enc *shine = GST_SHINEMP3ENC (enc);
  GstCaps *othercaps;

  /* parameters already parsed for us */
  shine->samplerate = GST_AUDIO_INFO_RATE (info);
  shine->num_channels = GST_AUDIO_INFO_CHANNELS (info);

  /* but we might be asked to reconfigure, so reset */
  gst_shinemp3enc_release_memory (shine);

  GST_DEBUG_OBJECT (shine, "setting up shine");
  if (!gst_shinemp3enc_setup (shine))
    goto setup_failed;

  othercaps = gst_caps_new_simple ("audio/mpeg",
      "mpegversion", G_TYPE_INT, 1,
      "mpegaudioversion", G_TYPE_INT, shine_mpeg_version(shine_find_samplerate_index(shine->samplerate)),
      "layer", G_TYPE_INT, 3,
      "channels", G_TYPE_INT, shine->mode == MONO ? 1 : shine->num_channels,
      "rate", G_TYPE_INT, shine->samplerate,
      NULL);

  /* and use these caps */
  gst_pad_set_caps (GST_AUDIO_ENCODER_SRC_PAD (enc), othercaps);
  gst_caps_unref (othercaps);

  /* report needs to base class:
   * hand one frame at a time, if we are pretty sure what a frame is */
  gst_audio_encoder_set_frame_samples_min (enc, shine_samples_per_pass(shine->shine));
  gst_audio_encoder_set_frame_samples_max (enc, shine_samples_per_pass(shine->shine));

  return TRUE;

setup_failed:
  GST_ELEMENT_ERROR (shine, LIBRARY, SETTINGS,
      ("Failed to configure shine encoder. Check your encoding parameters."), (NULL));
  return FALSE;
}

static void
gst_shinemp3enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstShineMP3Enc *shine = GST_SHINEMP3ENC (object);

  switch (prop_id) {
    case ARG_MODE:
      shine->mode = g_value_get_enum (value);
      break;
    case ARG_BITRATE:
      shine->bitrate = g_value_get_int (value);
      break;
    case ARG_EMPHASIS:
      shine->emphasis = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_shinemp3enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstShineMP3Enc *shine = GST_SHINEMP3ENC (object);

  switch (prop_id) {
    case ARG_MODE:
      g_value_set_enum (value, shine->mode);
      break;
    case ARG_BITRATE:
      g_value_set_int (value, shine->bitrate);
      break;
    case ARG_EMPHASIS:
      g_value_set_enum (value, shine->emphasis);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_shinemp3enc_flush_full (GstShineMP3Enc * shine, gboolean push)
{
  unsigned char *mp3_buf;
  long mp3_size;
  GstFlowReturn result = GST_FLOW_OK;

  if (!shine->shine)
    return GST_FLOW_OK;

  mp3_buf = shine_flush (shine->shine, &mp3_size);
  if (mp3_size > 0 && push) {
  	/* allocate output buffer */
    GstBuffer *out_buf = gst_buffer_new_and_alloc (mp3_size);
    memcpy(GST_BUFFER_DATA(out_buf), mp3_buf, mp3_size);

    GST_DEBUG_OBJECT (shine, "collecting final %d bytes", mp3_size);

    result = gst_audio_encoder_finish_frame (GST_AUDIO_ENCODER (shine), out_buf, -1);
  } else {
    GST_DEBUG_OBJECT (shine, "no final packet (size=%d, push=%d)", mp3_size, push);
  }

  return result;
}

static void
gst_shinemp3enc_flush (GstAudioEncoder * enc)
{
  gst_shinemp3enc_flush_full (GST_SHINEMP3ENC (enc), FALSE);
}

static GstFlowReturn
gst_shinemp3enc_handle_frame (GstAudioEncoder * enc, GstBuffer * in_buf)
{
  GstShineMP3Enc *shine = GST_SHINEMP3ENC (enc);
  guint16 *data;
  guint size;
  gint num_samples;
  guint samples_per_pass;
  GstFlowReturn result = GST_FLOW_OK;
  guint total_mp3_size = 0;

  /* squeeze remaining and push */
  if (G_UNLIKELY (in_buf == NULL))
    return gst_shinemp3enc_flush_full (shine, TRUE);

  data = GST_BUFFER_DATA (in_buf);
  size = GST_BUFFER_SIZE (in_buf);

  num_samples = size / (2 * shine->num_channels);
  samples_per_pass = shine_samples_per_pass(shine->shine);

  while (num_samples >= samples_per_pass) {
    unsigned char *mp3_buf;
    long mp3_size;

    mp3_buf = shine_encode_buffer_interleaved(shine->shine, data, &mp3_size);
    if (G_LIKELY (mp3_size > 0)) {
      /* allocate output buffer */
      GstBuffer *out_buf = gst_buffer_new_and_alloc (mp3_size);
      memcpy(GST_BUFFER_DATA(out_buf), mp3_buf, mp3_size);

      result = gst_audio_encoder_finish_frame (GST_AUDIO_ENCODER (shine), out_buf, -1);
      if (result != GST_FLOW_OK)
        break;
      total_mp3_size += mp3_size;
    }

    data += samples_per_pass * shine->num_channels;
    num_samples -= samples_per_pass;
  }

  GST_LOG_OBJECT (shine, "encoded %d bytes of audio to %d bytes of mp3",
      size, total_mp3_size);

  return result;
}

/* set up the encoder state */
static gboolean
gst_shinemp3enc_setup (GstShineMP3Enc* shine)
{
  GST_DEBUG_OBJECT (shine, "starting setup");

  /* See if samplerate and bitrate are valid */
  if (shine_check_config(shine->samplerate, shine->bitrate) < 0) {
    GST_ELEMENT_ERROR (shine, LIBRARY, SETTINGS,
        ("Failed to configure SHINEMP3ENC encoder. Unsupported samplerate/bitrate configuration."), (NULL));
    return FALSE;
  }

  if (shine->num_channels <= 0 || shine->num_channels > 2) {
    GST_ELEMENT_ERROR (shine, LIBRARY, SETTINGS,
        ("Failed to configure SHINEMP3ENC encoder. Only mono or stereo is supported."), (NULL));
    return FALSE;
  }

  /* force mono encoding if we only have one channel */
  if (shine->num_channels == 1)
    shine->mode = MONO;

  shine_set_config_mpeg_defaults(&shine->config.mpeg);
  shine->config.mpeg.bitr       = shine->bitrate;
  shine->config.mpeg.mode       = shine->mode;
  shine->config.mpeg.emph       = shine->emphasis;

  shine->config.wave.samplerate = shine->samplerate;
  shine->config.wave.channels   = shine->num_channels;

  /* initialize the shine encoder */
  shine->shine = shine_initialise(&shine->config);
  if (shine->shine != NULL) {
    /* FIXME: it would be nice to print out the mode here */
    GST_INFO ("shine encoder setup (%d kbit/s, %d Hz, %d channels)",
        shine->bitrate, shine->samplerate, shine->num_channels);
  } else {
    GST_ERROR_OBJECT (shine, "shine_initialise failed");
  }

  GST_DEBUG_OBJECT (shine, "done with setup");

  return shine->shine != NULL;
}

static gboolean
shinemp3enc_init (GstPlugin * shinemp3enc)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template shinemp3enc' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_shinemp3enc_debug, "shinemp3enc",
      0, "shine mp3 encoder");

#ifdef ENABLE_NLS
  GST_DEBUG ("binding text domain %s to locale dir %s", GETTEXT_PACKAGE,
      LOCALEDIR);
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

  return gst_element_register (shinemp3enc, "shinemp3enc", GST_RANK_PRIMARY,
      GST_TYPE_SHINEMP3ENC);
}

/* gstreamer looks for this structure to register shinemp3encs
 *
 * exchange the string 'Template shinemp3enc' with your shinemp3enc description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "shine",
    "Encode MP3s with libshine",
    shinemp3enc_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
