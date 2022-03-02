

var count = 0;
var itho_low = 0;
var itho_medium = 127;
var itho_high = 254;
var sensor = -1;

sessionStorage.setItem("statustimer", 0);

var websocketServerLocation = 'ws://' + window.location.hostname + ':8000/ws';


function startWebsock(websocketServerLocation){
  websock = new WebSocket(websocketServerLocation);
  websock.onmessage = function(b) {
          console.log(b);
          var f = JSON.parse(b.data);
          var g = document.body;
          if (f.wifisettings) {
            let x = f.wifisettings;
            processElements(x);
          }
          else if (f.messagebox) {
            let x = f.messagebox;
            count += 1;
            resetTimer();
            $('#message_box').show();
            $('#message_box').append(`<p class='messageP' id='mbox_p${count}'>Message: ${x.message}</p>`);
            removeAfter5secs(count);
          }
          else if (f.rflog) {
            let x = f.rflog;
            $('#rflog_outer').removeClass('hidden');
            var d = new Date();
            $('#rflog').prepend(`${d.toLocaleString('nl-NL')}: ${x.message}<br>`);
          }
          else if(f.sysmessage) {
            let x = f.sysmessage;
            $(`#${x.id}`).text(x.message);
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
          port:     $('#port').val(),
          hostname: $('#hostname').val(),
          ntpserver:$('#ntpserver').val()
        }
      }));
      update_page('wifisetup');
    }
    else if ($(this).attr('id') == 'reboot') {
      if (confirm("This will reboot the device, are you sure?")) {
        $('#rebootscript').append(html_reboot_script);
        websock.send('{\"reboot\":true,\"dontsaveconf\":'+document.getElementById("dontsaveconf").checked+'}');
      }
    }
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
    $(`#${id}`).remove();
}

function processElements(x) {
  for (var key in x) {
    if (x.hasOwnProperty(key)) {
      var el = $(`#${key}`);
      if(el.is('input')) {
        $(`#${key}`).val(x[key]);
      }
      else {
        var radios = $(`input[name='option-${key}']`);
        if(radios[1]) {
          if(radios.is(':checked') === false) {
              radios.filter(`[value='${x[key]}']`).prop('checked', true);
          }
          radio(key, x[key]);
        }
      }
    }
  }
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
}

function timeoutPromise(timer) {
  return new Promise(resolve => setTimeout(resolve, timer));
}

function getSettings(pagevalue) {
  if(websock.readyState === 1 ){
    websock.send('{\"' + pagevalue + '\":1}');
  }
  else {
    console.log("websock not open");
    setTimeout(getSettings, 1000, pagevalue);
  }
}


//function to load html main content
var lastPageReq = "";
function update_page(page) {
    lastPageReq = page;
    if(page != 'status') {
      sessionStorage.setItem("statustimer", 0);
    }
    $('#main').empty();
    $('#main').css('max-width', '768px')
    if (page == 'index') { $('#main').append(html_index); }
    if (page == 'wifisetup') { $('#main').append(html_wifisetup); }
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
      <input id="ithoslider" type="range" min="0" max="254" value="0" class="slider" style="width: 100%; margin: 0 0 2em 0;">
      <div style="text-align: center">
        <button id="itho_low" class="pure-button" style="float: left;">Low</button><button id="itho_medium" class="pure-button">Medium</button><button id="itho_high" class="pure-button" style="float: right;">High</button>
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
  var slide = document.getElementById("ithoslider");
  if(!!slide) {
    slide.addEventListener("click", function() {
      updateSlider(this.value);
    });
  }
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
              <label for="hostname">Hostname</label>
                <input id="hostname" type="text">
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
            <div class="pure-control-group">
              <label for="ntpserver">NTP server</label>
                <input id="ntpserver" type="text">
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
      <br><br><br>
    </div>
</div>
<script>
$(document).ready(function() {
  getSettings('wifisetup');
});
</script>
`;



