/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2013 Zhaoxiu Zeng <zhaoxiu.zeng@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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

#ifndef __GST_SHINEMP3ENC_H__
#define __GST_SHINEMP3ENC_H__

#include <gst/gst.h>
#include <gst/audio/gstaudioencoder.h>

G_BEGIN_DECLS

#include <shine/layer3.h>

/* #defines don't like whitespacey bits */
#define GST_TYPE_SHINEMP3ENC \
  (gst_shinemp3enc_get_type())
#define GST_SHINEMP3ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SHINEMP3ENC,GstShineMP3Enc))
#define GST_SHINEMP3ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SHINEMP3ENC,GstShineMP3EncClass))
#define GST_IS_SHINEMP3ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SHINEMP3ENC))
#define GST_IS_SHINEMP3ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SHINEMP3ENC))

typedef struct _GstShineMP3Enc      GstShineMP3Enc;
typedef struct _GstShineMP3EncClass GstShineMP3EncClass;

struct _GstShineMP3Enc
{
  GstAudioEncoder element;

  gint samplerate;
  gint num_channels;

  /* properties */
  gint mode;
  gint bitrate;
  gint emphasis;

  shine_config_t  config;
  shine_t         shine;
};

struct _GstShineMP3EncClass 
{
  GstAudioEncoderClass parent_class;
};

GType gst_shinemp3enc_get_type (void);

G_END_DECLS

#endif /* __GST_SHINEMP3ENC_H__ */
