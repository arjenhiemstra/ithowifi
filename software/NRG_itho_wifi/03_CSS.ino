

void css_code(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", pure_min_css_gz, pure_min_css_gz_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
