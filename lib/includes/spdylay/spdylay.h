/*
 * Spdylay - SPDY Library
 *
 * Copyright (c) 2012 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef SPDYLAY_H
#define SPDYLAY_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

struct spdylay_session;
typedef struct spdylay_session spdylay_session;

typedef enum {
  SPDYLAY_ERR_INVALID_ARGUMENT = -501,
  SPDYLAY_ERR_ZLIB = -502,
  SPDYLAY_ERR_ZLIB_BUF = -503,
  SPDYLAY_ERR_WOULDBLOCK = -504,
  SPDYLAY_ERR_PROTO = -505,
  SPDYLAY_ERR_INVALID_FRAME = -506,

  /* The errors < SPDYLAY_ERR_FATAL mean that the library is under
     unexpected condition that it cannot process any further data
     reliably (e.g., out of memory). */
  SPDYLAY_ERR_FATAL = -900,
  SPDYLAY_ERR_NOMEM = -901,
  SPDYLAY_ERR_CALLBACK_FAILURE = -902,
} spdylay_error;

typedef enum {
  SPDYLAY_MSG_MORE
} spdylay_io_flag;

typedef enum {
  SPDYLAY_SYN_STREAM = 1,
  SPDYLAY_SYN_REPLY = 2,
  SPDYLAY_RST_STREAM = 3,
  SPDYLAY_SETTINGS = 4,
  SPDYLAY_NOOP = 5,
  SPDYLAY_PING = 6,
  SPDYLAY_GOAWAY = 7,
  SPDYLAY_DATA = 100,
} spdylay_frame_type;

typedef enum {
  SPDYLAY_FLAG_NONE = 0,
  SPDYLAY_FLAG_FIN = 1,
  SPDYLAY_FLAG_UNIDIRECTIONAL = 2
} spdylay_flag;

typedef enum {
  SPDYLAY_PROTOCOL_ERROR = 1,
  SPDYLAY_INVALID_STREAM = 2,
  SPDYLAY_REFUSED_STREAM = 3,
  SPDYLAY_UNSUPPORTED_VERSION = 4,
  SPDYLAY_CANCEL = 5,
  SPDYLAY_INTERNAL_ERROR = 6,
  SPDYLAY_FLOW_CONTROL_ERROR = 7
} spdylay_status_code;

typedef struct {
  uint16_t version;
  uint16_t type;
  uint8_t flags;
  int32_t length;
} spdylay_ctrl_hd;

typedef struct {
  spdylay_ctrl_hd hd;
  int32_t stream_id;
  int32_t assoc_stream_id;
  /* 0 (Highest) to 3 (Lowest). Looks like spdy/2 spec is wrong. */
  uint8_t pri;
  char **nv;
} spdylay_syn_stream;

typedef struct {
  spdylay_ctrl_hd hd;
  int32_t stream_id;
  char **nv;
} spdylay_syn_reply;

typedef struct {
  spdylay_ctrl_hd hd;
  int32_t stream_id;
  uint32_t status_code;
} spdylay_rst_stream;

typedef union {
  int fd;
  void *ptr;
} spdylay_data_source;

typedef ssize_t (*spdylay_data_source_read_callback)
(spdylay_session *session, uint8_t *buf, size_t length, int *eof,
 spdylay_data_source *source, void *user_data);

typedef struct {
  spdylay_data_source source;
  spdylay_data_source_read_callback read_callback;
} spdylay_data_provider;

typedef struct {
  int32_t stream_id;
  uint8_t flags;
  spdylay_data_provider data_prd;
} spdylay_data;

typedef union {
  spdylay_syn_stream syn_stream;
  spdylay_syn_reply syn_reply;
  spdylay_rst_stream rst_stream;
  spdylay_data data;
} spdylay_frame;

typedef ssize_t (*spdylay_send_callback)
(spdylay_session *session,
 const uint8_t *data, size_t length, int flags, void *user_data);

typedef ssize_t (*spdylay_recv_callback)
(spdylay_session *session,
 uint8_t *buf, size_t length, int flags, void *user_data);

/*
 * Callback function invoked by spdylay_session_recv() when a control
 * frame is arrived.
 */
typedef void (*spdylay_on_ctrl_recv_callback)
(spdylay_session *session, spdylay_frame_type type, spdylay_frame *frame,
 void *user_data);

/*
 * Callback function invoked by spdylay_session_recv() when an invalid
 * control frame is arrived, which typically the case where RST_STREAM
 * will be sent
 */
typedef void (*spdylay_on_invalid_ctrl_recv_callback)
(spdylay_session *session, spdylay_frame_type type, spdylay_frame *frame,
 void *user_data);

typedef struct {
  spdylay_send_callback send_callback;
  spdylay_recv_callback recv_callback;
  spdylay_on_ctrl_recv_callback on_ctrl_recv_callback;
  spdylay_on_invalid_ctrl_recv_callback on_invalid_ctrl_recv_callback;
} spdylay_session_callbacks;

int spdylay_session_client_new(spdylay_session **session_ptr,
                               const spdylay_session_callbacks *callbacks,
                               void *user_data);

void spdylay_session_del(spdylay_session *session);

int spdylay_session_send(spdylay_session *session);

int spdylay_session_recv(spdylay_session *session);

int spdylay_session_want_read(spdylay_session *session);

int spdylay_session_want_write(spdylay_session *session);

int spdylay_req_submit(spdylay_session *session, const char *path);

/*
 * Submits SYN_REPLY frame against stream |stream_id|. |nv| must
 * include "status" and "version" key. "status" must be status code
 * (e.g., "200" or "200 OK"). "version" is HTTP response version
 * (e.g., "HTTP/1.1"). This function creates copies of all name/value
 * pairs in |nv|. If |data_prd| is not NULL, it provides data which
 * will be sent in subsequent DATA frames. If |data_prd| is NULL,
 * SYN_REPLY will have FLAG_FIN.
 */
int spdylay_reply_submit(spdylay_session *session,
                         int32_t stream_id, const char **nv,
                         spdylay_data_provider *data_prd);

#ifdef __cplusplus
}
#endif

#endif /* SPDYLAY_H */
