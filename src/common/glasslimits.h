// glasslimits.h
//
// System-wide limits and settings for GlassCoder
//
//   (C) Copyright 2015 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef GLASSLIMITS_H
#define GLASSLIMITS_H

#define DEFAULT_SERVER_PORT 80
#define DEFAULT_AUDIO_BITRATE 128
#define DEFAULT_AUDIO_SAMPLERATE 44100
#define DEFAULT_AUDIO_DEVICE AudioDevice::Jack
#define MAX_AUDIO_CHANNELS 2
#define RINGBUFFER_SIZE 262144

//
// Exit Codes
//
#define GLASS_EXIT_OK 0
#define GLASS_EXIT_UNSUPPORTED_CODEC_ERROR 1  // Unsupported codec
#define GLASS_EXIT_CHANNEL_ERROR 2            // invalid channel remix
#define GLASS_EXIT_DECODER_ERROR 3            // decoder error
#define GLASS_EXIT_FILEOPEN_ERROR 4           // unable to open local file
#define GLASS_EXIT_SERVER_ERROR 5             // malformed response from server
#define GLASS_EXIT_HTTP_ERROR 6               // non-200 http response
#define GLASS_EXIT_SRC_ERROR 7                // samplerate converter error
#define GLASS_EXIT_ARGUMENT_ERROR 8           // invalid command-line invocation
#define GLASS_EXIT_UNSUPPORTED_DEVICE_ERROR 9 // unsupported audio device
#define GLASS_EXIT_GENERAL_DEVICE_ERROR 10    // audio device error
#define GLASS_EXIT_UNSUPPORTED_PLAYLIST_ERROR 11 // unsupported playlist format
#define GLASS_EXIT_INVALID_PLAYLIST_ERROR 12  // invalid playlist format
#define GLASS_EXIT_NETWORK_ERROR 13           // general network error


#endif  // GLASSLIMITS_H
