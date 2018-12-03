#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/un.h>
#include <signal.h>
#include "sha1.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>

#include "bts-wscli.h"

int base64_encode(unsigned char *source, size_t sourcelen, char *target, size_t targetlen);
bool wsclient_handshake(wsclient *client);
int wsclient_open_connection(const char *host, const char *port);
wsclient_message* wsclient_in_data(wsclient *c, char in, int* errcode);
wsclient_message* wsclient_dispatch_message(wsclient *c, wsclient_frame *current, int *errcode);
wsclient_error *ws_error(int errcode);
int stricmp(const char *s1, const char *s2);

int _wsclient_read(wsclient *c, void *buf, size_t length);
int _wsclient_write(wsclient *c, const void *buf, size_t length);
int wsclient_complete_frame(wsclient *c, wsclient_frame *frame);
void wsclient_handle_control_frame(wsclient *c, wsclient_frame *ctl_frame);
void wsclient_cleanup_frames(wsclient_frame *first);

//somewhat hackish stricmp
int stricmp(const char *s1, const char *s2) {
   register unsigned char c1, c2;
   register unsigned char flipbit = ~(1 << 5);
   do {
      c1 = (unsigned char)*s1++ & flipbit;
      c2 = (unsigned char)*s2++ & flipbit;
      if (c1 == '\0')
         return c1 - c2;
   } while (c1 == c2);
   return c1 - c2;
}

char* msg_login = "[1,\"login\",[\"\",\"\"]]";
char* msg_database = "[1,\"database\",[]]";
char* msg_network_broadcast = "[1,\"network_broadcast\",[]]";
char* msg_history = "[1,\"history\",[]]";
char* msg_get_chain_id = "[2, \"get_chain_id\", []]";
char* msg_set_callback = "[2,\"set_block_applied_callback\",[0]]"; //5
char* msg_get_global_properties = "[2, \"get_global_properties\", []]"; //6
char* msg_get_dynamic_global_properties = "[2, \"get_dynamic_global_properties\", []]"; //7
char* msg_lookup_account_names = "[2, \"lookup_account_names\", [[\"nathan\"]]]"; //8

int onclose(wsclient *c) {
   fprintf(stdout, "onclose called: %d\n", c->sockfd);
   return 0;
}

int onerror(wsclient *c, wsclient_error *err) {
   fprintf(stdout, "onerror: (%d): %s\n", err->code, err->str);
   if (err->extra_code) {
      errno = err->extra_code;
      perror("recv");
   }
   return 0;
}

int onmessage(wsclient *c, wsclient_message *msg) {
   fprintf(stdout, "onmessage: (len=%llu): %s\n\n", msg->payload_len, msg->payload);
   return 0;
}

int onopen(wsclient *c) {
   fprintf(stdout, "onopen called: %d\n", c->sockfd);
   // wsclient_send(c, "Hello onopen \r\n");
   return 0;
}

void sigint_handler(int sig)
{
   static int interrupted = 0;
   printf("SIGINT caught!!\n");
   if (++interrupted > 1)
      exit(1);
}

void login(wsclient *client)
{
   wsclient_message* msg = NULL;
   msg = call(client, msg_login);
   if (msg)
   {
      free_message(msg);
   }
}

void init_api(wsclient *client)
{
   wsclient_message* msg = NULL;
   msg = call(client, msg_database);
   if (msg) 
      free_message(msg); 

   msg = call(client, msg_network_broadcast);
   if (msg)
      free_message(msg);

   msg = call(client, msg_history);
   if (msg)
      free_message(msg);

   msg = call(client, msg_get_chain_id);
   if (msg)
      free_message(msg);

   msg = call(client, msg_get_global_properties);
   if (msg)
      free_message(msg);

   msg = call(client, msg_get_dynamic_global_properties);
   if (msg)
      free_message(msg);

   msg = call(client, msg_lookup_account_names);
   if (msg)
      free_message(msg);
}

const char* get_dynamic_global_properties(wsclient *client)
{
   wsclient_message* msg = call(client, msg_get_dynamic_global_properties);
   if (msg)
   {
      char *ret = malloc(msg->payload_len + 1);
      memcpy(ret, msg->payload, msg->payload_len + 1);
      free_message(msg);
      return ret;
   }
}

const char* get_global_properties(wsclient *client)
{
   wsclient_message* msg = call(client, msg_get_global_properties);
   if (msg)
   {
      char *ret = malloc(msg->payload_len + 1);
      memcpy(ret, msg->payload, msg->payload_len + 1);
      free_message(msg);
      return ret;
   }
}

int main(int argc, char **argv) {

   signal(SIGINT, sigint_handler);

   //Initialize new wsclient* using specified URI
   const char* wsUrl = argc > 1 ? argv[1] : "ws://10.7.0.216:11011";
   wsclient *client = wsclient_new(wsUrl); // echo.websocket.org");
   if (!client) {
      fprintf(stderr, "Unable to initialize new WS client.\n");
      exit(1);
   }
   //set callback functions for this client
   wsclient_onopen(client, &onopen);
   wsclient_onmessage(client, &onmessage);
   wsclient_onerror(client, &onerror);
   wsclient_onclose(client, &onclose);

   if (!wsclient_handshake(client))
      return 1;

   wsclient_message* msg = NULL;
   login(client);

   init_api(client);

   for (;;)
   {
      get_dynamic_global_properties(client);
      get_global_properties(client);
      sleep(5);
   }

   printf("Disconnect to the server.\n");
   wsclient_close(client);

   return 0;
}

wsclient_message* wsclient_recv(wsclient *client)
{
   if (!client->sockfd)
      return NULL;

   wsclient_error *err = NULL;
   int sockfd;
   char buf[1024];
   int n, i, errcode = 0;
   wsclient_message* msg;

   do {
      memset(buf, 0, 1024);
      n = _wsclient_read(client, buf, 1023);
      for (i = 0; i < n; i++)
      {
         msg = wsclient_in_data(client, buf[i], &errcode);
         if (msg != NULL)
            return msg;
         if (errcode != 0)
            return NULL;
      }

   } while (n > 0);

   if (n < 0) {
      if (client->onerror) {
         err = ws_error(WS_RUN_THREAD_RECV_ERR);
         err->extra_code = n;
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
   }

   if (client->onclose) {
      client->onclose(client);
   }
   close(client->sockfd);
   free(client);

   return NULL;
}

wsclient_message* call(wsclient *client, char* parms)
{
   char request[5120];
   sprintf(request, "{\"id\":%d,\"method\":\"call\",\"params\":%s}", ++client->seq, parms);
   fprintf(stdout, "request[%d]: %s\n", client->seq, request);

   wsclient_send(client, request);
   wsclient_message* msg = wsclient_recv(client);

   if(msg!=NULL)
   {
      printf("return: (len=%llu): %s\n\n", msg->payload_len, msg->payload);
   }
   return msg;
}

wsclient *wsclient_new(const char *URI) {
   wsclient *client = NULL;

   client = (wsclient *)malloc(sizeof(wsclient));
   if (!client) {
      fprintf(stderr, "Unable to allocate memory in wsclient_new.\n");
      exit(WS_EXIT_MALLOC);
   }

   memset(client, 0, sizeof(wsclient));
   client->URI = (char *)malloc(strlen(URI) + 1);
   if (!client->URI) {
      fprintf(stderr, "Unable to allocate memory in wsclient_new.\n");
      exit(WS_EXIT_MALLOC);
   }
   memset(client->URI, 0, strlen(URI) + 1);
   strncpy(client->URI, URI, strlen(URI));
   client->flags |= CLIENT_CONNECTING;

   return client;
}


bool wsclient_handshake(wsclient *client)
{
   wsclient_error *err = NULL;
   const char *URI = client->URI;
   SHA1Context shactx;
   const char *UUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
   char pre_encode[256];
   char sha1bytes[20];
   char expected_base64[512];
   char request_headers[1024];
   char websocket_key[256];
   unsigned char key_nonce[16];
   char scheme[10];
   char host[255];
   char request_host[255];
   char port[10];
   char path[255];
   char recv_buf[1024];
   char *URI_copy = NULL, *p = NULL, *rcv = NULL, *tok = NULL;
   int i, z, sockfd, n, flags = 0, headers_space = 1024;
   URI_copy = (char *)malloc(strlen(URI) + 1);
   if (!URI_copy) {
      fprintf(stderr, "Unable to allocate memory in wsclient_handshake.\n");
      exit(WS_EXIT_MALLOC);
   }
   memset(URI_copy, 0, strlen(URI) + 1);
   strncpy(URI_copy, URI, strlen(URI));
   p = strstr(URI_copy, "://");
   if (p == NULL) {
      fprintf(stderr, "Malformed or missing scheme for URI.\n");
      exit(WS_EXIT_BAD_SCHEME);
   }
   strncpy(scheme, URI_copy, p - URI_copy);
   scheme[p - URI_copy] = '\0';
   if (strcmp(scheme, "ws") != 0 && strcmp(scheme, "wss") != 0) {
      fprintf(stderr, "Invalid scheme for URI: %s\n", scheme);
      exit(WS_EXIT_BAD_SCHEME);
   }
   if (strcmp(scheme, "ws") == 0) {
      strncpy(port, "80", 9);
   }
   else {
      strncpy(port, "443", 9); 
      client->flags |= CLIENT_IS_SSL; 
   }
   for (i = p - URI_copy + 3, z = 0; *(URI_copy + i) != '/' && *(URI_copy + i) != ':' && *(URI_copy + i) != '\0'; i++, z++) {
      host[z] = *(URI_copy + i);
   }
   host[z] = '\0';
   if (*(URI_copy + i) == ':') {
      i++;
      p = strchr(URI_copy + i, '/');
      if (!p)
         p = strchr(URI_copy + i, '\0');
      strncpy(port, URI_copy + i, (p - (URI_copy + i)));
      port[p - (URI_copy + i)] = '\0';
      i += p - (URI_copy + i);
   }
   if (*(URI_copy + i) == '\0') {
      //end of URI request path will be /
      strncpy(path, "/", 1);
   }
   else {
      strncpy(path, URI_copy + i, 254);
   }
   free(URI_copy);
   sockfd = wsclient_open_connection(host, port);


   if (sockfd < 0) {
      if (client->onerror) {
         err = ws_error(sockfd);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return false;
   }

   client->sockfd = sockfd; 

   //perform handshake
   //generate nonce
   srand(time(NULL));
   for (z = 0; z<16; z++) {
      key_nonce[z] = rand() & 0xff;
   }
   base64_encode(key_nonce, 16, websocket_key, 256);
   memset(request_headers, 0, 1024);

   if (strcmp(port, "80") != 0) {
      snprintf(request_host, 255, "%s:%s", host, port);
   }
   else {
      snprintf(request_host, 255, "%s", host);
   }
   snprintf(request_headers, 1024, "GET %s HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nHost: %s\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n", path, request_host, websocket_key);
   n = _wsclient_write(client, request_headers, strlen(request_headers));
   z = 0;
   memset(recv_buf, 0, 1024);
   //TODO: actually handle data after \r\n\r\n in case server
   // sends post-handshake data that gets coalesced in this recv
   do {
      n = _wsclient_read(client, recv_buf + z, 1023 - z);
      z += n;
   } while ((z < 4 || strstr(recv_buf, "\r\n\r\n") == NULL) && n > 0);

   if (n == 0) {
      if (client->onerror) {
         err = ws_error(WS_HANDSHAKE_REMOTE_CLOSED_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return false;
   }
   if (n < 0) {
      if (client->onerror) {
         err = ws_error(WS_HANDSHAKE_RECV_ERR);
         err->extra_code = n;
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return false;
   }
   //parse recv_buf for response headers and assure Accept matches expected value
   rcv = (char *)malloc(strlen(recv_buf) + 1);
   if (!rcv) {
      fprintf(stderr, "Unable to allocate memory in wsclient_handshake.\n");
      exit(WS_EXIT_MALLOC);
   }
   memset(rcv, 0, strlen(recv_buf) + 1);
   strncpy(rcv, recv_buf, strlen(recv_buf));
   memset(pre_encode, 0, 256);
   snprintf((char*)pre_encode, 256, "%s%s", websocket_key, UUID);
   SHA1Reset(&shactx);
   SHA1Input(&shactx, (unsigned char*)pre_encode, strlen(pre_encode));
   SHA1Result(&shactx);
   memset(pre_encode, 0, 256);
   snprintf(pre_encode, 256, "%08x%08x%08x%08x%08x", shactx.Message_Digest[0], shactx.Message_Digest[1], shactx.Message_Digest[2], shactx.Message_Digest[3], shactx.Message_Digest[4]);
   for (z = 0; z < (strlen(pre_encode) / 2); z++)
      sscanf(pre_encode + (z * 2), "%02hhx", sha1bytes + z);
   memset(expected_base64, 0, 512);
   base64_encode((unsigned char*)sha1bytes, 20, expected_base64, 512);
   for (tok = strtok(rcv, "\r\n"); tok != NULL; tok = strtok(NULL, "\r\n")) {
      if (*tok == 'H' && *(tok + 1) == 'T' && *(tok + 2) == 'T' && *(tok + 3) == 'P') {
         p = strchr(tok, ' ');
         p = strchr(p + 1, ' ');
         *p = '\0';
         if (strcmp(tok, "HTTP/1.1 101") != 0 && strcmp(tok, "HTTP/1.0 101") != 0) {
            if (client->onerror) {
	       printf("%s\n", tok);
               err = ws_error(WS_HANDSHAKE_BAD_STATUS_ERR);
               client->onerror(client, err);
               free(err);
               err = NULL;
            }
            return false;
         }
         flags |= REQUEST_VALID_STATUS;
      }
      else {
         p = strchr(tok, ' ');
         *p = '\0';
         if (strcmp(tok, "Upgrade:") == 0) {
            if (stricmp(p + 1, "websocket") == 0) {
               flags |= REQUEST_HAS_UPGRADE;
            }
         }
         if (strcmp(tok, "Connection:") == 0) {
            if (stricmp(p + 1, "upgrade") == 0) {
               flags |= REQUEST_HAS_CONNECTION;
            }
         }
         if (strcmp(tok, "Sec-WebSocket-Accept:") == 0) {
            if (strcmp(p + 1, expected_base64) == 0) {
               flags |= REQUEST_VALID_ACCEPT;
            }
         }
      }
   }
   if (!flags & REQUEST_HAS_UPGRADE) {
      if (client->onerror) {
         err = ws_error(WS_HANDSHAKE_NO_UPGRADE_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return false;
   }
   if (!flags & REQUEST_HAS_CONNECTION) {
      if (client->onerror) {
         err = ws_error(WS_HANDSHAKE_NO_CONNECTION_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return false;
   }
   if (!flags & REQUEST_VALID_ACCEPT) {
      if (client->onerror) {
         err = ws_error(WS_HANDSHAKE_BAD_ACCEPT_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return false;
   }  
   client->flags &= ~CLIENT_CONNECTING; 
   if (client->onopen != NULL) {
      client->onopen(client);
   }

   return true;
}


wsclient_error *ws_error(int errcode) {
   wsclient_error *err = NULL;
   err = (wsclient_error *)malloc(sizeof(wsclient_error));
   if (!err) {
      //one of the few places we will fail and exit
      fprintf(stderr, "Unable to allocate memory in ws_error.\n");
      exit(errcode);
   }
   memset(err, 0, sizeof(wsclient_error));
   err->code = errcode;
   switch (err->code) {
   case WS_OPEN_CONNECTION_ADDRINFO_ERR:
      err->str = *(errors + 1);
      break;
   case WS_OPEN_CONNECTION_ADDRINFO_EXHAUSTED_ERR:
      err->str = *(errors + 2);
      break;
   case WS_RUN_THREAD_RECV_ERR:
      err->str = *(errors + 3);
      break;
   case WS_DO_CLOSE_SEND_ERR:
      err->str = *(errors + 4);
      break;
   case WS_HANDLE_CTL_FRAME_SEND_ERR:
      err->str = *(errors + 5);
      break;
   case WS_COMPLETE_FRAME_MASKED_ERR:
      err->str = *(errors + 6);
      break;
   case WS_DISPATCH_MESSAGE_NULL_PTR_ERR:
      err->str = *(errors + 7);
      break;
   case WS_SEND_AFTER_CLOSE_FRAME_ERR:
      err->str = *(errors + 8);
      break;
   case WS_SEND_DURING_CONNECT_ERR:
      err->str = *(errors + 9);
      break;
   case WS_SEND_NULL_DATA_ERR:
      err->str = *(errors + 10);
      break;
   case WS_SEND_DATA_TOO_LARGE_ERR:
      err->str = *(errors + 11);
      break;
   case WS_SEND_SEND_ERR:
      err->str = *(errors + 12);
      break;
   case WS_HANDSHAKE_REMOTE_CLOSED_ERR:
      err->str = *(errors + 13);
      break;
   case WS_HANDSHAKE_RECV_ERR:
      err->str = *(errors + 14);
      break;
   case WS_HANDSHAKE_BAD_STATUS_ERR:
      err->str = *(errors + 15);
      break;
   case WS_HANDSHAKE_NO_UPGRADE_ERR:
      err->str = *(errors + 16);
      break;
   case WS_HANDSHAKE_NO_CONNECTION_ERR:
      err->str = *(errors + 17);
      break;
   case WS_HANDSHAKE_BAD_ACCEPT_ERR:
      err->str = *(errors + 18);
      break;
   default:
      err->str = *errors;
      break;
   }

   return err;
}


int wsclient_open_connection(const char *host, const char *port)
{
   struct addrinfo hints, *servinfo, *p;
   int rv, sockfd;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
      return WS_OPEN_CONNECTION_ADDRINFO_ERR;
   }

   for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         continue;
      }
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         continue;
      }
      break;
   }
   freeaddrinfo(servinfo);
   if (p == NULL) {
      return WS_OPEN_CONNECTION_ADDRINFO_EXHAUSTED_ERR;
   }
   return sockfd;
}

int wsclient_send(wsclient *client, char *strdata)
{
   wsclient_error *err = NULL;
   struct timeval tv;
   unsigned char mask[4];
   unsigned int mask_int;
   unsigned long long payload_len;
   unsigned char finNopcode;
   unsigned int payload_len_small;
   unsigned int payload_offset = 6;
   unsigned int len_size;
   unsigned long long be_payload_len;
   unsigned int sent = 0;
   int i, sockfd;
   unsigned int frame_size;
   char *data;

   sockfd = client->sockfd;

   if (client->flags & CLIENT_SENT_CLOSE_FRAME) {
      if (client->onerror) {
         err = ws_error(WS_SEND_AFTER_CLOSE_FRAME_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return 0;
   }
   if (client->flags & CLIENT_CONNECTING) {
      if (client->onerror) {
         err = ws_error(WS_SEND_DURING_CONNECT_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return 0;
   }
   if (strdata == NULL) {
      if (client->onerror) {
         err = ws_error(WS_SEND_NULL_DATA_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return 0;
   }

   gettimeofday(&tv, NULL);
   srand(tv.tv_usec * tv.tv_sec);
   mask_int = rand();
   memcpy(mask, &mask_int, 4);
   payload_len = strlen(strdata);
   finNopcode = 0x81; //FIN and text opcode.
   if (payload_len <= 125) {
      frame_size = 6 + payload_len;
      payload_len_small = payload_len;

   }
   else if (payload_len > 125 && payload_len <= 0xffff) {
      frame_size = 8 + payload_len;
      payload_len_small = 126;
      payload_offset += 2;
   }
   else if (payload_len > 0xffff && payload_len <= 0xffffffffffffffffLL) {
      frame_size = 14 + payload_len;
      payload_len_small = 127;
      payload_offset += 8;
   }
   else {
      if (client->onerror) {
         err = ws_error(WS_SEND_DATA_TOO_LARGE_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return -1;
   }
   data = (char *)malloc(frame_size);
   memset(data, 0, frame_size);
   *data = finNopcode;
   *(data + 1) = payload_len_small | 0x80; //payload length with mask bit on
   if (payload_len_small == 126) {
      payload_len &= 0xffff;
      len_size = 2;
      for (i = 0; i < len_size; i++) {
         *(data + 2 + i) = *((char *)&payload_len + (len_size - i - 1));
      }
   }
   if (payload_len_small == 127) {
      payload_len &= 0xffffffffffffffffLL;
      len_size = 8;
      for (i = 0; i < len_size; i++) {
         *(data + 2 + i) = *((char *)&payload_len + (len_size - i - 1));
      }
   }
   for (i = 0; i<4; i++)
      *(data + (payload_offset - 4) + i) = mask[i];

   memcpy(data + payload_offset, strdata, strlen(strdata));
   for (i = 0; i<strlen(strdata); i++)
      *(data + payload_offset + i) ^= mask[i % 4] & 0xff;
   sent = 0;
   i = 0;

   while (sent < frame_size && i >= 0) {
      i = _wsclient_write(client, data + sent, frame_size - sent);
      sent += i;
   }

   if (i < 0) {
      if (client->onerror) {
         err = ws_error(WS_SEND_SEND_ERR);
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
   }

   free(data);
   return sent;
}

int _wsclient_read(wsclient *c, void *buf, size_t length) {
    return recv(c->sockfd, buf, length, 0);
}

int _wsclient_write(wsclient *c, const void *buf, size_t length) {
    return send(c->sockfd, buf, length, 0);
}


void wsclient_onclose(wsclient *client, int(*cb)(wsclient *c)) {
   client->onclose = cb;
}

void wsclient_onopen(wsclient *client, int(*cb)(wsclient *c)) { 
   client->onopen = cb; 
}

void wsclient_onmessage(wsclient *client, int(*cb)(wsclient *c, wsclient_message *msg)) {
   client->onmessage = cb;
}

void wsclient_onerror(wsclient *client, int(*cb)(wsclient *c, wsclient_error *err)) {
   client->onerror = cb;
}

void wsclient_close(wsclient *client)
{
   wsclient_error *err = NULL;
   char data[6];
   int i = 0, n, mask_int;
   struct timeval tv;
   gettimeofday(&tv, NULL);
   srand(tv.tv_sec * tv.tv_usec);
   mask_int = rand();
   memcpy(data + 2, &mask_int, 4);
   data[0] = 0x88;
   data[1] = 0x80;

   do {
      n = _wsclient_write(client, data, 6);
      i += n;
   } while (i < 6 && n > 0);

   if (n < 0) {
      if (client->onerror) {
         err = ws_error(WS_DO_CLOSE_SEND_ERR);
         err->extra_code = n;
         client->onerror(client, err);
         free(err);
         err = NULL;
      }
      return;
   }

   client->flags |= CLIENT_SENT_CLOSE_FRAME;
}

void wsclient_handle_control_frame(wsclient *c, wsclient_frame *ctl_frame) {
   wsclient_error *err = NULL;
   wsclient_frame *ptr = NULL;
   int i, n = 0;
   char mask[4];
   int mask_int;
   struct timeval tv;
   gettimeofday(&tv, NULL);
   srand(tv.tv_sec * tv.tv_usec);
   mask_int = rand();
   memcpy(mask, &mask_int, 4); 
   switch (ctl_frame->opcode) {
   case 0x8:
      //close frame
      if ((c->flags & CLIENT_SENT_CLOSE_FRAME) == 0) {
         //server request close.  Send close frame as acknowledgement.
         for (i = 0; i<ctl_frame->payload_len; i++)
            *(ctl_frame->rawdata + ctl_frame->payload_offset + i) ^= (mask[i % 4] & 0xff); //mask payload
         *(ctl_frame->rawdata + 1) |= 0x80; //turn mask bit on
         i = 0;

         while (i < ctl_frame->payload_offset + ctl_frame->payload_len && n >= 0) {
            n = _wsclient_write(c, ctl_frame->rawdata + i, ctl_frame->payload_offset + ctl_frame->payload_len - i);
            i += n;
         } 
         if (n < 0) {
            if (c->onerror) {
               err = ws_error(WS_HANDLE_CTL_FRAME_SEND_ERR);
               err->extra_code = n;
               c->onerror(c, err);
               free(err);
               err = NULL;
            }
         }
      }
      c->flags |= CLIENT_SHOULD_CLOSE;
      break;
   default:
      fprintf(stderr, "Unhandled control frame received.  Opcode: %d\n", ctl_frame->opcode);
      break;
   }

   ptr = ctl_frame->prev_frame; //This very well may be a NULL pointer, but just in case we preserve it.
   free(ctl_frame->rawdata);
   memset(ctl_frame, 0, sizeof(wsclient_frame));
   ctl_frame->prev_frame = ptr;
   ctl_frame->rawdata = (char *)malloc(FRAME_CHUNK_LENGTH);
   memset(ctl_frame->rawdata, 0, FRAME_CHUNK_LENGTH); 
}

wsclient_message* wsclient_in_data(wsclient *client, char in, int* errcode)
{
   wsclient_frame *current = NULL, *new_frame = NULL;
   unsigned char payload_len_short; 
   if (client->current_frame == NULL) 
   {
      client->current_frame = (wsclient_frame *)malloc(sizeof(wsclient_frame));
      memset(client->current_frame, 0, sizeof(wsclient_frame));
      client->current_frame->payload_len = -1;
      client->current_frame->rawdata_sz = FRAME_CHUNK_LENGTH;
      client->current_frame->rawdata = (char *)malloc(client->current_frame->rawdata_sz);
      memset(client->current_frame->rawdata, 0, client->current_frame->rawdata_sz);
   }
   current = client->current_frame;
   if (current->rawdata_idx >= current->rawdata_sz) 
   {
      current->rawdata_sz += FRAME_CHUNK_LENGTH;
      current->rawdata = (char *)realloc(current->rawdata, current->rawdata_sz);
      memset(current->rawdata + current->rawdata_idx, 0, current->rawdata_sz - current->rawdata_idx);
   }
   *(current->rawdata + current->rawdata_idx++) = in; 

   if (wsclient_complete_frame(client, current) == 1) 
   {
      if (current->fin == 1) {
         //is control frame
         if ((current->opcode & 0x08) == 0x08) {
            wsclient_handle_control_frame(client, current);
         }
         else {
            wsclient_message* msg = wsclient_dispatch_message(client, current, errcode);
            client->current_frame = NULL;
            return msg;
         }
      }
      else {
         new_frame = (wsclient_frame *)malloc(sizeof(wsclient_frame));
         memset(new_frame, 0, sizeof(wsclient_frame));
         new_frame->payload_len = -1;
         new_frame->rawdata = (char *)malloc(FRAME_CHUNK_LENGTH);
         memset(new_frame->rawdata, 0, FRAME_CHUNK_LENGTH);
         new_frame->prev_frame = current;
         current->next_frame = new_frame;
         client->current_frame = new_frame;
      }
   }
   return NULL;
}

wsclient_message* wsclient_dispatch_message(wsclient *c, wsclient_frame *current, int* errcode)
{
   unsigned long long message_payload_len, message_offset;
   int message_opcode, i;
   char *message_payload;
   wsclient_frame *first = NULL;
   wsclient_message *msg = NULL;
   wsclient_error *err = NULL;
   if (current == NULL) {
      if (c->onerror) {
         err = ws_error(WS_DISPATCH_MESSAGE_NULL_PTR_ERR);
         c->onerror(c, err);
         free(err);
         err = NULL;
      }
      *errcode = 1;
      return NULL;
   }
   message_offset = 0;
   message_payload_len = current->payload_len;
   for (; current->prev_frame != NULL; current = current->prev_frame) {
      message_payload_len += current->payload_len;
   }
   first = current;
   message_opcode = current->opcode;
   message_payload = (char *)malloc(message_payload_len + 1);
   memset(message_payload, 0, message_payload_len + 1);
   for (; current != NULL; current = current->next_frame) {
      memcpy(message_payload + message_offset, current->rawdata + current->payload_offset, current->payload_len);
      message_offset += current->payload_len;
   } 
   wsclient_cleanup_frames(first);
   msg = (wsclient_message *)malloc(sizeof(wsclient_message));
   memset(msg, 0, sizeof(wsclient_message));
   msg->opcode = message_opcode;
   msg->payload_len = message_offset;
   msg->payload = message_payload;
   return msg;
}

void free_message(wsclient_message* msg)
{
   if(msg!=NULL)
   {
      if (msg->payload != NULL)
         free(msg->payload);

      free(msg);
   }
}

void wsclient_cleanup_frames(wsclient_frame *first)
{
   wsclient_frame *frame = NULL;
   wsclient_frame *next = first;
   while (next != NULL) {
      frame = next;
      next = frame->next_frame;
      if (frame->rawdata != NULL) {
         free(frame->rawdata);
      }
      free(frame);
   }
}

int wsclient_complete_frame(wsclient *c, wsclient_frame *frame)
{
   wsclient_error *err = NULL;
   int payload_len_short, i;
   unsigned long long payload_len = 0;
   if (frame->rawdata_idx < 2) {
      return 0;
   }
   frame->fin = (*(frame->rawdata) & 0x80) == 0x80 ? 1 : 0;
   frame->opcode = *(frame->rawdata) & 0x0f;
   frame->payload_offset = 2;
   if ((*(frame->rawdata + 1) & 0x80) != 0x0) {
      if (c->onerror) {
         err = ws_error(WS_COMPLETE_FRAME_MASKED_ERR);
         c->onerror(c, err);
         free(err);
         err = NULL;
      } 
      c->flags |= CLIENT_SHOULD_CLOSE; 
      return 0;
   }
   payload_len_short = *(frame->rawdata + 1) & 0x7f;
   switch (payload_len_short) {
   case 126:
      if (frame->rawdata_idx < 4) {
         return 0;
      }
      for (i = 0; i < 2; i++) {
         memcpy((void *)&payload_len + i, frame->rawdata + 3 - i, 1);
      }
      frame->payload_offset += 2;
      frame->payload_len = payload_len;
      break;
   case 127:
      if (frame->rawdata_idx < 10) {
         return 0;
      }
      for (i = 0; i < 8; i++) {
         memcpy((void *)&payload_len + i, frame->rawdata + 9 - i, 1);
      }
      frame->payload_offset += 8;
      frame->payload_len = payload_len;
      break;
   default:
      frame->payload_len = payload_len_short;
      break;

   }
   if (frame->rawdata_idx < frame->payload_offset + frame->payload_len) {
      return 0;
   }
   return 1;
}

 
