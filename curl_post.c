#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
//#include <jsonz/jsonz.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
 
struct http_response {
  char *data;
  int length;
};
 
static size_t http_callback(char *ptr, size_t count, size_t blocksize, void *userdata)
{
  struct http_response *response = userdata;
  size_t size = count * blocksize;
  memcpy(response->data + response->length, ptr, size);
  response->length += size;
  return size;
}
 
int main(void){
  CURL *curl;
  struct curl_httppost *form, *lastptr;
  struct curl_slist *headers;
 
  FILE *fin;
  unsigned char *file_contents;
  size_t bytes_read;
  struct stat sb;
  struct http_response *resp;
 
  if (stat("record_speech", &sb) != 0){
    return -1;
    }
 
    file_contents = malloc(sb.st_size + 1);
 
    fin = fopen("record_speech", "r");
 
  bytes_read = fread(file_contents, sizeof(unsigned char), sb.st_size, fin);
 
  if (bytes_read != sb.st_size) {
    return -1;
  }
 
  /**
   * Initialize the variables
   */
  resp = malloc(sizeof(*resp));
  if (!resp) {
    return -1;
  }
 
  resp->data = malloc(1024);
  resp->length = 0;
 
  curl = curl_easy_init();
  if (!curl) {
    return -1;
  }
 
  form = NULL;
  lastptr = NULL;
  headers = NULL;
 
  headers = curl_slist_append(headers, "Content-Type: audio/x-speex-with-header-byte; rate=16000");
 
  curl_formadd(&form, &lastptr, CURLFORM_COPYNAME, "record_speechdd", CURLFORM_BUFFER, "data", CURLFORM_BUFFERLENGTH, bytes_read, CURLFORM_BUFFERPTR, file_contents, CURLFORM_END);
 
  /**
   * Setup the cURL handle
   */
  curl_easy_setopt(curl, CURLOPT_URL, "http://www.google.com/speech-api/v1/recognize?xjerr=1&pfilter=1&client=chromium&lang=ru-RU&maxresults=4");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
 
  /**
   * SSL certificates are not available on iOS, so we have to trust Google
   * (0 means false)
   */
  //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
 
  /**
   * Initiate the HTTP(S) transfer
   */
  curl_easy_perform(curl);
 
  /**
   * Clean up
   */
  curl_formfree(form);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
 
  /**
   * NULL-terminate the JSON response string
   */
  resp->data[resp->length] = '\0';
 
  printf("%s\n", resp->data);
 
  return 1;
}