// Copyright (c) 2004-2012 Sergey Lyubka
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef MONGOOSE_HEADER_INCLUDED
#define  MONGOOSE_HEADER_INCLUDED

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct mg_context;     // Handle for the HTTP service itself
struct mg_connection;  // Handle for the individual connection


// This structure contains information about the HTTP request.
struct mg_request_info {
  const char *request_method; // "GET", "POST", etc
  const char *uri;            // URL-decoded URI
  const char *http_version;   // E.g. "1.0", "1.1"
  const char *query_string;   // URL part after '?', not including '?', or NULL
  const char *remote_user;    // Authenticated user, or NULL if no auth used
  long remote_ip;             // Client's IP address
  int remote_port;            // Client's port
  int is_ssl;                 // 1 if SSL-ed, 0 if not
  void *user_data;            // User data pointer passed to mg_start()

  int num_headers;            // Number of HTTP headers
  struct mg_header {
    const char *name;         // HTTP header name
    const char *value;        // HTTP header value
  } http_headers[64];         // Maximum 64 headers
};


// This structure needs to be passed to mg_start(), to let mongoose know
// which callbacks to invoke. For detailed description, see
// https://github.com/valenok/mongoose/blob/master/UserManual.md
struct mg_callbacks {
  // Called when mongoose has received new HTTP request.
  // If callback returns non-zero,
  // callback must process the request by sending valid HTTP headers and body,
  // and mongoose will not do any further processing.
  // If callback returns 0, mongoose processes the request itself. In this case,
  // callback must not send any data to the client.
  int  (*begin_request)(struct mg_connection *);

  // Called when mongoose has finished processing request.
  void (*end_request)(const struct mg_connection *, int reply_status_code);

  // Called when mongoose is about to log a message. If callback returns
  // non-zero, mongoose does not log anything.
  int  (*log_message)(const struct mg_connection *, const char *message);

  // Called when mongoose initializes SSL library.
  int  (*init_ssl)(void *ssl_context, void *user_data);

  // Called when websocket request is received, before websocket handshake.
  // If callback returns 0, mongoose proceeds with handshake, otherwise
  // cinnection is closed immediately.
  int (*websocket_connect)(const struct mg_connection *);

  // Called when websocket handshake is successfully completed, and
  // connection is ready for data exchange.
  void (*websocket_ready)(struct mg_connection *);

  // Called when data frame has been received from the client.
  // Parameters:
  //    bits: first byte of the websocket frame, see websocket RFC at
  //          http://tools.ietf.org/html/rfc6455, section 5.2
  //    data, data_len: payload, with mask (if any) already applied.
  // Return value:
  //    0:     keep this websocket connection opened.
  //    non-0: close this websocket connection.
  int  (*websocket_data)(struct mg_connection *, int bits,
                         char *data, size_t data_len);

  // Called when mongoose tries to open a file. Used to intercept file open
  // calls, and serve file data from memory instead.
  // Parameters:
  //    path:     Full path to the file to open.
  //    data_len: Placeholder for the file size, if file is served from memory.
  // Return value:
  //    NULL: do not serve file from memory, proceed with normal file open.
  //    non-NULL: pointer to the file contents in memory. data_len must be
  //              initilized with the size of the memory block.
  const char * (*open_file)(const struct mg_connection *,
                             const char *path, size_t *data_len);

  // Called when mongoose is about to serve Lua server page (.lp file), if
  // Lua support is enabled.
  // Parameters:
  //   lua_context: "lua_State *" pointer.
  void (*init_lua)(struct mg_connection *, void *lua_context);

  // Called when mongoose has uploaded a file to a temporary directory as a
  // result of mg_upload() call.
  // Parameters:
  //    file_file: full path name to the uploaded file.
  void (*upload)(struct mg_connection *, const char *file_name);

  // Called when mongoose is about to send HTTP error to the client.
  // Implementing this callback allows to create custom error pages.
  // Parameters:
  //   status: HTTP error status code.
  int  (*http_error)(struct mg_connection *, int status);
};

// Start web server.
//
// Parameters:
//   callbacks: mg_callbacks structure with user-defined callbacks.
//   options: NULL terminated list of option_name, option_value pairs that
//            specify Mongoose configuration parameters.
//
// Side-effects: on UNIX, ignores SIGCHLD and SIGPIPE signals. If custom
//    processing is required for these, signal handlers must be set up
//    after calling mg_start().
//
//
// Example:
//   const char *options[] = {
//     "document_root", "/var/www",
//     "listening_ports", "80,443s",
//     NULL
//   };
//   struct mg_context *ctx = mg_start(&my_func, NULL, options);
//
// Refer to https://github.com/valenok/mongoose/blob/master/UserManual.md
// for the list of valid option and their possible values.
//
// Return:
//   web server context, or NULL on error.
struct mg_context *mg_start(const struct mg_callbacks *callbacks,
                            void *user_data,
                            const char **configuration_options);


// Stop the web server.
//
// Must be called last, when an application wants to stop the web server and
// release all associated resources. This function blocks until all Mongoose
// threads are stopped. Context pointer becomes invalid.
void mg_stop(struct mg_context *);


// Get the value of particular configuration parameter.
// The value returned is read-only. Mongoose does not allow changing
// configuration at run time.
// If given parameter name is not valid, NULL is returned. For valid
// names, return value is guaranteed to be non-NULL. If parameter is not
// set, zero-length string is returned.
const char *mg_get_option(const struct mg_context *ctx, const char *name);


// Return array of strings that represent valid configuration options.
// For each option, a short name, long name, and default value is returned.
// Array is NULL terminated.
const char **mg_get_valid_option_names(void);


// Add, edit or delete the entry in the passwords file.
//
// This function allows an application to manipulate .htpasswd files on the
// fly by adding, deleting and changing user records. This is one of the
// several ways of implementing authentication on the server side. For another,
// cookie-based way please refer to the examples/chat.c in the source tree.
//
// If password is not NULL, entry is added (or modified if already exists).
// If password is NULL, entry is deleted.
//
// Return:
//   1 on success, 0 on error.
int mg_modify_passwords_file(const char *passwords_file_name,
                             const char *domain,
                             const char *user,
                             const char *password);


// Return information associated with the request.
struct mg_request_info *mg_get_request_info(struct mg_connection *);


// Send data to the client.
// Return:
//  0   when the connection has been closed
//  -1  on error
//  >0  number of bytes written on success
int mg_write(struct mg_connection *, const void *buf, size_t len);


// Macros for enabling compiler-specific checks for printf-like arguments.
#undef PRINTF_FORMAT_STRING
#if _MSC_VER >= 1400
#include <sal.h>
#if _MSC_VER > 1400
#define PRINTF_FORMAT_STRING(s) _Printf_format_string_ s
#else
#define PRINTF_FORMAT_STRING(s) __format_string s
#endif
#else
#define PRINTF_FORMAT_STRING(s) s
#endif

#ifdef __GNUC__
#define PRINTF_ARGS(x, y) __attribute__((format(printf, x, y)))
#else
#define PRINTF_ARGS(x, y)
#endif

// Send data to the client using printf() semantics.
//
// Works exactly like mg_write(), but allows to do message formatting.
int mg_printf(struct mg_connection *,
              PRINTF_FORMAT_STRING(const char *fmt), ...) PRINTF_ARGS(2, 3);


// Send contents of the entire file together with HTTP headers.
void mg_send_file(struct mg_connection *conn, const char *path);


// Read data from the remote end, return number of bytes read.
int mg_read(struct mg_connection *, void *buf, size_t len);


// Get the value of particular HTTP header.
//
// This is a helper function. It traverses request_info->http_headers array,
// and if the header is present in the array, returns its value. If it is
// not present, NULL is returned.
const char *mg_get_header(const struct mg_connection *, const char *name);


// Get a value of particular form variable.
//
// Parameters:
//   data: pointer to form-uri-encoded buffer. This could be either POST data,
//         or request_info.query_string.
//   data_len: length of the encoded data.
//   var_name: variable name to decode from the buffer
//   dst: destination buffer for the decoded variable
//   dst_len: length of the destination buffer
//
// Return:
//   On success, length of the decoded variable.
//   On error:
//      -1 (variable not found).
//      -2 (destination buffer is NULL, zero length or too small to hold the
//          decoded variable).
//
// Destination buffer is guaranteed to be '\0' - terminated if it is not
// NULL or zero length.
int mg_get_var(const char *data, size_t data_len,
               const char *var_name, char *dst, size_t dst_len);

// Fetch value of certain cookie variable into the destination buffer.
//
// Destination buffer is guaranteed to be '\0' - terminated. In case of
// failure, dst[0] == '\0'. Note that RFC allows many occurrences of the same
// parameter. This function returns only first occurrence.
//
// Return:
//   On success, value length.
//   On error:
//      -1 (either "Cookie:" header is not present at all or the requested
//          parameter is not found).
//      -2 (destination buffer is NULL, zero length or too small to hold the
//          value).
int mg_get_cookie(const struct mg_connection *,
                  const char *cookie_name, char *buf, size_t buf_len);


// Download data from the remote web server.
//   host: host name to connect to, e.g. "foo.com", or "10.12.40.1".
//   port: port number, e.g. 80.
//   use_ssl: wether to use SSL connection.
//   error_buffer, error_buffer_size: error message placeholder.
//   request_fmt,...: HTTP request.
// Return:
//   On success, valid pointer to the new connection, suitable for mg_read().
//   On error, NULL. error_buffer contains error message.
// Example:
//   char ebuf[100];
//   struct mg_connection *conn;
//   conn = mg_download("google.com", 80, 0, ebuf, sizeof(ebuf),
//                      "%s", "GET / HTTP/1.0\r\nHost: google.com\r\n\r\n");
struct mg_connection *mg_download(const char *host, int port, int use_ssl,
                                  char *error_buffer, size_t error_buffer_size,
                                  PRINTF_FORMAT_STRING(const char *request_fmt),
                                  ...) PRINTF_ARGS(6, 7);


// Close the connection opened by mg_download().
void mg_close_connection(struct mg_connection *conn);


// File upload functionality. Each uploaded file gets saved into a temporary
// file and MG_UPLOAD event is sent.
// Return number of uploaded files.
int mg_upload(struct mg_connection *conn, const char *destination_dir);


// Convenience function -- create detached thread.
// Return: 0 on success, non-0 on error.
typedef void * (*mg_thread_func_t)(void *);
int mg_start_thread(mg_thread_func_t f, void *p);


// Return builtin mime type for the given file name.
// For unrecognized extensions, "text/plain" is returned.
const char *mg_get_builtin_mime_type(const char *file_name);


// Return Mongoose version.
const char *mg_version(void);


// MD5 hash given strings.
// Buffer 'buf' must be 33 bytes long. Varargs is a NULL terminated list of
// ASCIIz strings. When function returns, buf will contain human-readable
// MD5 hash. Example:
//   char buf[33];
//   mg_md5(buf, "aa", "bb", NULL);
char *mg_md5(char buf[33], ...);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MONGOOSE_HEADER_INCLUDED
