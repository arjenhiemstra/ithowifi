
void zepto_min_js_gz_code(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", zepto_min_js_gz, zepto_min_js_gz_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void handleGeneralJs(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/javascript");
  response->addHeader("Server","Project WiFi Web Server");
  
  response->print("var on_ap = false; var hostname = '");
  response->print(EspHostname());
  response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_index); });");
  
  request->send(response);
}

void handleGeneralJsOnAp(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/javascript");
  response->addHeader("Server","Project WiFi Web Server");
  
  response->print("var on_ap = true; var hostname = '");
  response->print(EspHostname());
  response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_wifisetup); });");
  
  request->send(response);
}
