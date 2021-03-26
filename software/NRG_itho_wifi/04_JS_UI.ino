
const char controls_js[] PROGMEM = R"=====(

var count = 0;
var itho_low = 0;
var itho_medium = 127;
var itho_high = 254;
var sht3x = -1;
var websocketServerLocation = 'ws://' + window.location.hostname + '/ws';

function startWebsock(websocketServerLocation){
  websock = new WebSocket(websocketServerLocation);
  websock.onmessage = function(b) {
          console.log(b);
          var f = JSON.parse(b.data);
          var g = document.body;
          if (f.wifisettings) {
            let x = f.wifisettings;
            $('#ssid').val(x.ssid);
            $('#passwd').val(x.passwd);
            var $radios = $('input[name=\'option-dhcp\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value=' + x.dhcp + ']').prop('checked', true);
            }
            radio("dhcp", x.dhcp);
            $('#renew').val(x.renew);
            $('#ip').val(x.ip);
            $('#subnet').val(x.subnet);
            $('#gateway').val(x.gateway);
            $('#dns1').val(x.dns1);
            $('#dns2').val(x.dns2);
            $('#port').val(x.port);
          }
          else if (f.systemsettings) {
            let x = f.systemsettings;
            $('#sys_username').val(x.sys_username);
            $('#sys_password').val(x.sys_password);
            var $radios = $('input[name=\'option-syssec_web\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.syssec_web + '"]').prop('checked', true);
            }
            var $radios = $('input[name=\'option-syssec_api\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.syssec_api + '"]').prop('checked', true);
            }        
            var $radios = $('input[name=\'option-syssec_edit\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.syssec_edit + '"]').prop('checked', true);
            }
            var $radios = $('input[name=\'option-syssht30\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.syssht30 + '"]').prop('checked', true);
            }
            var $radios = $('input[name=\'option-mqtt_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.mqtt_active + '"]').prop('checked', true);
            }
            radio("mqtt_active", x.mqtt_active);
            $('#mqtt_serverName').val(x.mqtt_serverName);
            $('#mqtt_username').val(x.mqtt_username);
            $('#mqtt_password').val(x.mqtt_password);
            $('#mqtt_port').val(x.mqtt_port);
            $('#mqtt_state_topic').val(x.mqtt_state_topic);
            mqtt_state_topic_tmp = x.mqtt_state_topic;
            $('#mqtt_sensor_topic').val(x.mqtt_sensor_topic);
            $('#mqtt_ha_topic').val(x.mqtt_ha_topic);
            $('#mqtt_cmd_topic').val(x.mqtt_cmd_topic);
            mqtt_cmd_topic_tmp = x.mqtt_cmd_topic;
            $('#mqtt_lwt_topic').val(x.mqtt_lwt_topic);
            $radios = $('input[name=\'option-mqtt_domoticz_active\']');
            $('#mqtt_idx, #label-mqtt_idx').hide();
            $('#sensor_idx, #label-sensor_idx').hide();
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.mqtt_domoticz_active + '"]').prop('checked', true);
            }
            radio("mqtt_domoticz", x.mqtt_domoticz_active);
            radio("mqtt_ha", x.mqtt_ha_active);
            $radios = $('input[name=\'option-mqtt_ha_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.mqtt_ha_active + '"]').prop('checked', true);
            }
            radio("mqtt_ha", x.mqtt_ha_active);
            $('#mqtt_idx').val(x.mqtt_idx);
            $('#sensor_idx').val(x.sensor_idx);
            var $radios = $('input[name=\'option-autopilot_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.autopilot_active + '"]').prop('checked', true);
            }
            radio("autopilot_active", x.autopilot_active);
            $('#autopilot_hum_upper').val(x.autopilot_hum_upper);
            $('#autopilot_hum_lower').val(x.autopilot_hum_lower);
            $('#itho_fallback').val(x.itho_fallback);
            $('#itho_low').val(x.itho_low);
            $('#itho_medium').val(x.itho_medium);
            $('#itho_high').val(x.itho_high);
            $('#itho_timer1').val(x.itho_timer1);
            $('#itho_timer2').val(x.itho_timer2);
            $('#itho_timer3').val(x.itho_timer3);
            $radios = $('input[name=\'option-vremotejoin_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.itho_sendjoin + '"]').prop('checked', true);
            }
            $radios = $('input[name=\'option-vremotemedium_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.itho_forcemedium + '"]').prop('checked', true);
            }
            $radios = $('input[name=\'option-vremoteapi_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.itho_vremapi + '"]').prop('checked', true);
            }
            $radios = $('input[name=\'option-vremoteswap_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.itho_vremswap + '"]').prop('checked', true);
            }
            $radios = $('input[name=\'option-itho_remotes_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value="' + x.itho_rf_support + '"]').prop('checked', true);
            }
            radio("itho_rf_support", x.itho_rf_support);
            if (x.itho_rf_support == 1 && x.rfInitOK == false) {
              if (confirm("For changes to take effect click 'Ok' to reboot")) {
                $('#main').empty();
                $('#main').append("<br><br><br><br>");
                $('#main').append(html_reboot_script);
                websock.send('{\"reboot\":true}');
              }
            }
            if (x.itho_rf_support == 1 && x.rfInitOK == true) {              
              $('#remotemenu').removeClass('hidden');
            }
          }     
          else if (f.remotes) {
            let x = f.remotes;
            $('#RemotesTable').empty();
            buildHtmlTable('#RemotesTable', x);
          }
          else if (f.wifiscanresult) {
            let x = f.wifiscanresult;
            $('#wifiscanresult').append('<div class=\'ScanResults\'><input id=\'' + x.id + '\' class=\'pure-input-1-5\' name=\'optionsWifi\' value=\'' + x.ssid + '\' type=\'radio\'>' + returnSignalSVG(x.sigval) + '' + returnWifiSecSVG(x.sec) + ' ' + x.ssid + '</label></div>');
          }
          else if (f.systemstat) {
            let x = f.systemstat;
            if('sensor_temp' in x) {
              $('#sensor_temp').html('Temperature: '+ round(x.sensor_temp, 1) + '&#8451;');
            }
            if('sensor_hum' in x) {
              $('#sensor_hum').html('Humidity: ' + round(x.sensor_hum, 1) + '%');
            }
            $('#memory_box').show();
            $('#memory_box').html('<p><b>Memory:</b><p><p>free: <b>' + x.freemem + '</b></p><p>low: <b>' + x.memlow + '</b></p>');
            $('#mqtt_conn').removeClass();
            var button = returnMqttState(x.mqqtstatus);
            $('#mqtt_conn').addClass("pure-button " + button.button);
            $('#mqtt_conn').text(button.state);
            $('#itho').val(x.itho);
            $('#ithotextval').html(x.itho);
            if ('itho_low' in x) {
              itho_low = x.itho_low;
            }
            if ('itho_medium' in x) {
              itho_medium = x.itho_medium;
            }
            if ('itho_high' in x) {
              itho_high = x.itho_high;
            }                        
            var initstatus = '';
            if (x.ithoinit == -1) {
              initstatus = '<span style="color:#ca3c3c;">init failed - please power cycle the itho unit -</span>';
            }
            else if (x.ithoinit == 1) {
              initstatus = '<span style="color:#1cb841;">connected</span>';
            }
            else {
              initstatus = 'unknown status';
            }
            $('#ithoinit').html(initstatus);
            if ('sht3x' in x) {
              sht3x = x.sht3x;
            }
            if(x.itho_llm > 0) {
              $('#itho_llm').removeClass();
              $('#itho_llm').addClass("pure-button button-success");
              $('#itho_llm').text("On " + x.itho_llm + "s");
            }
            else {
              $('#itho_llm').removeClass();
              $('#itho_llm').addClass("pure-button button-secondary");
              $('#itho_llm').text("Off");   
            }
          }  
          else if (f.messagebox) {
            let x = f.messagebox;
            count += 1;
            resetTimer();
            $('#message_box').show();
            $('#message_box').append('<p class=\'messageP\' id=\'mbox_p' + count + '\'>Message: ' + x.message + '</p>');
            removeAfter5secs(count);
          }
          else if (f.rflog) {
            let x = f.rflog;
            $('#rflog_outer').removeClass('hidden');
            var d = new Date();
            $('#rflog').prepend(d.toISOString() + ': ' + x.message + '<br>');
          }
          else if (f.ota) {
            let x = f.ota;
            $('#updateprg').html('Firmware update progress: ' + x.percent + '%');
            moveBar(x.percent, "updateBar");
          }
  };
  websock.onopen = function(a) {
      console.log('websock open');
      document.getElementById("layout").style.opacity = 1;
      document.getElementById("loader").style.display = "none";
      if (lastPageReq !== "") {
        update_page(lastPageReq);
      }
      getSettings('syssetup');
  };
  websock.onclose = function(a) {
      console.log('websock close');
      // Try to reconnect in 2 seconds
      websock = null;
      document.getElementById("layout").style.opacity = 0.3;
      document.getElementById("loader").style.display = "block";
      setTimeout(function(){startWebsock(websocketServerLocation)}, 2000);
  };
  websock.onerror = function(a) {
      console.log(a);
  };
}


$(document).ready(function() {
  document.getElementById("layout").style.opacity = 0.3;
  document.getElementById("loader").style.display = "block";
  startWebsock(websocketServerLocation);
  //handle menu clicks
  $(document).on('click', 'ul.pure-menu-list li a', function(event) {
    var page = $(this).attr('href');
    update_page(page);
    $('li.pure-menu-item').removeClass("pure-menu-selected");
    $(this).parent().addClass("pure-menu-selected");
    event.preventDefault();
  });
  $(document).on('click', '#headingindex', function(event) {
    update_page('index');
    $('li.pure-menu-item').removeClass("pure-menu-selected");
    event.preventDefault();
  });
  //handle wifi network select
  $(document).on('change', 'input', function(e) {
    if ($(this).attr('name') == 'optionsWifi') {
      $('#ssid').val($(this).attr('value'));
    } 
  }); 
  //handle submit buttons
  $(document).on('click', 'button', function(e) {
    if ($(this).attr('id') == 'wifisubmit') {
      websock.send(JSON.stringify({
        wifisettings: {
          ssid:     $('#ssid').val(),
          passwd:   $('#passwd').val(),
          dhcp:     $('input[name=\'option-dhcp\']:checked').val(),
          renew:    $('#renew').val(),
          ip:       $('#ip').val(),
          subnet:   $('#subnet').val(),
          gateway:  $('#gateway').val(),
          dns1:     $('#dns1').val(),
          dns2:     $('#dns2').val(),
          port:     $('#port').val()
        }
      }));
      update_page('wifisetup');
    }
    //syssubmit
    else if ($(this).attr('id') == 'syssumbit') {
      websock.send(JSON.stringify({
        systemsettings: {
          sys_username:     $('#sys_username').val(),
          sys_password:     $('#sys_password').val(),
          syssec_web:       $('input[name=\'option-syssec_web\']:checked').val(),          
          syssec_api:       $('input[name=\'option-syssec_api\']:checked').val(),          
          syssec_edit:      $('input[name=\'option-syssec_edit\']:checked').val(),
          syssht30:         $('input[name=\'option-syssht30\']:checked').val(),
          itho_rf_support:  $('input[name=\'option-itho_remotes_active\']:checked').val()
        }
      }));
      update_page('system');
    }    
    //mqttsubmit
    else if ($(this).attr('id') == 'mqttsubmit') {
      websock.send(JSON.stringify({
        systemsettings: {
          mqtt_active:          $('input[name=\'option-mqtt_active\']:checked').val(),
          mqtt_serverName:      $('#mqtt_serverName').val(),
          mqtt_username:        $('#mqtt_username').val(),
          mqtt_password:        $('#mqtt_password').val(),
          mqtt_port:            $('#mqtt_port').val(),
          mqtt_version:         $('#mqtt_version').val(),
          mqtt_state_topic:     $('#mqtt_state_topic').val(),
          mqtt_sensor_topic:    $('#mqtt_sensor_topic').val(),
          mqtt_ha_topic:        $('#mqtt_ha_topic').val(),
          mqtt_cmd_topic:       $('#mqtt_cmd_topic').val(),
          mqtt_lwt_topic:       $('#mqtt_lwt_topic').val(),
          mqtt_idx:             $('#mqtt_idx').val(),
          sensor_idx:           $('#sensor_idx').val(),
          mqtt_domoticz_active: $('input[name=\'option-mqtt_domoticz_active\']:checked').val(),
          mqtt_ha_active:       $('input[name=\'option-mqtt_ha_active\']:checked').val()
        }
      }));
      update_page('mqtt');
    }
    else if ($(this).attr('id') == 'ithosubmit') {
      websock.send(JSON.stringify({
        systemsettings: {
          itho_fallback:    $('#itho_fallback').val(),
          itho_low:         $('#itho_low').val(),
          itho_medium:      $('#itho_medium').val(),
          itho_high:        $('#itho_high').val(),
          itho_timer1:      $('#itho_timer1').val(),
          itho_timer2:      $('#itho_timer2').val(),
          itho_timer3:      $('#itho_timer3').val(),
          itho_sendjoin:    $('input[name=\'option-vremotejoin_active\']:checked').val(),
          itho_forcemedium: $('input[name=\'option-vremotemedium_active\']:checked').val(),
          itho_vremapi:     $('input[name=\'option-vremoteapi_active\']:checked').val(),
          itho_vremswap:    $('input[name=\'option-vremoteswap_active\']:checked').val()
        }
      }));
      update_page('itho');
    }
    else if ($(this).attr('id') == 'autopilotsubmit') {
      websock.send(JSON.stringify({
        systemsettings: {
          autopilot_active:       $('input[name=\'option-autopilot_active\']:checked').val(),
          autopilot_hum_upper:    $('#autopilot_hum_upper').val(),
          autopilot_hum_lower:    $('#autopilot_hum_lower').val(),
        }
      }));
      update_page('autopilot');
    }
    else if ($(this).attr('id') == 'itho_llm') {
      websock.send('{\"itho_llm\":true}');
    }
    else if ($(this).attr('id') == 'itho_remove_remote') {
      var selected = $('input[name=\'optionsRemotes\']:checked').val();
      if (selected == null) {
        alert("Please select a remote.")
      }
      else {
        websock.send('{\"itho_remove_remote\":' + selected + '}');
      }
    }
    else if ($(this).attr('id') == 'itho_update_remote') {
      var selected = $('input[name=\'optionsRemotes\']:checked').val();
      if (selected == null) {
        alert("Please select a remote.");
      }
      else {
        websock.send('{\"itho_update_remote\":' + selected + ', \"value\":\"' + $('#name_remote-' + selected).val() + '\"}');
      }
    }
    else if ($(this).attr('id') == 'resetwificonf') {
      if (confirm("This will reset the wifi config to factory default, are you sure?")) {
        websock.send('{\"resetwificonf\":true}');
      }
    }
    else if ($(this).attr('id') == 'resetsysconf') {
      if (confirm("This will reset the system config to factory default, are you sure?")) {
        websock.send('{\"resetsysconf\":true}');
      }
    }
    else if ($(this).attr('id') == 'reboot') {
      if (confirm("This will reboot the device, are you sure?")) {
        $('#rebootscript').append(html_reboot_script);
        websock.send('{\"reboot\":true}');
      }
    }
    else if ($(this).attr('id') == 'wifiscan') {
      $('.ScanResults').remove();
      $('.hidden').removeClass('hidden');
      websock.send('{\"wifiscan\":true}');
    }
    else if ($(this).attr('id') == 'button1') {
      websock.send('{\"ithobutton\":1}');
    }
    else if ($(this).attr('id') == 'button2') {
      websock.send('{\"ithobutton\":2}');
    }
    else if ($(this).attr('id') == 'button3') {
      websock.send('{\"ithobutton\":3}');
    }
    else if ($(this).attr('id') == 'buttonjoin') {
      websock.send('{\"ithobutton\":11}');
    }
    else if ($(this).attr('id') == 'buttonleave') {
      websock.send('{\"ithobutton\":99}');
    }
    else if ($(this).attr('id') == 'buttontype') {
      websock.send('{\"ithobutton\":20}');
    }
    else if ($(this).attr('id') == 'buttonstatus') {
      websock.send('{\"ithobutton\":30}');
    }
    else if ($(this).attr('id') == 'buttonstatusformat') {
      websock.send('{\"ithobutton\":31}');
    }    
    else if ($(this).attr('id') == 'updatesubmit') {
      e.preventDefault();
      var form = $('#updateform')[0];
      var data = new FormData(form);
      let filename = data.get('update').name;
      if(!filename.endsWith(".bin")) {
        count += 1;
        resetTimer();
        $('#message_box').show();
        $('#message_box').append('<p class=\'messageP\' id=\'mbox_p' + count + '\'>Updater: file name error, please select a *.bin firmware file</p>');
        removeAfter5secs(count);
        return;
      }
      $('#uploadProgress').show();
      $('#updateProgress').show();
      $('#uploadprg').show();
      $('#updateprg').show();
      
      $.ajax({
          url: '/update',
          type: 'POST',
          data: data,
          contentType: false,
          processData: false,
          xhr: function() {
              var xhr = new window.XMLHttpRequest();
              xhr.upload.addEventListener('progress', function(evt) {
                  if (evt.lengthComputable) {
                      var per = Math.round(10 + (((evt.loaded / evt.total)*100)*0.9));
                      $('#uploadprg').html('File upload progress: ' + per + '%');
                      moveBar(per, "uploadBar");
                      if (per == 100) {
                        $('#uploaddone').show();
                        $('#uploaddone').html('Done!');
                      }
                  }
             }, false);
             return xhr;
          },
          success:function(d, s) {
              moveBar(100, "updateBar");
              $('#updateprg').html('Firmware update progress: 100%');
              $('#updatedone').show();
              $('#updatedone').html('Done!');  
              console.log('success!')
              $('#time').show();
              startCountdown();
         },
          error: function () {
            console.log('failed!');
            startCountdown();
          }
        });
    }
    e.preventDefault();
    return false;
  });
  //keep the message box on top on scroll
  $(window).scroll(function(e) {
    var $el = $('#message_box');
    var isPositionFixed = ($el.css('position') == 'fixed');
    if ($(this).scrollTop() > 100 && !isPositionFixed) {
      $('#message_box').css({
        'position': 'fixed',
        'top': '0px'
      });
    }
  });
});

var timerHandle = setTimeout(function() {
    $('#message_box').hide();
}, 5000);

function resetTimer() {
    window.clearTimeout(timerHandle);
    timerHandle = setTimeout(function() {
        $('#message_box').hide();
    }, 5000);
}

function removeID(id) {
    $('#' + id).remove();
}

function removeAfter5secs(count) {
  //await timeoutPromise(200);
    new Promise(resolve => {
        setTimeout(() => {
            removeID('mbox_p' + count);
        }, 5000);
    });
}

function round(value, precision) {
    var multiplier = Math.pow(10, precision || 0);
    return Math.round(value * multiplier) / multiplier;
}

var mqtt_state_topic_tmp = "";
var mqtt_cmd_topic_tmp = "";

function radio(origin, state) {
  if (origin == "dhcp") {
    if (state == 'on') {
      $('#ip, #subnet, #gateway, #dns1, #dns2').prop('readonly', true);
      $('#renew, #port').prop('readonly', false);
      $('#option-dhcp-on, #option-dhcp-off').prop('disabled', false);
    }
    else {
      $('#ip, #subnet, #gateway, #dns1, #dns2, #port').prop('readonly', false);
      $('#renew').prop('readonly', true);
      $('#option-dhcp-on, #option-dhcp-off').prop('disabled', false);
    }
  }
  else if (origin == "mqtt_active") {
    if (state == 1) {
      $('#mqtt_serverName, #mqtt_username, #mqtt_password, #mqtt_port, #mqtt_state_topic, #mqtt_sensor_topic, #mqtt_ha_topic, #mqtt_cmd_topic, #mqtt_lwt_topic, #mqtt_idx').prop('readonly', false);
      $('#option-mqtt_domoticz-on, #option-mqtt_domoticz-off').prop('disabled', false);
      $('#option-mqtt_ha-on, #option-mqtt_ha-off').prop('disabled', false);
    }
    else {
      $('#mqtt_serverName, #mqtt_username, #mqtt_password, #mqtt_port, #mqtt_state_topic, #mqtt_sensor_topic, #mqtt_ha_topic, #mqtt_cmd_topic, #mqtt_lwt_topic, #mqtt_idx').prop('readonly', true);
      $('#option-mqtt_domoticz-on, #option-mqtt_domoticz-off').prop('disabled', true);
      $('#option-mqtt_ha-on, #option-mqtt_ha-off').prop('disabled', true);
    }
  }
  else if (origin == "autopilot_active") {
    if (state == 1) {
      $('#autopilot_hum_upper, #autopilot_hum_lower').prop('readonly', false);
      $('#autopilot_hum_upper, #autopilot_hum_lower').prop('disabled', false);
    }
    else {
      $('#autopilot_hum_upper, #autopilot_hum_lower').prop('readonly', true);
      $('#autopilot_hum_upper, #autopilot_hum_lower').prop('disabled', true);
    }
  }
  else if (origin == "mqtt_domoticz") {
    if (state == 1) {
      $('#mqtt_idx').prop('readonly', false);
      $('#mqtt_idx, #label-mqtt_idx').show();
      if(sht3x > 0) { $('#sensor_idx, #label-sensor_idx').show(); }
      else { $('#sensor_idx, #label-sensor_idx').hide(); }
      $('#mqtt_state_topic').val("domoticz/in");
      $('#mqtt_cmd_topic').val("domoticz/out");
      $('#mqtt_sensor_topic, #label-mqtt_sensor').hide();
      $('#mqtt_ha_topic, #label-mqtt_ha').hide();
      $('#mqtt_lwt_topic, #label-lwt_topic').hide();
    }
    else {
      $('#mqtt_idx').prop('readonly', true);
      $('#mqtt_idx, #label-mqtt_idx').hide();
      $('#sensor_idx, #label-sensor_idx').hide();
      $('#mqtt_state_topic').val(mqtt_state_topic_tmp);
      $('#mqtt_cmd_topic').val(mqtt_cmd_topic_tmp);
      if(sht3x > 0) { $('#mqtt_sensor_topic, #label-mqtt_sensor').show(); }
      else { $('#mqtt_sensor_topic, #label-mqtt_sensor').hide(); }
      $('#mqtt_ha_topic, #label-mqtt_ha').show();
      $('#mqtt_lwt_topic, #label-lwt_topic').show();
    }
  }
  else if (origin == "mqtt_ha") {
    if (state == 1) {
      $('#mqtt_ha_topic, #label-mqtt_ha').show();
      $('#mqtt_idx').prop('readonly', true);
      $('#mqtt_idx, #label-mqtt_idx').hide();
      $('#sensor_idx, #label-sensor_idx').hide();
      if(sht3x > 0) { $('#mqtt_sensor_topic, #label-mqtt_sensor').show(); }
      else { $('#mqtt_sensor_topic, #label-mqtt_sensor').hide(); }
      $('#mqtt_lwt_topic, #label-lwt_topic').show();
    }
    else {
      $('#mqtt_ha_topic, #label-mqtt_ha').hide();
    }
  }
  else if (origin == "remote") {
    $('[id^=name_remote-]').each(function(index, item){
        $(item).prop('readonly', true);
      if (index == state) {
        $(item).prop('readonly', false);
      }
    });
  }
}

function timeoutPromise(timer) {
  return new Promise(resolve => setTimeout(resolve, timer));
}
// try 10 times within 2 secs to send a websock request
const getSettings = async (page) => {  
  for (let i = 1; i < 11; i++) {
    await timeoutPromise(200);
    if(websock.readyState === 1){
    websock.send(JSON.stringify(new function(){ this[page] = 1; }));
    //console.log(page + ' requested');
    return "success";
    }
  }
  //console.log(page + ' requested but failed');
  return "failed";
}

//
// Reboot script
//
var secondsRemaining;
var intervalHandle;
function resetPage() {
  window.location.href=('http://' + hostname + '.local');
}
function tick() {
  var timeDisplay = document.getElementById('time');
  var min = Math.floor(secondsRemaining/60);
  var sec=secondsRemaining-(min*60);
  if (sec<10) {
      sec='0'+sec;
  }
  var message = 'This page will reload to the start page in ' + min.toString() + ':' + sec + ' seconds...';
  timeDisplay.innerHTML = message;
  if (secondsRemaining === 0) { 
    clearInterval(intervalHandle);
    resetPage();
  }
  secondsRemaining--;
}
function startCountdown() {
  secondsRemaining = 20;
  intervalHandle = setInterval(tick, 1000);
}
function moveBar(nPer, element) {
  var elem = document.getElementById(element);
  elem.style.width = nPer + '%'; 
}
//
//
//
function updateSlider(value) {
  var val = parseInt(value);
  $('#ithotextval').html(val);
  websock.send(JSON.stringify({'itho':val}));
}

//function to load html main content
var lastPageReq = "";
function update_page(page) {
    lastPageReq = page;  
    $('#main').empty();
    $('#main').css('max-width', '768px')
    if (page == 'index') { $('#main').append(html_index); }
    if (page == 'wifisetup') { $('#main').append(html_wifisetup); }
    if (page == 'system') { $('#main').append(html_systemsettings_start); 
      if(hw_revision == "2") { $('#sys_fieldset').append(html_systemsettings_cc1101); }      
      $('#sys_fieldset').append(html_systemsettings_end);
    }
    if (page == 'itho') { $('#main').append(html_ithosettings); }
    if (page == 'autopilot') { $('#main').append(html_autopilotsettings); }
    if (page == 'remotes') { $('#main').append(html_remotessetup); }    
    if (page == 'mqtt') { $('#main').append(html_mqttsetup); }
    if (page == 'api') { $('#main').append(html_api); }      
    if (page == 'help') { $('#main').append(html_help); }
    if (page == 'reset') { $('#main').append(html_reset); }
    if (page == 'update') { $('#main').append(html_update); }
    if (page == 'debug') { $('#main').load( 'debug' ); }
}

//handle menu collapse on smaller screens
window.onload = function(){ 
  (function(window, document) {
    var layout = document.getElementById('layout'),
      menu = document.getElementById('menu'),
      menuLink = document.getElementById('menuLink'),
      content = document.getElementById('main');
    function toggleClass(element, className) {
      var classes = element.className.split(/\s+/),
        length = classes.length,
        i = 0;
      for (; i < length; i++) {
        if (classes[i] === className) {
          classes.splice(i, 1);
          break;
        }
      }
      if (length === classes.length) {
        classes.push(className);
      }
      element.className = classes.join(' ');
    }
    function toggleAll(e) {
      var active = 'active';
      e.preventDefault();
      toggleClass(layout, active);
      toggleClass(menu, active);
      toggleClass(menuLink, active);
    }
    menuLink.onclick = function(e) {
      toggleAll(e);
    };
    content.onclick = function(e) {
      if (menu.className.indexOf('active') !== -1) {
        toggleAll(e);
      }
    };
  }(this, this.document));
};

function ValidateIPaddress(ipaddress) 
{
 if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ipaddress)) {
    return true;
  }
return false;
}
function ValidateBetween(min, max) {
    if (x < min || x > max) { return false; }
    return true;
}

function returnMqttState(state) {
  state = state + 5;
  var states = [ "Disabled", "Connection Timeout" ,"Connection Lost" ,"Connection Failed" ,"Disconnected" ,"Connected" ,"MQTT version unsupported" ,"Client ID rejected" ,"Server Unavailable" ,"Bad Credentials" ,"Client Unauthorized"];
  var button = "";
  if (state == 0) {
    button = "";
  }
  else if (state < 5) {
    button = "button-error";
  }
  else if (state > 5) {
     button = "button-warning";
  }
  else {
    button = "button-success";
  }
  return {"state":states[state], "button":button};
}

//functions to generate SVG images
function returnSignalSVG(signalVal) {
   var returnString = '';
   returnString += '<svg id=\'svgSignalImage\' width=28 height=28 xmlns=\'http://www.w3.org/2000/svg\' xmlns:xlink=\'http://www.w3.org/1999/xlink\' version=\'1.1\' x=\'0px\' y=\'0px\' viewBox=\'0 0 96 120\' style=\'enable-background:new 0 0 96 96;\' xml:space=\'preserve\'>';
   returnString += '<rect x=\'5\' y=\'70.106\' width=\'13.404\' height=\'13.404\' rx=\'2\' ry=\'2\' fill=\'#';
   if (signalVal > 0) {
       returnString += '000000';
   } else {
       returnString += 'cccccc';
   }
   returnString += '\'/>';
   returnString += '<rect x=\'24.149\' y=\'56.702\' width=\'13.404\' height=\'26.809\' rx=\'2\' ry=\'2\' fill=\'#';
   if (signalVal > 1) {
       returnString += '000000';
   } else {
       returnString += 'cccccc';
   }
   returnString += '\'/>';
   returnString += '<rect x=\'43.298\' y=\'43.298\' width=\'13.404\' height=\'40.213\' rx=\'2\' ry=\'2\' fill=\'#';
   if (signalVal > 2) {
       returnString += '000000';
   } else {
       returnString += 'cccccc';
   }
   returnString += '\'/>';
   returnString += '<rect x=\'62.447\' y=\'29.894\' width=\'13.403\' height=\'53.617\' rx=\'2\' ry=\'2\' fill=\'#';
   if (signalVal > 3) {
       returnString += '000000';
   } else {
       returnString += 'cccccc';
   }
   returnString += '\'/>';
   returnString += '<rect x=\'81.596\' y=\'16.489\' width=\'13.404\' height=\'67.021\' rx=\'2\' ry=\'2\' fill=\'#';
   if (signalVal > 4) {
       returnString += '000000';
   } else {
       returnString += 'cccccc';
   }
   returnString += '\'/></svg>';
   return returnString;
}

function returnWifiSecSVG(secVal) {
   var returnString = '';
   returnString += '<svg id=\'svgSignalImage\' width=30 height=30 xmlns=\'http://www.w3.org/2000/svg\' xmlns:xlink=\'http://www.w3.org/1999/xlink\' version=\'1.1\' x=\'0px\' y=\'0px\' viewBox=\'0 0 96 120\' style=\'enable-background:new 0 0 96 96;\' xml:space=\'preserve\'>';
   if (secVal == 2) {
       returnString += '<g><path d=\'M64,41h-4v-8.2c0-6.6-5.4-12-12-12s-12,5.4-12,12V41h-4c-2.2,0-4,2-4,4.2v22c0,4.4,3.6,7.8,8,7.8h24c4.4,0,8-3.3,8-7.8v-22   C68,43,66.2,41,64,41z M40,32.8c0-4.4,3.6-8,8-8s8,3.6,8,8V41H40V32.8z M64,67.2c0,2.2-1.8,3.8-4,3.8H36c-2.2,0-4-1.5-4-3.8V45h32   V67.2z\'/><g><path d=\'M48,62c-1.1,0-2-0.9-2-2v-2c0-1.1,0.9-2,2-2s2,0.9,2,2v2C50,61.1,49.1,62,48,62z\'/></g></g>';
   } else if (secVal == 1) {
       returnString += '<g><path d=\'M66,20.8c-6.6,0-12,5.4-12,12V41H32c-2.2,0-4,2-4,4.2v22c0,4.4,3.6,7.8,8,7.8h24c4.4,0,8-3.3,8-7.8v-22   c0-2.2-1.8-4.2-4-4.2h-6v-8.2c0-4.4,3.6-8,8-8s8,3.6,8,8V36h4v-3.2C78,26.1,72.6,20.8,66,20.8z M64,67.2c0,2.2-1.8,3.8-4,3.8H36   c-2.2,0-4-1.5-4-3.8V45h32V67.2z\'/><g><path d=\'M48,62c-1.1,0-2-0.9-2-2v-2c0-1.1,0.9-2,2-2s2,0.9,2,2v2C50,61.1,49.1,62,48,62z\'/></g></g>';
   } else {
       returnString += '<path d=\'M52.7,18.9c-9-0.9-19.1,4.5-21.4,18.3l6.9,1.2c1.7-10,8.4-13.1,14-12.6c4.8,0.4,9.8,3.6,9.8,9c0,4.4-2.5,6.8-6.6,10.1  c-3.9,3.2-8.7,7.3-8.7,14.6v6.1h6.9v-6.1c0-3.9,2.3-6,6.2-9.2c4-3.4,9.1-7.7,9.1-15.5C68.8,26.3,62.1,19.7,52.7,18.9z\'/><rect x=\'46.6\' y=\'73.4\' width=\'6.9\' height=\'7.8\'/>';
   }
   returnString += '</svg>';
   return returnString;
}

var remotesCount;

function buildHtmlTable(selector, jsonVar) {
  var columns = addAllColumnHeaders(jsonVar, selector);
  var headerTbody$ = $('<tbody>');
  remotesCount = jsonVar.length;
  for (var i = 0; i < remotesCount; i++) {
    var row$ = $('<tr>');
    row$.append($('<td>').html('<input type=\'radio\' id=\'option-select_remote-' + i + '\' name=\'optionsRemotes\' onchange=\"radio(\'remote\', '+i+')\" value=\'' + i + '\'/>'));
    for (var colIndex = 0; colIndex < columns.length; colIndex++) {
      var cellValue = jsonVar[i][columns[colIndex]].toString();
      if (cellValue == null) cellValue = '';
      if (cellValue == "0,0,0") cellValue = "empty slot";
      if (colIndex == 2) {
        row$.append($('<td>').html('<input type=\'text\' id=\'name_remote-' + i + '\' value=\'' + cellValue + '\' readonly=\'\' />'));
      }
      else {
        row$.append($('<td>').html(cellValue));
      }
    }
    headerTbody$.append(row$);
  }
  $(selector).append(headerTbody$);
}

function addAllColumnHeaders(jsonVar, selector) {
  var columnSet = [];
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr>');
  headerTr$.append($('<th>').html('select'));
  
  for (var i = 0; i < jsonVar.length; i++) {
    var rowHash = jsonVar[i];
    for (var key in rowHash) {
      if ($.inArray(key, columnSet) == -1) {
        columnSet.push(key);
        headerTr$.append($('<th>').html(key));
      }
    }
  }
  headerThead$.append(headerTr$);
  $(selector).append(headerThead$);

  return columnSet;
}

//
// HTML string literals
//
var html_index = `
<div class="header"><h1>Itho CVE WiFi controller</h1></div><br><br>
<div class="pure-g">
    <div class="pure-u-1 pure-u-md-1-5"></div>
    <div class="pure-u-1 pure-u-md-3-5">
    <div>
      <div style="text-align: center">
        <div style="float: left;">
          <svg style="width:24px;height:24px" viewBox="0 0 24 24">
            <path fill="currentColor" d="M12.5,2C9.64,2 8.57,4.55 9.29,7.47L15,13.16C15.87,13.37 16.81,13.81 17.28,14.73C18.46,17.1 22.03,17 22.03,12.5C22.03,8.92 18.05,8.13 14.35,10.13C14.03,9.73 13.61,9.42 13.13,9.22C13.32,8.29 13.76,7.24 14.75,6.75C17.11,5.57 17,2 12.5,2M3.28,4L2,5.27L4.47,7.73C3.22,7.74 2,8.87 2,11.5C2,15.07 5.96,15.85 9.65,13.87C9.97,14.27 10.4,14.59 10.89,14.79C10.69,15.71 10.25,16.75 9.27,17.24C6.91,18.42 7,22 11.5,22C13.8,22 14.94,20.36 14.94,18.21L18.73,22L20,20.72L3.28,4Z"></path>
          </svg>
        </div>
        <div style="float: right;">
          <svg style="width:24px;height:24px" viewBox="0 0 24 24">
            <path fill="currentColor" d="M12,11A1,1 0 0,0 11,12A1,1 0 0,0 12,13A1,1 0 0,0 13,12A1,1 0 0,0 12,11M12.5,2C17,2 17.11,5.57 14.75,6.75C13.76,7.24 13.32,8.29 13.13,9.22C13.61,9.42 14.03,9.73 14.35,10.13C18.05,8.13 22.03,8.92 22.03,12.5C22.03,17 18.46,17.1 17.28,14.73C16.78,13.74 15.72,13.3 14.79,13.11C14.59,13.59 14.28,14 13.88,14.34C15.87,18.03 15.08,22 11.5,22C7,22 6.91,18.42 9.27,17.24C10.25,16.75 10.69,15.71 10.89,14.79C10.4,14.59 9.97,14.27 9.65,13.87C5.96,15.85 2,15.07 2,11.5C2,7 5.56,6.89 6.74,9.26C7.24,10.25 8.29,10.68 9.22,10.87C9.41,10.39 9.73,9.97 10.14,9.65C8.15,5.96 8.94,2 12.5,2Z"></path>
          </svg>
        </div>
        <div id="ithotextval">
          -
        </div>  
      </div>
      <input id="itho" type="range" min="0" max="254" value="0" class="slider" onchange="updateSlider(this.value)" style="width: 100%; margin: 0 0 2em 0;">
      <div style="text-align: center">
        <a href="#" class="pure-button" onclick="updateSlider(itho_low)" style="float: left;">Low</a> <a href="#" onclick="updateSlider(itho_medium)" class="pure-button">Medium</a> <a href="#" class="pure-button" onclick="updateSlider(itho_high)" style="float: right;">High</a>
      </div>
      <div style="text-align: center;margin: 2em 0 0 0;">
        <div id="sensor_temp"></div>
        <div id="sensor_hum"></div>
      </div>
    </div>
    </div>
    <div class="pure-u-1 pure-u-md-1-5"></div>
</div>
<script>
$(document).ready(function() {
  getSettings('sysstat');
});
</script>

`;

var html_wifisetup = `
<div class="header"><h1>Wifi setup</h1></div>
<div class="pure-g">
    <div class="pure-u-1 pure-u-md-3-5">
      <form class="pure-form pure-form-aligned" id="wifiform" action="#">
          <fieldset>
            <div class="pure-control-group">
              <label for="ssid">SSID</label>
                <input id="ssid" type="text">
            </div>
            <div class="pure-control-group">
              <label for="passwd">Password</label>
                <input id="passwd" type="Password">
            </div>
            <div class="pure-controls">
              <button id="wifisubmit" class="pure-button pure-button-primary">Save</button>
            </div>
            <br>
            <div class="pure-control-group">
              <label for="option-dhcp" class="pure-radio">Use DHCP</label> 
              <input id="option-dhcp-on" type="radio" name="option-dhcp" onchange='radio("dhcp", "on")' value="on"> on
              <input id="option-dhcp-off" type="radio" name="option-dhcp" onchange='radio("dhcp", "off")' value="off"> off
            </div>
            <div class="pure-control-group">
              <label for="renew">DHCP Renew interval</label>
                <input id="renew" type="text">
            </div>
            <div class="pure-control-group">
              <label for="ip">IP address</label>
                <input id="ip" type="text">
            </div>
            <div class="pure-control-group">
              <label for="subnet">Subnet</label>
                <input id="subnet" type="text">
            </div>
            <div class="pure-control-group">
              <label for="gateway">Gateway</label>
                <input id="gateway" type="text">
            </div>
            <div class="pure-control-group">
              <label for="dns1">DNS server 1</label>
                <input id="dns1" type="text">
            </div>
            <div class="pure-control-group">
              <label for="dns1">DNS server 2</label>
                <input id="dns2" type="text">
            </div>
          </fieldset>
      </form>
    </div>
    <div class="pure-u-1 pure-u-md-2-5">
      <div>
        <div><button id="wifiscan" class="pure-button pure-button-active">Scan</button></div>
      </div>
      <div class="hidden">
        <div><p>Scan results:</p></div>
      </div>
      <div id="wifiscanresult"></div>
    </div>
</div>
<script>
$(document).ready(function() {
  getSettings('wifisetup');
});
</script>
`;

var html_systemsettings_start = `
<div class="header"><h1>System settings</h1></div>
<p>Configuration of generic system (security) settings</p>
<style>.pure-form-aligned .pure-control-group label {width: 15em;}</style>
      <form class="pure-form pure-form-aligned">
          <fieldset id="sys_fieldset">
            <legend><br>System security:</legend>
            <div class="pure-control-group">
              <label for="sys_username">Username</label>
                <input id="sys_username" maxlength="20" type="text">
            </div>
            <div class="pure-control-group">
              <label for="sys_password">Password</label>
                <input id="sys_password" maxlength="20" type="text">
            </div>
            <div class="pure-control-group">
              <label for="option-syssec_web" class="pure-radio">Web interface authentication</label> 
              <input id="option-syssec_web-1" type="radio" name="option-syssec_web" value="1"> on
              <input id="option-syssec_web-0" type="radio" name="option-syssec_web" value="0"> off
            </div>
            <div class="pure-control-group">
              <label for="option-syssec_api" class="pure-radio">API authentication</label> 
              <input id="option-syssec_api-1" type="radio" name="option-syssec_api" value="1"> on
              <input id="option-syssec_api-0" type="radio" name="option-syssec_api" value="0"> off
            </div>
            <div class="pure-control-group">
              <label for="option-syssec_edit" class="pure-radio">File editor authentication</label> 
              <input id="option-syssec_edit-1" type="radio" name="option-syssec_edit" value="1"> on
              <input id="option-syssec_edit-0" type="radio" name="option-syssec_edit" value="0"> off
            </div>
            <legend><br>Autodetect built-in itho humidity/temp sensor (reboot needed):</legend>
            <p>The itho humidity/temp sensor is only present in the latest itho CVE series: CVE-S ECO.</p><p>Values will be posted on the MQTT sensor topic formatted as JSON.</p><p>Sensor support is limited, might be buggy and for testing only.</p>
            <br>
            <div class="pure-control-group">
              <label for="option-syssht30" class="pure-radio">Hum/Temp sensor support</label> 
              <input id="option-syssht30-1" type="radio" name="option-syssht30" value="1"> on
              <input id="option-syssht30-0" type="radio" name="option-syssht30" value="0"> off
            </div>
          </fieldset>
      </form>
<script>
$(document).ready(function() {
  getSettings('syssetup');
});
</script>
`;

var html_systemsettings_cc1101 = `
            <legend><br>Autodetect CC1101 RF module (reboot needed):</legend>
            <p>Activate the CC1101 RF module. If autodetect fails this setting will be automatically switched off again.</p>
            <div class="pure-control-group">
              <label for="option-itho_remotes" class="pure-radio">Itho RF remote support</label>
              <input id="option-itho_remotes-1" type="radio" name="option-itho_remotes_active" onchange='radio("itho_remotes", 1)' value="1"> on
              <input id="option-itho_remotes-0" type="radio" name="option-itho_remotes_active" onchange='radio("itho_remotes", 0)' value="0"> off
            </div>
            <br>
`;

var html_systemsettings_end = `
            <div class="pure-controls">
              <button id="syssumbit" class="pure-button pure-button-primary">Save</button>
            </div>
`;


var html_ithosettings = `
<div class="header"><h1>Itho settings</h1></div>
<p>Configuration of Itho unit speed and timer values</p>
<style>.pure-form-aligned .pure-control-group label {width: 15em;}</style>
      <form class="pure-form pure-form-aligned">
          <fieldset>
            <legend><br>Speed settings (0-254):</legend>
            <div class="pure-control-group">
              <label for="itho_fallback">Start/fallback speed</label>
                <input id="itho_fallback" type="number" min="0" max="254" size="6">
            </div>
            <div class="pure-control-group">
              <label for="itho_low">Low</label>
                <input id="itho_low" type="number" min="0" max="254" size="6">
            </div>
            <div class="pure-control-group">
              <label for="itho_medium">Medium</label>
                <input id="itho_medium" type="number" min="0" max="254" size="6">
            </div>
            <div class="pure-control-group">
              <label for="itho_high">High</label>
                <input id="itho_high" type="number" min="0" max="254" size="6">
            </div>
            <legend><br>Timer settings (0-65535 minutes):</legend>
            <div class="pure-control-group">
              <label for="itho_timer1">Timer1</label>
                <input id="itho_timer1" type="number" min="0" max="65535" size="6">
            </div>
            <div class="pure-control-group">
              <label for="itho_timer2">Timer2</label>
                <input id="itho_timer2" type="number" min="0" max="65535" size="6">
            </div>
            <div class="pure-control-group">
              <label for="itho_timer3">Timer3</label>
                <input id="itho_timer3" type="number" min="0" max="65535" size="6">
            </div>
            <legend><br>Virtual remote settings:</legend>
            <p>The add-on can present itself as a virtual remote that can be joined to the itho unit.</p>
            <p>This virtual remote can be used to force the itho unit in medium mode before sending a command from the add-on. This way the add-on can overrule the current speed settings of the itho (ie. due to active input from a built in humidity sensor or another remote)</p>
            <p>A join command will only be accepted by the itho unit after a power cycle.</p>
            <div class="pure-control-group">
              <label for="option-vremotejoin" class="pure-radio">Send join command</label>
              <input id="option-vremotejoin-2" type="radio" name="option-vremotejoin_active" value="2"> every power on
              <input id="option-vremotejoin-1" type="radio" name="option-vremotejoin_active" value="1"> next power on
              <input id="option-vremotejoin-0" type="radio" name="option-vremotejoin_active" value="0"> off
            </div>
            <div class="pure-control-group">
              <label for="option-vremotemedium" class="pure-radio">Force medium mode</label>
              <input id="option-vremotemedium-1" type="radio" name="option-vremotemedium_active" value="1"> on
              <input id="option-vremotemedium-0" type="radio" name="option-vremotemedium_active" value="0"> off
            </div>
            <div class="pure-control-group hidden">
              <label for="option-vremoteapi" class="pure-radio">API uses virtual remote</label>
              <input id="option-vremoteapi-1" type="radio" name="option-vremoteapi_active" value="1"> on
              <input id="option-vremoteapi-0" type="radio" name="option-vremoteapi_active" value="0"> off
            </div>
            <div class="pure-control-group">
              <label for="option-vremoteswap" class="pure-radio">Swap high/low</label>
              <input id="option-vremoteswap-1" type="radio" name="option-vremoteswap_active" value="1"> on
              <input id="option-vremoteswap-0" type="radio" name="option-vremoteswap_active" value="0"> off
            </div>
            <br>
            <div class="pure-controls">
              <button id="ithosubmit" class="pure-button pure-button-primary">Save</button>
            </div>            
          </fieldset>
      </form>
<script>
$(document).ready(function() {
  getSettings('ithosetup');
});
</script>
`;

var html_autopilotsettings = `
<div class="header"><h1>Auto-Pilot settings</h1></div>
<p>Configuration of auto-pilot threshold values</p>
<style>.pure-form-aligned .pure-control-group label {width: 15em;}</style>
      <form class="pure-form pure-form-aligned">
          <fieldset>
            <div class="pure-control-group">
              <label for="option-autopilot_active" class="pure-radio">Auto-Pilot</label> 
              <input id="option-autopilot_active-1" type="radio" name="option-autopilot_active" onchange='radio("autopilot_active", 1)' value="1"> on
              <input id="option-autopilot_active-0" type="radio" name="option-autopilot_active" onchange='radio("autopilot_active", 0)' value="0"> off
            </div>
            <legend><br>Thresholds:</legend>
            <div class="pure-control-group">
              <label for="autopilot_hum_upper">Humidity Upper Limit (switch on)</label>
                <input id="autopilot_hum_upper" type="number" min="0" max="99" size="6">
            </div>
            <div class="pure-control-group">
              <label for="autopilot_hum_lower">Humidity Lower Limit (switch off)</label>
                <input id="autopilot_hum_lower" type="number" min="0" max="99" size="6">
            </div>
            <div class="pure-controls">
              <button id="autopilotsubmit" class="pure-button pure-button-primary">Save</button>
            </div>            
          </fieldset>
      </form>
<script>
$(document).ready(function() {
  getSettings('autopilotsetup');
});
</script>
`;

var html_remotessetup = `
<div class="header"><h1>RF Remotes setup</h1></div>
<style>.pure-form-aligned .pure-control-group label {width: 15em;}</style>
      <form class="pure-form pure-form-aligned">
          <fieldset>
              <br><br>
              <div class="pure-control-group">
                <label for="mqtt_conn">Learn/Leave mode</label>
                  <button id="itho_llm" class="pure-button">Unknown</button>
              </div>
          </fieldset>
          <fieldset>
              <br>
                <legend><br>RF remotes:</legend>
              <br>
              <table id="RemotesTable" class="pure-table pure-table-bordered"></table>
              <div class="pure-control-group">
                <button id="itho_update_remote" class="pure-button">Update</button>&nbsp;<button id="itho_remove_remote" class="pure-button">Remove</button>
              </div>
          </fieldset>
      </form>
<script>
$(document).ready(function() {
  getSettings('ithoremotes');
});
</script>
`;

var html_mqttsetup = `
<div class="header"><h1>MQTT setup</h1></div>
<p>Configuration of the MQTT server to publish the status to and subscribe topic to receive commands</p>
<style>.pure-form-aligned .pure-control-group label {width: 15em;}</style>
      <form class="pure-form pure-form-aligned">
          <fieldset>
            <div class="pure-control-group">
              <label>MQTT Status</label>
                <button id="mqtt_conn" class="pure-button" style="pointer-events:none;">Unknown</button>
            </div>
            <br>
            <div class="pure-control-group">
              <label for="option-mqtt_active" class="pure-radio">MQTT Active</label> 
              <input id="option-mqtt_active-1" type="radio" name="option-mqtt_active" onchange='radio("mqtt_active", 1)' value="1"> on
              <input id="option-mqtt_active-0" type="radio" name="option-mqtt_active" onchange='radio("mqtt_active", 0)' value="0"> off
            </div>
            <br>
            <div class="pure-control-group">
              <label for="mqtt_serverName">Server</label>
                <input id="mqtt_serverName" maxlength="63" type="text">
            </div>
            <div class="pure-control-group">
              <label for="mqtt_username">Username</label>
                <input id="mqtt_username" maxlength="30" type="text">
            </div>
            <div class="pure-control-group">
              <label for="mqtt_password">Password</label>
                <input id="mqtt_password" maxlength="30" type="Password">
            </div>              
            <div class="pure-control-group">
              <label for="mqtt_port">Port</label>
                <input id="mqtt_port" maxlength="5" type="text">
            </div>
            <div class="pure-control-group">
              <label for="mqtt_state_topic">State topic</label>
                <input id="mqtt_state_topic" maxlength="120" type="text">
            </div>
            <div class="pure-control-group">
              <label id="label-mqtt_sensor" for="mqtt_sensor_topic">Sensor topic</label>
                <input id="mqtt_sensor_topic" maxlength="120" type="text">
            </div>
            <div class="pure-control-group">
              <label for="mqtt_cmd_topic">Command topic</label>
                <input id="mqtt_cmd_topic" maxlength="120" type="text">
            </div>
            <div class="pure-control-group">
              <label id="label-lwt_topic" for="mqtt_lwt_topic">Last will topic</label>
                <input id="mqtt_lwt_topic" maxlength="120" type="text">
            </div>
            <div class="pure-control-group">
              <label for="option-mqtt_ha" class="pure-radio">Home Assistant MQTT Discovery</label> 
              <input id="option-mqtt_ha-1" type="radio" name="option-mqtt_ha_active" onchange='radio("mqtt_ha", 1)' value="1"> on
              <input id="option-mqtt_ha-0" type="radio" name="option-mqtt_ha_active" onchange='radio("mqtt_ha", 0)' value="0"> off
            </div>
            <div class="pure-control-group">
              <label id="label-mqtt_ha" for="mqtt_ha_topic">Home Assistant Discovery topic prefix</label>
                <input id="mqtt_ha_topic" maxlength="120" type="text">
            </div>
            <br>
            <div class="pure-control-group">
              <label for="option-mqtt_domoticz" class="pure-radio">Domoticz MQTT</label> 
              <input id="option-mqtt_domoticz-1" type="radio" name="option-mqtt_domoticz_active" onchange='radio("mqtt_domoticz", 1)' value="1"> on
              <input id="option-mqtt_domoticz-0" type="radio" name="option-mqtt_domoticz_active" onchange='radio("mqtt_domoticz", 0)' value="0"> off
            </div>
            <div class="pure-control-group">
              <label id="label-mqtt_idx" for="mqtt_idx" style="display: none;">Device IDX</label>
                <input id="mqtt_idx" maxlength="5" type="text" style="display: none;">
            </div>
            <div class="pure-control-group">
              <label id="label-sensor_idx" for="sensor_idx" style="display: none;">Sensor IDX</label>
                <input id="sensor_idx" maxlength="5" type="text" style="display: none;">
            </div>            
            <br>
            <div class="pure-controls">
              <button id="mqttsubmit" class="pure-button pure-button-primary">Save</button>
            </div>
          </fieldset>
      </form>
<script>
$(document).ready(function() {
  getSettings('mqttsetup');
});
</script>
`;


var html_api = `
<div class="header"><h1>IthoWifi - API</h1></div>
<h3>API Description</h3>
<strong>General information HTML API</strong><br><br>
A simple HTML API is available at the following URL: <a href='api.html' target='_blank'>api.html</a><br><br>
The request should be formatted as follows: <br>http://[DNS or IP]/api.html?[param]=[value]<br><br>
ie. http://192.168.4.1/api.html?command=medium<br>
or<br>
http://192.168.4.1/api.html?speed=150&timer=15<br><br>
Unless specified otherwise:<br><ul>
<li>A successful command will return 'OK', an unsuccessful command will return 'NOK'</li>
<li>String params/values are supplied without quote marks</li>
<li>Values outside specified values/ranges will be ignored or 0 in case of an overflow</li>
</ul>
<br>
<strong>General information MQTT API</strong><br><br>
Unless specified otherwise:<br><ul>
<li>The command must be sent as valid JSON</li>
<li>The command must be sent to the command topic</li>
<li>Values outside specified key/value ranges will be ignored or 0 in case of an overflow</li>
<li>String values must be supplied with quote marks in accordance with JSON standards</li>
</ul>
<br>
<strong>API table:</strong>
<table class="pure-table pure-table-bordered" style="font-size:.85em"><thead><tr><th>key or param</th><th>datatype</th><th style="width:160px">value</th><th>datatype</th><th style="text-align:center">MQTT<br>(JSON)</th><th style="text-align:center">HTML<br>(URL params)</th></tr></thead>
<tbody><tr><td>dtype</td><td>string</td><td>ithofan</td><td>string</td><td style="text-align:center">●</td><td style="text-align:center">◌</td></tr><tr><td colspan="6">Comments:<br><em>If Domoticz MQTT support is on and commands originate from other than configured IDX, this key/value pair needs to be present for commands to get processed.</em></td></tr>
<tr><td>username</td><td>string</td><td>max 20 chars long</td><td>string</td><td style="text-align:center">◌</td><td style="text-align:center">●</td></tr><tr><td>password</td><td>string</td><td>max 20 chars long</td><td>string</td><td style="text-align:center">◌</td><td style="text-align:center">●</td></tr>
<tr><td>command</td><td>string</td><td>low, medium, high, timer1, timer2, timer3, clearqueue</td><td>string</td><td style="text-align:center">●</td><td style="text-align:center">●</td></tr><tr><td colspan="6">Comments:<br>
<em>Resulting speed/timer settings are configurable. Value without timer sets the base/fallback speed of the fan. Timers will be queued on highest speed setting first for the duration of the timer.<br>For MQTT; sending only the value instead of a JSON key/value pair also works for single commands</em></td></tr>
<tr><td>speed</td><td>string</td><td>0-254</td><td>uint8_t</td><td style="text-align:center">●</td><td style="text-align:center">●</td></tr><tr><td colspan="6">Comments:<br><em>Speed without a timer will reset the queue (different behaviour configurable) and set a new base/fallback speed.<br>
For MQTT; sending only the value instead of a JSON key/value pair also works for single commands</em></td></tr><tr><td>timer</td><td>string</td><td>0-65535</td><td>uint16_t</td><td style="text-align:center">●</td><td style="text-align:center">●</td></tr><tr><td colspan="6">Comments:<br>
<em>only effective with "command" or "speed" key/param present, could overrule timer value of timer1, timer2, timer3. Highest speed setting on the queue will be active for the duration of the timer.</em></td></tr><tr><td>clearqueue</td><td>string</td><td>true</td><td>string</td>
<td style="text-align:center">●</td><td style="text-align:center">◌</td></tr><tr><td colspan="6">Comments:<br><em>Clear all timers on the queue, scheduled to run after all other commands have been processed. Speed will fallback to last value before items got enqueued</em></td></tr><tr><td>get</td>
<td>string</td><td>currentspeed</td><td>string</td><td style="text-align:center">◌</td><td style="text-align:center">●</td></tr><tr><td colspan="6">Comments:<br><em>Returns current active itho speed setting in range 0-254</em></td></tr><tr><td></td><td></td><td>0-254</td><td>uint8_t</td>
<td style="text-align:center">●</td><td style="text-align:center">◌</td></tr><tr><td colspan="6">Comments:<br><em>Return type present on MQTT "State topic"</em></td></tr></tbody></table><p><br><br></p>
`;

var html_help = `
<div class="header"><h1>Need some help?</h1></div>
<br><br><span>More information and contact options are available at GitHub: <a href='https://github.com/arjenhiemstra/ithowifi' target='_blank'>https://github.com/arjenhiemstra/ithowifi</a></span>  
`;

var html_reboot_script = `
<p id="time">This page will reload to the start page in... </p>
<script>
$(document).ready(function() {
  startCountdown();
});
</script>
`;

var html_edit = `
<div class="header"><h1>File editor</h1></div>
<p>Be very carefull, use only if absolutely necessary!</p><br>
<iframe id="editor" src="/edit" width="100%" height="100%" style="border:none;padding:5px"></iframe>
<script>
$(document).ready(function() {
  $('#main').css('max-width', '1200px')
});
</script>
`;

var html_reset = `
<div class="header"><h1>Restart/Restore device</h1></div><br><br>
<form class="pure-form">
  <fieldset>
    <button id="reboot" class="pure-button">Restart device</button>
    <button id="resetwificonf" class="pure-button">Restore wifi config</button>
    <button id="resetsysconf" class="pure-button">Restore system config</button>

    <div id="rebootscript"></div>
  </fieldset>
</form>    
`;

var html_update = `
<div class="header"><h1>Update firmware</h1></div>
<br>
<div class="pure-control-group">
    <label>Current firmware version:</label>
    <label id="firmware_ver">unknown</label>
</div>
<br>
<div class="pure-control-group">
    <label for="hardware_rev">Hardware revision:</label>
    <label id="hardware_rev">unknown</label>
</div>
<br>
<div class="pure-control-group">
    <label for="latest_fw">Latest firmware version:</label>
    <label id="latest_fw">unknown</label><br>
    <a href="" id="latest_fw_button" class="pure-button pure-button-primary hidden">Download firmware file</a>
</div>
<form class="pure-form pure-form-stacked" method='POST' action='#' enctype='multipart/form-data' id='updateform'>
  <fieldset>
    <p>Update the firmware of your device</p>
    <input type='file' name='update'><br>
    <button id="updatesubmit" class="pure-button pure-button-primary">Update</button>
  </fieldset>
  <p id="uploadprg" style="display: none;">File upload progress: 0%</p>
  <div id="uploadProgress" style="border-radius: 20px;max-width: 300px;background-color: #ccc;display: none;">
    <div id="uploadBar" style="border-radius: 20px;width: 10%;height: 20px;background-color: #999;"></div>
  </div>
  <div id="uploaddone" style="display: none;"></div>
  <p id="updateprg" style="display: none;">Firmware update progress: 0%</p>
  <div id="updateProgress" style="border-radius: 20px;max-width: 300px;background-color: #ccc;display: none;">
    <div id="updateBar" style="border-radius: 20px;width: 10%;height: 20px;background-color: #999;"></div>    
  </div>
  <div id="updatedone" style="display: none;"></div>
  <p id="time" style="display: none;">This page will reload to the start page in... </p>
</form>
<script>
$('#firmware_ver').text(fw_version);
$('#hardware_rev').text(hw_revision);
function process(key,value) {
    if (key == hw_revision) {    
      let latest_fw = value.latest_fw; 
      let download_link = value.link;       
      if (latest_fw == fw_version) {
        $('#latest_fw').text(' firmware is up-to-date');
      }
      else {
        $('#latest_fw').text(latest_fw);
        $('#latest_fw_button').removeClass('hidden');
        $('#latest_fw_button').attr("href", download_link);
      }
    }   
}

function traverse(o,func) {
    for (var i in o) {
        func.apply(this,[i,o[i]]);  
        if (o[i] !== null && typeof(o[i])=='object') {
            traverse(o[i],func);
        }
    }
}

$.ajax({
  type: 'GET',
  url: 'https://raw.githubusercontent.com/arjenhiemstra/ithowifi/master/compiled_firmware_files/firmware.json',
  dataType: 'json',
  timeout: 3000,
  success: function(data){
    traverse(data,process);
  },
  error: function(xhr, type){
    if (on_ap) {
      $('#latest_fw').text(' firmware check not possible on Access Point mode');
    }
    else {
      $('#latest_fw').text(' firmware check failed, no internet connection?');
    }
    
  }
})

</script>
`;


)=====";
