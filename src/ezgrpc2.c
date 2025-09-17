/* ezgrpc2.c - a grpc server without the extra fancy
 * features
 *
 *
 *            _____ _____     ____  ____   ____ _____
 *           | ____|__  /__ _|  _ \|  _ \ / ___|___  \
 *           |  _|   / // _` | |_) | |_) | |     __/  |
 *           | |___ / /| (_| |  _ <|  __/| |___ / ___/
 *           |_____/____\__, |_| \_\_|    \____|______|
 *                      |___/
 *
 *
 * Copyright (c) 2023-2025 M. N. Yoshie & Al-buharie Amjari
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   2. Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "ezgrpc2.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/rand.h>
#include <nghttp2/nghttp2.h>


#include "ezgrpc2_event.h"
#include "ezgrpc2_server_settings.h"
#include "ezgrpc2_server_settings_struct.h"


#include "core.h"

#include "internal_nghttp2_callbacks.h"
#include "internal_helpers.h"


#define logging_fp stderr
// int logging_level;

//#define EZENABLE_IPV6
#define EZWINDOW_SIZE (1<<20)

#define atlog(fmt, ...) ezlog("@ %s: " fmt, __func__, ##__VA_ARGS__)
//#define atlog(fmt, ...) (void)0

//#define EZENABLE_DEBUG
#define ezlogm(...) \
  ezlogf(ezsession, __FILE__, __LINE__, __VA_ARGS__)




/* returns a value where a whole valid message is found
 * f the return value equals vec.len, then the message
 * is complete and not truncated.
 */
size_t count_grpc_message(void *data, size_t len, int *nb_message) {
  i8 *wire = (i8 *)data;
  size_t seek, last_seek = 0;
  for (seek = 0; seek < len; ) {
    /* compressed flag */
    seek += 1;

//    if (seek + 4 > len) {
//      atlog(
//          COLSTR(
//              "(1) prefixed-length messages overflowed. seeked %zu. len %zu\n",
//              BHYEL),
//          seek, len);
//      return last_seek;
//    }
    /* message length */
    u32 msg_len = ntohl(uread_u32(wire + seek));
    /* 4 bytes message lengtb */
    seek += sizeof(u32);

    seek += msg_len;

    if (seek <= len)
      last_seek = seek;
  }

  ezlog("lseek %zu len %zu\n", last_seek, len);

  if (seek > len) {
    atlog(COLSTR("(2) prefixed-length messages overflowed\n", BHYEL));
  }

  if (seek < len) {
    atlog(COLSTR("prefixed-length messages underflowed\n", BHYEL));
  }

  return last_seek;
}



/* clang-format off */













/* THIS IS INTENTIONALLY LEFT BLANK */














/*------------------------------------------------------.
| API FUNCTIONS: You will only have to care about these |
`------------------------------------------------------*/



















EZGRPC2_API int ezgrpc2_session_send(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id,
  ezgrpc2_list_t *lmessages) {
  int res;
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    return 1;
  }
  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, stream_id);
  if (ezstream == NULL) {
    /* stream doesn't exists */
    return 2;
  }

  ezgrpc2_list_concat_and_empty_src(ezstream->lqueue_omessages, lmessages);


  nghttp2_data_provider2 data_provider;
  data_provider.source.ptr = ezstream;
  data_provider.read_callback = data_source_read_callback2;
  nghttp2_submit_data2(ezsession->ngsession, 0, stream_id, &data_provider);

  while (nghttp2_session_want_write(ezsession->ngsession)) {
    res = nghttp2_session_send(ezsession->ngsession);
    if (res) {
      ezlog(COLSTR("nghttp2: %s. killing...\n", BHRED),
            nghttp2_strerror(res));
      return 3;
    }
  }
  return 0;
}











EZGRPC2_API int ezgrpc2_session_end_stream(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id,
  int status) {
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    return 1;
  }

  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, stream_id);
  if (ezstream == NULL)
    return 2;

  char grpc_status[32] = {0};
  int len = snprintf(grpc_status, 31, "%d",(int) status);
  i8 *grpc_message = ezgrpc2_grpc_status_strstatus(status);
  nghttp2_nv trailers[] = {
      {(void *)"grpc-status", (void *)grpc_status, 11, len},
      {(void *)"grpc-message", (void *)grpc_message, 12,
       strlen(grpc_message)},
  };
  if (!ezgrpc2_list_is_empty(ezstream->lqueue_omessages)) {
    atlog(COLSTR("ending stream but messages are still queued\n", BHRED));

  }
  nghttp2_submit_trailer(ezsession->ngsession, stream_id, trailers, 2);
  atlog("trailer submitted\n");
  nghttp2_session_send(ezsession->ngsession);
  return 0;
}










EZGRPC2_API int ezgrpc2_session_end_session(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 last_stream_id,
  int error_code) {
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    /* session doesn't exists */
    return 1;
  }
  nghttp2_submit_goaway(ezsession->ngsession, 0, last_stream_id, error_code, NULL, 0);
  nghttp2_session_send(ezsession->ngsession);

  return 0;
}

EZGRPC2_API int ezgrpc2_session_find_header(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id, ezgrpc2_header_t *ezheader) {
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    return 1;
  }

  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, stream_id);
  if (ezstream == NULL)
    return 2;

  ezgrpc2_header_t *found;
  if ((found = ezgrpc2_list_find(ezstream->lheaders, list_cmp_ezheader_name, ezheader))
      == NULL)
    return 3;

  return ezheader->value = found->value, ezheader->vlen = found->vlen, 0;
}


EZGRPC2_API const char *ezgrpc2_license(void){
  static const char *license = 
    "ezgrpc2 - A grpc server without the extra fancy features.\n"
    "https://github.com/mnyoshie/ezgrpc2\n"
    "\n"
    "Copyright (c) 2023-2025 M. N. Yoshie & Al-buharie Amjari\n"
    "\n"
    "Redistribution and use in source and binary forms, with or without\n"
    "modification, are permitted provided that the following conditions\n"
    "are met:\n"
    "\n"
    "  1. Redistributions of source code must retain the above copyright notice,\n"
    "  this list of conditions and the following disclaimer.\n"
    "\n"
    "  2. Neither the name of the copyright holder nor the names of its\n"
    "  contributors may be used to endorse or promote products derived from\n"
    "  this software without specific prior written permission.\n"
    "\n"
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
    "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
    "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
    "A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n"
    "HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n"
    "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED\n"
    "TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n"
    "PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n"
    "LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
    "NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
    "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
#ifdef _WIN32
    "\n"
    "wepoll - epoll for Windows\n"
    "https://github.com/piscisaureus/wepoll\n"
    "\n"
    "Copyright 2012-2020, Bert Belder <bertbelder@gmail.com>\n"
    "All rights reserved.\n"
    "\n"
    "Redistribution and use in source and binary forms, with or without\n"
    "modification, are permitted provided that the following conditions are\n"
    "met:\n"
    "\n"
    "  * Redistributions of source code must retain the above copyright\n"
    "    notice, this list of conditions and the following disclaimer.\n"
    "\n"
    "  * Redistributions in binary form must reproduce the above copyright\n"
    "    notice, this list of conditions and the following disclaimer in the\n"
    "    documentation and/or other materials provided with the distribution.\n"
    "\n"
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
    "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
    "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
    "A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n"
    "OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n"
    "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n"
    "LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n"
    "DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n"
    "THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
    "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
    "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
#endif
    ;
  return license;
}
