

const char controls_js[] PROGMEM = R"=====(

var count = 0;
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
          else if (f.mqttsettings) {
            let x = f.mqttsettings;
            var $radios = $('input[name=\'option-mqtt_active\']');
            if($radios.is(':checked') === false) {
                $radios.filter('[value=' + x.mqtt_active + ']').prop('checked', true);
            }
            radio("mqtt_active", x.mqtt_active);
            mqtt_serverName:   $('#mqtt_serverName').val(x.mqtt_serverName);
            mqtt_username:     $('#mqtt_username').val(x.mqtt_username);
            mqtt_password:     $('#mqtt_password').val(x.mqtt_password);
            mqtt_port:         $('#mqtt_port').val(x.mqtt_port);
            mqtt_state_topic:  $('#mqtt_state_topic').val(x.mqtt_state_topic);
            mqtt_cmd_topic:    $('#mqtt_cmd_topic').val(x.mqtt_cmd_topic);
          }          
          else if (f.wifiscanresult) {
            let x = f.wifiscanresult;
            $('#wifiscanresult').append('<div class=\'ScanResults\'><input id=\'' + x.id + '\' class=\'pure-input-1-5\' name=\'optionsWifi\' value=\'' + x.ssid + '\' type=\'radio\'>' + returnSignalSVG(x.sigval) + '' + returnWifiSecSVG(x.sec) + ' ' + x.ssid + '</label></div>');
          }
          else if (f.systemstat) {
            let x = f.systemstat;
            $('#memory_box').show();
            $('#memory_box').html('<p><b>Memory:</b><p><p>free: <b>' + x.freemem + '</b></p><p>low: <b>' + x.memlow + '</b></p>');
            $('#mqtt_conn').removeClass();
            var button = returnMqttState(x.mqqtstatus);
            $('#mqtt_conn').addClass("pure-button " + button.button);
            $('#mqtt_conn').text(button.state);
            $('#itho').val(x.itho);
            var i2cstatus = '';
            if (x.i2cstat == 'nok') {
              i2cstatus = '<span style="color:#ca3c3c;">init failed</span><br>- please power cycle the itho unit -';
            }
            else if (x.i2cstat == 'initok') {
              i2cstatus = '<span style="color:#1cb841;">connected</span>';
            }
            else {
              i2cstatus = 'unknown status';
            }
            $('#i2cstat').html(i2cstatus);
            
          }  
          else if (f.messagebox) {
            let x = f.messagebox;
            count += 1;
            resetTimer();
            $('#message_box').show();
            $('#message_box').append('<p class=\'messageP\' id=\'mbox_p' + count + '\'>Message: ' + x.message1 + ' ' + x.message2 + '</p>');
            removeAfter5secs(count);
          }
          else if (f.ota) {
            let x = f.ota;
            $('#updateprg').html('Firmware update progress: ' + x.percent + '%');
            moveBar(x.percent, "updateBar");    
          }
  };
  websock.onopen = function(a) {
      console.log('websock open');
  };
  websock.onclose = function(a) {
      console.log('websock close');
      // Try to reconnect in 5 seconds
      websock = null;
      setTimeout(function(){startWebsock(websocketServerLocation)}, 5000);      
  };
  websock.onerror = function(a) {
      console.log(a);
  };    
}


$(document).ready(function() {
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
          password: $('#passwd').val(),
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
    //mqttmsubmit
   else if ($(this).attr('id') == 'mqttsubmit') {
      websock.send(JSON.stringify({
        mqttsettings: {
          mqtt_active:       $('input[name=\'option-mqtt_active\']:checked').val(),
          mqtt_serverName:   $('#mqtt_serverName').val(),
          mqtt_username:     $('#mqtt_username').val(),
          mqtt_password:     $('#mqtt_password').val(),
          mqtt_port:         $('#mqtt_port').val(),
          mqtt_version:      $('#mqtt_version').val(),
          mqtt_state_topic:  $('#mqtt_state_topic').val(),
          mqtt_cmd_topic:    $('#mqtt_cmd_topic').val()
        }
      }));
      update_page('mqtt');
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
    else if ($(this).attr('id') == 'updatesubmit') {
      e.preventDefault();
      $('#uploadProgress').show();
      $('#updateProgress').show();
      $('#uploadprg').show();
      $('#updateprg').show();
        var form = $('#updateform')[0];
        var data = new FormData(form);
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
                          console.log("evt: ");
                          console.log(evt);
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
              error: function (a, b, c) {
                console.log('failed!')
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

function radio(origin, state) {
  if (origin == "dhcp") {
    if (on_ap == true) {
      $('#renew, #ip, #subnet, #gateway, #dns1, #dns2, #port').prop('readonly', true);
      $('#option-dhcp-on, #option-dhcp-off').prop('disabled', true);
    }
    else if (state == 'on') {
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
    if (state == 'on') {
      $('#mqtt_serverName, #mqtt_username, #mqtt_password, #mqtt_port, #mqtt_state_topic, #mqtt_cmd_topic').prop('readonly', false);
    }
    else {
      $('#mqtt_serverName, #mqtt_username, #mqtt_password, #mqtt_port, #mqtt_state_topic, #mqtt_cmd_topic').prop('readonly', true);
    }     
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
  websock.send(JSON.stringify({'itho':val}));
}

//function to load html main content
function update_page(page) {
    $('#main').empty();

    if (page == 'index') { $('#main').append(html_index); }
    if (page == 'wifisetup') { $('#main').append(html_wifisetup); }
    if (page == 'mqtt') { $('#main').append(html_mqttsetup); }
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
      // The className is not found
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
      console.log(e + " menu link clicked, link: " + $(this).attr('href'));
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
  var states = [ "Disabled", "Connection Timeout" ,"Connection Lost" ,"Connection Failed" ,"Disconnected" ,"Connected" ,"MQTT version unsupported" ,"Client ID rejected" ,"Server Unavailable" ,"Bad Credentials" ,"Client Unauthorized"]
  var button = "";

  if (state == 0) {
    button = ""
  }
  else if (state < 5) {
    button = "button-error"
  }
  else if (state > 5) {
     button = "button-warning"
  }
  else {
    button = "button-success"
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


//
// HTML string literals
//
var html_index = `

<div class="header">
  <h1>Itho CVE WiFi controller</h1>
</div>
<br><br>
<div class="pure-g">
    <div class="pure-u-1 pure-u-md-1-5"></div>
    <div class="pure-u-1 pure-u-md-3-5">
    
    <div>
      <div style="float: right;">
        <svg style="width:24px;height:24px" viewBox="0 0 24 24">
          <path fill="currentColor" d="M12,11A1,1 0 0,0 11,12A1,1 0 0,0 12,13A1,1 0 0,0 13,12A1,1 0 0,0 12,11M12.5,2C17,2 17.11,5.57 14.75,6.75C13.76,7.24 13.32,8.29 13.13,9.22C13.61,9.42 14.03,9.73 14.35,10.13C18.05,8.13 22.03,8.92 22.03,12.5C22.03,17 18.46,17.1 17.28,14.73C16.78,13.74 15.72,13.3 14.79,13.11C14.59,13.59 14.28,14 13.88,14.34C15.87,18.03 15.08,22 11.5,22C7,22 6.91,18.42 9.27,17.24C10.25,16.75 10.69,15.71 10.89,14.79C10.4,14.59 9.97,14.27 9.65,13.87C5.96,15.85 2,15.07 2,11.5C2,7 5.56,6.89 6.74,9.26C7.24,10.25 8.29,10.68 9.22,10.87C9.41,10.39 9.73,9.97 10.14,9.65C8.15,5.96 8.94,2 12.5,2Z"></path>
        </svg>
      </div>
      <div style="float: left;">
        <svg style="width:24px;height:24px" viewBox="0 0 24 24">
          <path fill="currentColor" d="M12.5,2C9.64,2 8.57,4.55 9.29,7.47L15,13.16C15.87,13.37 16.81,13.81 17.28,14.73C18.46,17.1 22.03,17 22.03,12.5C22.03,8.92 18.05,8.13 14.35,10.13C14.03,9.73 13.61,9.42 13.13,9.22C13.32,8.29 13.76,7.24 14.75,6.75C17.11,5.57 17,2 12.5,2M3.28,4L2,5.27L4.47,7.73C3.22,7.74 2,8.87 2,11.5C2,15.07 5.96,15.85 9.65,13.87C9.97,14.27 10.4,14.59 10.89,14.79C10.69,15.71 10.25,16.75 9.27,17.24C6.91,18.42 7,22 11.5,22C13.8,22 14.94,20.36 14.94,18.21L18.73,22L20,20.72L3.28,4Z"></path>
        </svg>
      </div>
      <input id="itho" type="range" min="0" max="254" value="0" class="slider" onchange="updateSlider(this.value)" style="width: 100%; margin: 0 0 2em 0;">

      <div style="text-align: center">
        <a href="#" class="pure-button" onclick="updateSlider(0)" style="float: left;">Low</a> <a href="#" onclick="updateSlider(127)" class="pure-button">Set to 50%</a> <a href="#" class="pure-button" onclick="updateSlider(254)" style="float: right;">High</a>
      </div>

    </div>
  <div style="position:fixed;z-index:1001;padding:10px;width:320px;text-align:center;left:50%;margin-left:-160px;bottom:0;"><span>Itho I2C connection status: </span><span id="i2cstat">unknown</span></div>    
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
<div class="header">
  <h1>Wifi setup</h1>
</div>

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
        <fieldset style="border-color: white;">
        <div><button id="wifiscan" class="pure-button pure-button-active">Scan</button></div>
        </fieldset>
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
var html_mqttsetup = `
<div class="header">
  <h1>MQTT setup</h1>
</div>

<p>Configuration of the MQTT server to publish the status to and subscribe topic to receive commands</p>

<style>.pure-form-aligned .pure-control-group label {width: 15em;}</style>
      <form class="pure-form pure-form-aligned">
          <fieldset>
            <div class="pure-control-group">
              <label for="mqtt_conn">MQTT Status</label>
                <button id="mqtt_conn" class="pure-button" style="pointer-events:none;">Unknown</button>
            </div>
            <br>  
            <div class="pure-control-group">        
              <label for="option-mqtt_active" class="pure-radio">MQTT Active</label> 
              <input id="option-mqtt_active-on" type="radio" name="option-mqtt_active" onchange='radio("mqtt_active", "on")' value="on"> on
              <input id="option-mqtt_active-off" type="radio" name="option-mqtt_active" onchange='radio("mqtt_active", "off")' value="off"> off
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
              <label for="mqtt_cmd_topic">Command topic</label>
                <input id="mqtt_cmd_topic" maxlength="120" type="text">
            </div>            
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
var html_help = `
  <div class="header">
    <h1>Need some help?</h1>
  </div>
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

var html_reset = `
  <div class="header">
    <h1>Restart/Restore device</h1>
  </div>
  <br><br>
<form class="pure-form">
  <fieldset>
    <button id="reboot" class="pure-button">Restart device</button>
    <button id="resetwificonf" class="pure-button">Restore wifi config</button>
    <button id="resetsysconf" class="pure-button">Restore system config</button>

    <div id="rebootscript">

    </div>
  
  </fieldset>
</form>    

`;

var html_update = `
  <div class="header">
    <h1>Update firmware</h1>
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

`;


)=====";


void controls_js_code(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", controls_js);
  response->addHeader("Server","Project WiFi Web Server");
  request->send(response);
}
