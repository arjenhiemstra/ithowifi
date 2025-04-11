
var debug = false;
var count = 0;
var itho_low = 0;
var itho_medium = 127;
var itho_high = 255;
var sensor = -1;
var uuid = 0;
var wifistat_to;
var statustimer_to;
var saved_status_count = 0;
var current_status_count = 0;
localStorage.setItem("statustimer", 0);
localStorage.setItem("statustimer", 0);
var settingIndex = -1;

var websocketServerLocation = location.protocol.indexOf("https") > -1 ? 'wss://' + window.location.hostname + ':8000/ws' : 'ws://' + window.location.hostname + ':8000/ws';

let messageQueue = [];
let connectTimeout, pingTimeout;
let websock;

function startWebsock(websocketServerLocation) {
  messageQueue = [];
  clearTimeout(connectTimeout);
  clearTimeout(pingTimeout);
  pingTimeout = !1;
  connectTimeout = setTimeout((() => {
    if (debug) console.log("websock connect timeout."),
      //websock.close(),
      startWebsock(websocketServerLocation);
  }), 1000);
  websock = null;
  websock = new WebSocket(websocketServerLocation);

  websock.addEventListener('open', function (event) {
    if (debug) console.log('websock open');
    clearTimeout(connectTimeout);
    document.getElementById("layout").style.opacity = 1;
    document.getElementById("loader").style.display = "none";
    if (lastPageReq !== "") {
      update_page(lastPageReq);
    }
    getSettings('syssetup');
  });
  websock.addEventListener('message', function (event) {
    "pong" == event.data ? (clearTimeout(pingTimeout),
      pingTimeout = !1) : messageQueue.push(event.data);
  });
  websock.addEventListener('close', function (event) {
    if (debug) console.log('websock close');
    document.getElementById("layout").style.opacity = 0.3;
    document.getElementById("loader").style.display = "block";
    // setTimeout(startWebsock, 200, websocketServerLocation);
  });
  websock.addEventListener('error', function (event) {
    if (debug) console.log("websock Error!", event);
    startWebsock(websocketServerLocation);
    //websock.close();
  });
  setInterval((() => {
    pingTimeout || websock.readyState != WebSocket.OPEN || (pingTimeout = setTimeout((() => {
      if (debug) console.log("websock ping timeout.");
      startWebsock(websocketServerLocation);
    }
    ), 6e4),
      websock_send("ping"))
  }
  ), 5.5e4)
}

function websock_send(message) {
  if (websock.readyState === 1) {
    if (debug) console.log(message);
    websock.send(message);
  }
  else {
    if (debug) console.log("websock.readyState != open");
  }
}

(async function processMessages() {
  while (messageQueue.length > 0) {
    const message = messageQueue.shift();
    processMessage(message);
  }
  await new Promise(resolve => setTimeout(resolve, 0));
  processMessages();
})();

function processMessage(message) {
  if (debug) console.log(message);
  let f;
  try {
    f = JSON.parse(decodeURIComponent(message));
  } catch (error) {
    f = JSON.parse(message);
  }
  let g = document.body;
  if (f.wifisettings) {
    let x = f.wifisettings;
    processElements(x);
  }
  else if (f.logsettings) {
    let x = f.logsettings;
    processElements(x);
    if (x.rfloglevel > 0) $('#rflog_outer').removeClass('hidden');
  }
  else if (f.wifistat) {
    let x = f.wifistat;
    processElements(x);
  }
  else if (f.debuginfo) {
    let x = f.debuginfo;
    processElements(x);
  }
  else if (f.systemsettings) {
    let x = f.systemsettings;
    processElements(x);
    if ("itho_rf_support" in x) {
      if (x.itho_rf_support == 1 && x.rfInitOK == false) {
        if (confirm("For changes to take effect click 'Ok' to reboot")) {
          $('#main').empty();
          $('#main').append("<br><br><br><br>");
          $('#main').append(html_reboot_script);
          websock_send('{"reboot":true}');
        }
      }
      if (x.itho_rf_support == 1 && x.rfInitOK == true) $('#remotemenu').removeClass('hidden');
      else $('#remotemenu').addClass('hidden');
    }
    if (x.mqtt_ha_active == 1) $('#hadiscmenu').removeClass('hidden');
    else $('#hadiscmenu').addClass('hidden');
    if ("i2cmenu" in x) {
      if (x.i2cmenu == 1) {
        $('#i2cmenu').removeClass('hidden');
      }
      else {
        $('#i2cmenu').addClass('hidden');
      }
    }
    if ("api_version" in x) {
      localStorage.setItem("api_version", x.api_version);
    }
  }
  else if (f.remotes) {
    let x = f.remotes;
    let remfunc = f.remfunc;
    $('#RemotesTable').empty();
    buildHtmlTableRemotes('#RemotesTable', remfunc, x);
  }
  else if (f.vremotes) {
    let x = f.vremotes;
    let remfunc = f.remfunc;
    $('#vremotesTable').empty();
    buildHtmlTableRemotes('#vremotesTable', remfunc, x);
  }
  else if (f.ithostatusinfo) {
    let x = f.ithostatusinfo;
    $('#StatusTable').empty();
    if (f.target == "hadisc") {
      if (f.itho_status_ready) {
        current_status_count = f.count;
        status_items_loaded = true;
        $("#HADiscForm, #save_update_had").removeClass('hidden');
        buildHtmlHADiscTable(x);
      }
      else {
        $("#ithostatusrdy").html("Itho status items not (completely) loaded yet:<br><br><div id='iis'></div><br><br>Reload to try again or disable I2C commands which might be unsupported for your device.<br><br><button id='hadreload' class='pure-button pure-button-primary'>Reload</button><br>");
        $("#ithostatusrdy").removeClass('hidden');
        $("#HADiscForm, #save_update_had").addClass('hidden');
        showItho(f.iis);
      }
    }
    else {
      buildHtmlStatusTable('#StatusTable', x);
    }
  }
  else if (f.hadiscsettings) {
    let x = f.hadiscsettings;
    saved_status_count = x.sscnt;
    if ((current_status_count !== saved_status_count) && (saved_status_count > 0)) {
      localStorage.setItem("ithostatus", JSON.stringify(x));
      $("#ithostatusrdy").html("The stored number of HA Discovery items does not match the current number of detected Itho status items. Activating/deactivating I2C functions can be a source of this discrepancy. Please check if configured items are still correct and click 'Save and update' to update the stored items.<br><button id='hadignore' class='pure-button pure-button-primary'>Ignore</button>");
      $("#ithostatusrdy").removeClass('hidden');
    }
    else {
      $("#ithostatusrdy").addClass('hidden');
      $("#ithostatusrdy").html('');
      updateStatusTableFromCompactJson(x);
    }

  }
  else if (f.i2cdebuglog) {
    let x = f.i2cdebuglog;
    $('#I2CLogTable').empty();
    buildHtmlTablePlain('#I2CLogTable', x);
  }
  else if (f.i2csniffer) {
    let x = f.i2csniffer;
    $('#i2clog_outer').removeClass('hidden');
    var d = new Date();
    $('#i2clog').prepend(`${d.toLocaleString('nl-NL')}: ${x}<br>`);
  }
  else if (f.ithodevinfo) {
    let x = f.ithodevinfo;
    processElements(x);
    localStorage.setItem("itho_setlen", x.itho_setlen);
    localStorage.setItem("itho_devtype", x.itho_devtype);
    localStorage.setItem("itho_fwversion", x.itho_fwversion);
    localStorage.setItem("itho_hwversion", x.itho_hwversion);
  }
  else if (f.ithosettings) {
    let x = f.ithosettings;
    updateSettingsLocStor(x);

    if (x.Index === 0 && x.update === false) {
      clearSettingsLocStor();
      localStorage.setItem("ihto_settings_complete", "false");
      $('#SettingsTable').empty();
      //add header
      addColumnHeader(x, '#SettingsTable', true);
      $('#SettingsTable').append('<tbody>');
    }
    if (x.update === false) {
      addRowTableIthoSettings($('#SettingsTable > tbody'), x);
    }
    else {
      updateRowTableIthoSettings(x);
    }
    if (x.Index < localStorage.getItem("itho_setlen") - 1 && x.loop === true && settingIndex == x.Index) {
      settingIndex++;
      websock_send(JSON.stringify({
        ithogetsetting: true,
        index: settingIndex,
        update: x.update
      }));
    }
    if (x.Index === localStorage.getItem("itho_setlen") - 1 && x.update === false && x.loop === true) {
      settingIndex = 0;
      websock_send(JSON.stringify({
        ithogetsetting: true,
        index: 0,
        update: true
      }));
    }
  }
  else if (f.wifiscanresult) {
    let x = f.wifiscanresult;
    $('#wifiscanresult').append(`<div class='ScanResults'><input id='${x.id}' class='pure-input-1-5' name='optionsWifi' value='${x.ssid}' type='radio'>${returnSignalSVG(x.sigval)}${returnWifiSecSVG(x.sec)} ${x.ssid}</label></div>`);
  }
  else if (f.systemstat) {
    let x = f.systemstat;
    uuid = x.uuid;
    if ('sensor_temp' in x) {
      $('#sensor_temp').html(`Temperature: ${round(x.sensor_temp, 1)}&#8451;`);
    }
    if ('sensor_hum' in x) {
      $('#sensor_hum').html(`Humidity: ${round(x.sensor_hum, 1)}%`);
    }
    $('#memory_box').show();
    $('#memory_box').html(`<p><b>Memory:</b><p><p>free: <b>${x.freemem}</b></p><p>low: <b>${x.memlow}</b></p>`);
    $('#mqtt_conn').removeClass();
    var button = returnMqttState(x.mqqtstatus);
    $('#mqtt_conn').addClass(`pure-button ${button.button}`);
    $('#mqtt_conn').text(button.state);
    $('#ithoslider').val(x.itho);
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
      initstatus = '<span style="color:#ca3c3c;">init failed - please power cycle the Itho unit -</span>';
    }
    else if (x.ithoinit == -2) {
      initstatus = '<span style="color:#ca3c3c;">i2c bus stuck - please power cycle the Itho unit -</span>';
    }
    else if (x.ithoinit == 1) {
      initstatus = '<span style="color:#1cb841;">connected</span>';
    }
    else if (x.ithoinit == 0) {
      initstatus = '<span style="color:#777;">setting up i2c connection</span>';
    }
    else {
      initstatus = 'unknown status';
    }
    $('#ithoinit').html(initstatus);
    if ('sensor' in x) {
      sensor = x.sensor;
    }
    if (x.itho_llm > 0) {
      $('#itho_llm').removeClass();
      $('#itho_llm').addClass("pure-button button-success");
      $('#itho_llm').text(`On ${x.itho_llm}`);
    }
    else {
      $('#itho_llm').removeClass();
      $('#itho_llm').addClass("pure-button button-secondary");
      $('#itho_llm').text("Off");
    }
    if (x.copy_id > 0) {
      $('#itho_copyid_vremote').removeClass();
      $('#itho_copyid_vremote').addClass("pure-button button-success");
      $('#itho_copyid_vremote').text(`Press join. Time remaining: ${x.copy_id}`);
    }
    else {
      $('#itho_copyid_vremote').removeClass();
      $('#itho_copyid_vremote').addClass("pure-button");
      $('#itho_copyid_vremote').text("Copy ID");
    }
    if ('format' in x) {
      if (x.format) {
        $('#format').text('Format filesystem');
      }
      else {
        $('#format').text('Format failed');
      }

    }
  }
  else if (f.remtypeconf) {
    let x = f.remtypeconf;
    if (hw_revision.startsWith('NON-CVE ') || itho_pwm2i2c == 0) {
      addvRemoteInterface(x.remtype);
    }
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
  else if (f.ota) {
    let x = f.ota;
    $('#updateprg').html(`Firmware update progress: ${x.percent}%`);
    moveBar(x.percent, "updateBar");
  }
  else if (f.sysmessage) {
    let x = f.sysmessage;
    $(`#${x.id}`).text(x.message);
  }
  else {
    processElements(f);
  }

}

function initButton() {
  document.getElementById("command-button").addEventListener("click", sendCommand)
}
function trapKeyPress() {
  document.getElementById("command-text").addEventListener("keypress", (e => {
    "Enter" === e.code && (e.preventDefault(), document.getElementById("command-button").click())
  })),
    document.addEventListener("keydown", (e => {
      document.activeElement && "command-text" === document.activeElement.id && ("ArrowUp" === e.code ? (commandHistoryIdx--, commandHistoryIdx < 0 && (commandHistoryIdx = commandHistory.length > 0 ? commandHistory.length - 1 : 0), commandHistoryIdx >= 0 && commandHistoryIdx < commandHistory.length && (document.getElementById("command-text").value = commandHistory[commandHistoryIdx])) : "ArrowDown" === e.code && (commandHistoryIdx++, commandHistoryIdx >= commandHistory.length && (commandHistoryIdx = 0), commandHistoryIdx >= 0 && commandHistoryIdx < commandHistory.length && (document.getElementById("command-text").value = commandHistory[commandHistoryIdx])))
    }))
}

function clearSettingsLocStor() {
  let setlen = localStorage.getItem("itho_setlen");
  for (var index = 0; index < setlen; index++) {
    const localStorageKey = 'settingsIndex_' + index;
    let existingData = localStorage.getItem(localStorageKey);
    if (existingData) {
      localStorage.removeItem(localStorageKey);
    }
  }
}

function updateSettingsLocStor(receivedData) {
  let setlen = localStorage.getItem("itho_setlen");
  if (receivedData.hasOwnProperty('Index')) {

    const localStorageKey = 'settingsIndex_' + receivedData.Index;
    let existingData = localStorage.getItem(localStorageKey);

    if (existingData) {
      existingData = JSON.parse(existingData);

      // Update existing data with new values
      for (const key in receivedData) {
        existingData[key] = receivedData[key];
      }

      // Store the updated data in LocalStorage
      localStorage.setItem(localStorageKey, JSON.stringify(existingData));
      if (existingData.Index == (setlen - 1)) {
        localStorage.setItem("ihto_settings_complete", "true");
        localStorage.setItem("uuid", uuid);
        $('#downloadsettingsdiv').removeClass('hidden');
      }
    }
    else {
      // If no existing data, store the new data as is
      localStorage.setItem("ihto_settings_complete", "false");
      localStorage.setItem(localStorageKey, JSON.stringify(receivedData));
    }

  }
}
function loadSettingsLocStor() {

  let setlen = localStorage.getItem("itho_setlen");
  if (typeof setlen === 'undefined' || setlen == null) {
    if (debug) console.log("error: loadSettingsLocStor setting length unavailable");
    return;
  }
  for (var index = 0; index < setlen; index++) {
    const localStorageKey = 'settingsIndex_' + index;
    let existingData = localStorage.getItem(localStorageKey);
    if (existingData) {
      existingData = JSON.parse(existingData);

      if (index == 0) {
        $('#SettingsTable').empty();
        addColumnHeader(existingData, '#SettingsTable', true);
        $('#SettingsTable').append('<tbody>');
      }
      let current_tmp = existingData.Current;
      let maximum_tmp = existingData.Maximum;
      let minimum_tmp = existingData.Minimum;
      existingData.Current = null;
      existingData.Maximum = null;
      existingData.Minimum = null;
      addRowTableIthoSettings($('#SettingsTable > tbody'), existingData);
      existingData.Current = current_tmp;
      existingData.Maximum = maximum_tmp;
      existingData.Minimum = minimum_tmp;
      updateRowTableIthoSettings(existingData);
    }
    else {
      if (debug) console.log("error: no cached setting info for index:" + index);
    }
  }

}


$(document).ready(function () {
  document.getElementById("layout").style.opacity = 0.3;
  document.getElementById("loader").style.display = "block";
  startWebsock(websocketServerLocation);

  //handle menu clicks
  $(document).on('click', 'ul.pure-menu-list li a', function (event) {
    var page = $(this).attr('href');
    update_page(page);
    $('li.pure-menu-item').removeClass("pure-menu-selected");
    $(this).parent().addClass("pure-menu-selected");
    event.preventDefault();
  });
  $(document).on('click', '#headingindex', function (event) {
    update_page('index');
    $('li.pure-menu-item').removeClass("pure-menu-selected");
    event.preventDefault();
  });
  //handle wifi network select
  $(document).on('change', 'input', function (e) {
    if ($(this).attr('name') == 'optionsWifi') {
      $('#ssid').val($(this).attr('value'));
    }
  });
  //handle submit buttons
  $(document).on('click', 'button', function (e) {
    if ($(this).attr('id').startsWith('command-')) {
      const items = $(this).attr('id').split('-');
      websock_send(`{"command":"${items[1]}"}`);
    }
    else if ($(this).attr('id') == 'wifisubmit') {
      hostname = $('#hostname').val();
      websock_send(JSON.stringify({
        wifisettings: {
          ssid: $('#ssid').val(),
          passwd: $('#passwd').val(),
          appasswd: $('#appasswd').val(),
          dhcp: $('input[name=\'option-dhcp\']:checked').val(),
          ip: $('#ip').val(),
          subnet: $('#subnet').val(),
          gateway: $('#gateway').val(),
          dns1: $('#dns1').val(),
          dns2: $('#dns2').val(),
          port: $('#port').val(),
          hostname: $('#hostname').val(),
          ntpserver: $('#ntpserver').val(),
          timezone: $('#timezone').val(),
          aptimeout: $('#aptimeout').val()
        }
      }));
      update_page('wifisetup');
    }
    //syssubmit
    else if ($(this).attr('id') == 'syssumbit') {
      if (!isValidJsonArray($('#api_settings_activated').val())) {
        alert("error: Activated settings input value is not a valid JSON array!");
        return;
      }
      else {
        if (!areAllUnsignedIntegers(JSON.parse($('#api_settings_activated').val()))) {
          alert("error: Activated settings array contains non integer values!");
          return;
        }
      }
      websock_send(JSON.stringify({
        systemsettings: {
          sys_username: $('#sys_username').val(),
          sys_password: $('#sys_password').val(),
          syssec_web: $('input[name=\'option-syssec_web\']:checked').val(),
          syssec_api: $('input[name=\'option-syssec_api\']:checked').val(),
          syssec_edit: $('input[name=\'option-syssec_edit\']:checked').val(),
          api_version: $('input[name=\'option-api_version\']:checked').val(),
          api_normalize: $('input[name=\'option-api_normalize\']:checked').val(),
          api_settings: $('input[name=\'option-api_settings\']:checked').val(),
          api_settings_activated: JSON.parse($('#api_settings_activated').val()),
          syssht30: $('input[name=\'option-syssht30\']:checked').val(),
          itho_rf_support: $('input[name=\'option-itho_rf_support\']:checked').val(),
          itho_fallback: $('#itho_fallback').val(),
          itho_low: $('#itho_low').val(),
          itho_medium: $('#itho_medium').val(),
          itho_high: $('#itho_high').val(),
          itho_timer1: $('#itho_timer1').val(),
          itho_timer2: $('#itho_timer2').val(),
          itho_timer3: $('#itho_timer3').val(),
          itho_updatefreq: $('#itho_updatefreq').val(),
          itho_counter_updatefreq: $('#itho_counter_updatefreq').val(),
          itho_numvrem: $('#itho_numvrem').val(),
          //itho_numrfrem: $('#iitho_numrfrem').val(),
          itho_sendjoin: $('input[name=\'option-itho_sendjoin\']:checked').val(),
          itho_forcemedium: $('input[name=\'option-itho_forcemedium\']:checked').val(),
          itho_vremoteapi: $('input[name=\'option-itho_vremoteapi\']:checked').val(),
          itho_pwm2i2c: $('input[name=\'option-itho_pwm2i2c\']:checked').val(),
          itho_31da: $('input[name=\'option-itho_31da\']:checked').val(),
          itho_31d9: $('input[name=\'option-itho_31d9\']:checked').val(),
          itho_2401: $('input[name=\'option-itho_2401\']:checked').val(),
          itho_4210: $('input[name=\'option-itho_4210\']:checked').val(),
          i2c_safe_guard: $('input[name=\'option-i2c_safe_guard\']:checked').val(),
          i2c_sniffer: $('input[name=\'option-i2c_sniffer\']:checked').val()
        }
      }));
      update_page('system');
    }
    else if ($(this).attr('id') == 'syslogsubmit') {
      websock_send(JSON.stringify({
        logsettings: {
          loglevel: $('#loglevel').val(),
          syslog_active: $('input[name=\'option-syslog_active\']:checked').val(),
          esplog_active: $('input[name=\'option-esplog_active\']:checked').val(),
          webserial_active: $('input[name=\'option-webserial_active\']:checked').val(),
          rfloglevel: $('#rfloglevel').val(),
          logserver: $('#logserver').val(),
          logport: $('#logport').val(),
          logref: $('#logref').val()
        }
      }));
      update_page('syslog');
    }
    //mqttsubmit
    else if ($(this).attr('id') == 'mqttsubmit') {
      websock_send(JSON.stringify({
        systemsettings: {
          mqtt_active: $('input[name=\'option-mqtt_active\']:checked').val(),
          mqtt_serverName: $('#mqtt_serverName').val(),
          mqtt_username: $('#mqtt_username').val(),
          mqtt_password: $('#mqtt_password').val(),
          mqtt_port: $('#mqtt_port').val(),
          mqtt_version: $('#mqtt_version').val(),
          mqtt_base_topic: $('#mqtt_base_topic').val(),
          mqtt_ha_topic: $('#mqtt_ha_topic').val(),
          mqtt_domoticzin_topic: $('#mqtt_domoticzin_topic').val(),
          mqtt_domoticzout_topic: $('#mqtt_domoticzout_topic').val(),
          mqtt_idx: $('#mqtt_idx').val(),
          sensor_idx: $('#sensor_idx').val(),
          mqtt_domoticz_active: $('input[name=\'option-mqtt_domoticz_active\']:checked').val(),
          mqtt_ha_active: $('input[name=\'option-mqtt_ha_active\']:checked').val()
        }
      }));
      update_page('mqtt');
    }
    else if ($(this).attr('id') == 'itho_llm') {
      websock_send('{"itho_llm":true}');
    }
    else if ($(this).attr('id') == 'itho_remove_remote' || $(this).attr('id') == 'itho_remove_vremote') {
      var selected = $('input[name=\'optionsRemotes\']:checked').val();
      if (selected == null) {
        alert("Please select a remote.")
      }
      else {
        var val = parseInt(selected, 10) + 1;
        websock_send('{"' + $(this).attr('id') + '":' + val + '}');
      }
    }
    else if ($(this).attr('id') == 'itho_update_remote' || $(this).attr('id') == 'itho_update_vremote') {
      var i = $('input[name=\'optionsRemotes\']:checked').val();
      if (i == null) {
        alert("Please select a remote.");
      }
      else {
        var remfunc = (typeof $('#func_remote-' + i).val() === 'undefined') ? 0 : $('#func_remote-' + i).val();
        var remtype = (typeof $('#type_remote-' + i).val() === 'undefined') ? 0 : $('#type_remote-' + i).val();
        var bidirectional = (typeof $('input[id=\'bidirect_remote-' + i + '\']:checked').val() === 'undefined') ? 0 : 1;
        var id = $('#id_remote-' + i).val();
        if (id == 'empty slot') id = "00,00,00";
        if (isHex(id.split(",")[0]) && isHex(id.split(",")[1]) && isHex(id.split(",")[2])) {
          websock_send(`{"${$(this).attr('id')}":${i},"id":[${parseInt(id.split(",")[0], 16)},${parseInt(id.split(",")[1], 16)},${parseInt(id.split(",")[2], 16)}],"value":"${$('#name_remote-' + i).val()}","remtype":${remtype},"remfunc":${remfunc},"bidirectional":${bidirectional}}`);
        }
        else {
          alert("ID error, please use HEX notation separated by ',' (ie. 'A1,34,7F')");
        }
      }
    }
    else if ($(this).attr('id') == 'update_rf_id') {
      var id = $('#module_rf_id_str').val();
      if (isHex(id.split(",")[0]) && isHex(id.split(",")[1]) && isHex(id.split(",")[2])) {
        websock_send(`{"update_rf_id":[${parseInt(id.split(",")[0], 16)},${parseInt(id.split(",")[1], 16)},${parseInt(id.split(",")[2], 16)}]}`);
      }
      else {
        alert("ID error, please use HEX notation separated by ',' (ie. 'A1,34,7F')");
      }
    }
    else if ($(this).attr('id') == 'update_num_rf') {
      websock_send(JSON.stringify({
        systemsettings: {
          itho_numrfrem: $('#itho_numrfrem').val()
        }
      }));
    }
    else if ($(this).attr('id') == 'itho_copyid_vremote') {
      var i = $('input[name=\'optionsRemotes\']:checked').val();
      if (i == null) {
        alert("Please select a remote.");
      }
      else {
        var val = parseInt(i, 10) + 1;
        websock_send(`{"copy_id":true, "index":${val}}`);
      }
    }
    else if ($(this).attr('id').substr(0, 15) == 'ithosetrefresh-') {
      var row = parseInt($(this).attr('id').substr(15));
      var i = $('input[name=\'options-ithoset\']:checked').val();
      if (i == null) {
        alert("Please select a row.");
      }
      else if (i != row) {
        alert("Please select the correct row.");
      }
      else {
        $('input[name=\'options-ithoset\']:checked').prop('checked', false);
        $('[id^=ithosetrefresh-]').each(function (index) {
          $(`#ithosetrefresh-${index}, #ithosetupdate-${index}`).removeClass('pure-button-primary');
        });
        $(`#Current-${i}, #Minimum-${i}, #Maximum-${i}`).html(`<div style='margin: auto;' class='dot-elastic'></div>`);
        websock_send('{"ithosetrefresh":' + i + '}');
      }
    }
    else if ($(this).attr('id').substr(0, 14) == 'ithosetupdate-') {
      var row = parseInt($(this).attr('id').substr(14));
      var i = $('input[name=\'options-ithoset\']:checked').val();
      if (i == null) {
        alert("Please select a row.");
      }
      else if (i != row) {
        alert("Please select the correct row.");
      }
      else {
        if (Number.isInteger(parseFloat($('#name_ithoset-' + i).val()))) {
          websock_send(JSON.stringify({
            ithosetupdate: i,
            value: parseInt($('#name_ithoset-' + i).val())
          }));
        }
        else {
          websock_send(JSON.stringify({
            ithosetupdate: i,
            value: parseFloat($('#name_ithoset-' + i).val())
          }));
        }

        $('input[name=\'options-ithoset\']:checked').prop('checked', false);
        $('[id^=ithosetrefresh-]').each(function (index) {
          $(`#ithosetrefresh-${index}, #ithosetupdate-${index}`).removeClass('pure-button-primary');
        });
        $(`#Current-${i}, #Minimum-${i}, #Maximum-${i}`).html(`<div style='margin: auto;' class='dot-elastic'></div>`);
      }
    }
    else if ($(this).attr('id') == 'resetwificonf') {
      if (confirm("This will reset the wifi config to factory default, are you sure?")) {
        websock_send('{"resetwificonf":true}');
      }
    }
    else if ($(this).attr('id') == 'resetsysconf') {
      if (confirm("This will reset the system configs files to factory default, are you sure?")) {
        websock_send('{"resetsysconf":true}');
      }
    }
    else if ($(this).attr('id') == 'saveallconfigs') {
      websock_send('{"saveallconfigs":true}');
    }
    else if ($(this).attr('id') == 'reboot') {
      if (confirm("This will reboot the device, are you sure?")) {
        $('#rebootscript').append(html_reboot_script);
        websock_send('{"reboot":true}');
      }
    }
    else if ($(this).attr('id') == 'format') {
      if (confirm("This will erase all settings, are you sure?")) {
        websock_send('{"format":true}');
        $('#format').text('Formatting...');
      }
    }
    else if ($(this).attr('id') == 'wifiscan') {
      $('.ScanResults').remove();
      $('.hidden').removeClass('hidden');
      websock_send('{"wifiscan":true}');
    }
    else if ($(this).attr('id').startsWith('button_vremote-')) {
      const items = $(this).attr('id').split('-');
      websock_send(`{"vremote":${items[1]}, "command":"${items[2]}"}`);
    }
    else if ($(this).attr('id').startsWith('button_remote-')) {
      const items = $(this).attr('id').split('-');
      websock_send(`{"remote":${items[1]}, "command":"${items[2]}"}`);
    }
    else if ($(this).attr('id').startsWith('ithobutton-')) {
      const items = $(this).attr('id').split('-');
      websock_send(`{"ithobutton":"${items[1]}"}`);
      if (items[1] == 'shtreset') $(`#i2c_sht_reset`).text("Processing...");
    }
    else if ($(this).attr('id').startsWith('button-')) {
      const items = $(this).attr('id').split('-');
      websock_send(`{"button":"${items[1]}"}`);
    }
    else if ($(this).attr('id').startsWith('rfdebug-')) {
      const items = $(this).attr('id').split('-');
      if (items[1] == 0) $('#rflog_outer').addClass('hidden');
      if (items[1] > 0) $('#rflog_outer').removeClass('hidden');
      if (items[1] == 12762) {
        websock_send(`{"rfdebug":${items[1]}, "faninfo":${$('#rfdebug-12762-faninfo').val()}, "timer":${$('#rfdebug-12762-timer').val()}}`);
      }
      else if (items[1] == 12761) {
        websock_send(`{"rfdebug":${items[1]}, "status":${$('#rfdebug-12761-status').val()}, "fault":${$('#rfdebug-12761-fault').val()}, "frost":${$('#rfdebug-12761-frost').val()}, "filter":${$('#rfdebug-12761-filter').val()}}`);

      }
      else {
        websock_send(`{"rfdebug":${items[1]}}`);
      }
    }
    else if ($(this).attr('id').startsWith('i2csniffer-')) {
      const items = $(this).attr('id').split('-');
      if (items[1] == 0) $('#i2clog_outer').addClass('hidden');
      if (items[1] > 0) $('#i2clog_outer').removeClass('hidden');
      websock_send(`{"i2csniffer":${items[1]}}`);
    }
    else if ($(this).attr('id') == 'button2410') {
      websock_send(JSON.stringify({
        ithobutton: 2410,
        index: parseInt($('#itho_setting_id').val())
      }));
    }
    else if ($(this).attr('id') == 'button2410set') {
      websock_send(JSON.stringify({
        ithobutton: 24109,
        ithosetupdate: parseInt($('#itho_setting_id_set').val()),
        value: parseFloat($('#itho_setting_value_set').val())
      }));
    }
    else if ($(this).attr('id') == 'button4210') {
      websock_send(JSON.stringify({
        ithobutton: 4210
      }));
    }
    else if ($(this).attr('id') == 'buttonCE30') {
      websock_send(JSON.stringify({
        ithobutton: 0xCE30,
        ithotemp: parseFloat($('#itho_ce30_temp').val() * 100.),
        ithotemptemp: parseFloat($('#itho_ce30_temptemp').val() * 100.),
        ithotimestamp: $('#itho_ce30_timestamp').val()
      }));
    }
    else if ($(this).attr('id') == 'buttonC000') { //CO2
      websock_send(JSON.stringify({
        ithobutton: 0xC000,
        itho_c000_speed1: Number($('#itho_c000_speed1').val()),
        itho_c000_speed2: Number($('#itho_c000_speed2').val())
      }));
    }
    else if ($(this).attr('id') == 'button9298') { //CO2 value
      websock_send(JSON.stringify({
        ithobutton: 0x9298,
        itho_9298_val: Number($('#itho_9298_val').val())
      }));
    }
    else if ($(this).attr('id') == 'button4030') {
      if ($('#itho_4030_password').val() == 'thisisunsafe') {
        websock.send(JSON.stringify({
          ithobutton: 4030,
          idx: Number($('#itho_4030_index').val()),
          dt: Number($('#itho_4030_datatype').val()),
          val: Number($('#itho_4030_value').val()),
          chk: Number($('#itho_4030_checked').val()),
        }));
      }
    }
    else if ($(this).attr('id') == 'ithogetsettings') {
      if (localStorage.getItem("ihto_settings_complete") == "true" && localStorage.getItem("uuid") == uuid) {
        loadSettingsLocStor();
        $('#settings_cache_load').removeClass('hidden');
        $('#downloadsettingsdiv').removeClass('hidden');
      }
      else {
        settingIndex = 0;
        websock_send(JSON.stringify({
          ithogetsetting: true,
          index: 0,
          update: false
        }));
      }
    }
    else if ($(this).attr('id') == 'ithoforcerefresh') {
      $('#settings_cache_load').addClass('hidden');
      $('#downloadsettingsdiv').addClass('hidden');
      settingIndex = 0;
      websock_send(JSON.stringify({
        ithogetsetting: true,
        index: 0,
        update: false
      }));
    }
    else if ($(this).attr('id') == 'downloadsettings') {
      if (localStorage.getItem("ihto_settings_complete") == "true" && localStorage.getItem("uuid") == uuid) {
        let settings = {};
        let setlen = localStorage.getItem("itho_setlen");
        settings['uuid'] = uuid;
        settings['itho_setlen'] = setlen;
        settings['itho_devtype'] = localStorage.getItem("itho_devtype");
        settings['itho_fwversion'] = localStorage.getItem("itho_fwversion");
        settings['itho_hwversion'] = localStorage.getItem("itho_hwversion");
        let errordetect = false;

        for (var index = 0; index < setlen; index++) {
          const localStorageKey = 'settingsIndex_' + index;
          let existingData = localStorage.getItem(localStorageKey);
          if (existingData) {
            let obj = JSON.parse(existingData);
            delete obj.loop;
            delete obj.update;
            settings[localStorageKey] = obj;
            for (const key in obj) {
              if (obj[key] == "ret_error") {
                errordetect = true;
              }
            }
          }
        }
        if (errordetect) {
          alert("error: values with errors detected, settings load not complete!");
        }
        else {
          let dataStr = JSON.stringify(settings);
          let dataBlob = new Blob([dataStr], { type: 'application/json' });
          let downloadLink = document.createElement('a');
          downloadLink.href = window.URL.createObjectURL(dataBlob);
          downloadLink.download = 'settings.json';
          document.body.appendChild(downloadLink);
          downloadLink.click();
          document.body.removeChild(downloadLink);
        }

      }
      else {
        alert("error: download of settings file not possible!");
      }
    }
    else if ($(this).attr('id') == 'updatesubmit') {
      e.preventDefault();
      var form = $('#updateform')[0];
      var data = new FormData(form);
      let filename = data.get('update').name;
      if (!filename.endsWith(".bin")) {
        count += 1;
        resetTimer();
        $('#message_box').show();
        $('#message_box').append(`<p class='messageP' id='mbox_p${count}'>Updater: file name error, please select a *.bin firmware file</p>`);
        removeAfter5secs(count);
        return;
      }
      $('#updatesubmit').addClass("pure-button-disabled");
      $('#updatesubmit').text("Update in progress...");
      $('#uploadProgress, #updateProgress, #uploadprg, #updateprg').show();
      $.ajax({ url: '/update', type: 'POST', data: data, contentType: false, processData: false, xhr: function () { var xhr = new window.XMLHttpRequest(); xhr.upload.addEventListener('progress', function (evt) { if (evt.lengthComputable) { var per = Math.round(10 + (((evt.loaded / evt.total) * 100) * 0.9)); $('#uploadprg').html('File upload progress: ' + per + '%'); moveBar(per, "uploadBar"); } }, false); return xhr; }, success: function (d, s) { moveBar(100, "updateBar"); $('#updateprg').html('Firmware update progress: 100%'); $('#updatesubmit').text("Update finished"); $('#time').show(); startCountdown(); }, error: function () { $('#updatesubmit').text("Update failed"); $('#time').show(); startCountdown(); } });
    }
    else if ($(this).attr('id') == 'hadreload') {
      $("#ithostatusrdy, #HADiscForm, #save_update_had").addClass('hidden');
      $("#ithostatusrdy").html('');
      setTimeout(function () { getSettings('hadiscinfo'); }, 1000);
    }
    else if ($(this).attr('id') == 'hadignore') {
      $("#ithostatusrdy").addClass('hidden');
      $("#ithostatusrdy").html('');
      let obj = localStorage.getItem("ithostatus");
      updateStatusTableFromCompactJson(JSON.parse(obj));
      localStorage.removeItem("ithostatus");
    }
    else if ($(this).attr('id') == 'save_update_had') {
      var jsonvar = generateCompactJson();
      websock_send(JSON.stringify({
        hadiscsettings: jsonvar
      }));
    }
    e.preventDefault();
    return false;
  });
  //keep the message box on top on scroll
  $(window).scroll(function (e) {
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

var timerHandle = setTimeout(function () {
  $('#message_box').hide();
}, 5000);

function resetTimer() {
  window.clearTimeout(timerHandle);
  timerHandle = setTimeout(function () {
    $('#message_box').hide();
  }, 5000);
}

function removeID(id) {
  $(`#${id}`).remove();
}

function processElements(x) {
  for (var key in x) {
    if (x.hasOwnProperty(key)) {
      if (Array.isArray(x[key])) {
        x[key] = JSON.stringify(x[key]);
      }
      var el = $(`#${key}`);
      if (el.is('input') || el.is('select')) {
        if ($(`#${key}`).val() !== x[key]) {
          $(`#${key}`).val(x[key]);
        }
      }
      else if (el.is('span')) {
        if ($(`#${key}`).text() !== x[key]) {
          $(`#${key}`).text(x[key]);
        }
      }
      else if (el.is('a')) {
        $(`#${key}`).attr("href", x[key]);
      }
      else {
        var radios = $(`input[name='option-${key}']`);
        if (radios[1]) {
          if (radios.is(':checked') === false) {
            radios.filter(`[value='${x[key]}']`).prop('checked', true);
          }
          radio(key, x[key]);
        }
      }
      var elbyname = $(`[name='${key}']`).each(function () {
        if ($(this).is('span')) {
          if ($(this).text() !== x[key]) {
            $(this).text(x[key]);
          }
        }
      });
    }
  }
}

function removeAfter5secs(count) {
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

function getlog(url) {
  const xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200) {
      let res = xhr.responseText.split(/\r?\n/).reverse().slice(1).join("<br>");
      $('#dblog').html(res);
    }
  }
  xhr.onerror = (e) => {
    $('#dblog').html(xhr.statusText);
  };
  xhr.send(null);
}

function radio(origin, state) {
  if (origin == "dhcp") {
    if (state == 'on') {
      $('#ip, #subnet, #gateway, #dns1, #dns2').prop('readonly', true);
      $('#port').prop('readonly', false);
      $('#option-dhcp-on, #option-dhcp-off').prop('disabled', false);
    }
    else {
      $('#ip, #subnet, #gateway, #dns1, #dns2, #port').prop('readonly', false);
      $('#option-dhcp-on, #option-dhcp-off').prop('disabled', false);
    }
  }
  else if (origin == "mqtt_active") {
    if (state == 1) {
      $('#mqtt_serverName, #mqtt_username, #mqtt_password, #mqtt_port, #mqtt_base_topic, #mqtt_ha_topic, #mqtt_domoticzin_topic, #mqtt_domoticzout_topic, #mqtt_idx, #sensor_idx').prop('readonly', false);
      $('#option-mqtt_domoticz_active-0, #option-mqtt_domoticz_active-1, #option-mqtt_ha_active-1, #option-mqtt_ha_active-0').prop('disabled', false);
    }
    else {
      $('#mqtt_serverName, #mqtt_username, #mqtt_password, #mqtt_port, #mqtt_base_topic, #mqtt_ha_topic, #mqtt_domoticzin_topic, #mqtt_domoticzout_topic, #mqtt_idx, #sensor_idx').prop('readonly', true);
      $('#option-mqtt_domoticz_active-0, #option-mqtt_domoticz_active-1, #option-mqtt_ha_active-1, #option-mqtt_ha_active-0').prop('disabled', true);

    }
  }
  else if (origin == "mqtt_domoticz_active") {
    if (state == 1) {
      $('#mqtt_domoticzin_topic, #label-mqtt_domoticzin, #label-mqtt_domoticzout, #mqtt_domoticzout_topic, #mqtt_idx, #label-mqtt_idx, #sensor_idx, #label-sensor_idx').show();
    }
    else {
      $('#mqtt_domoticzin_topic, #label-mqtt_domoticzin, #label-mqtt_domoticzout, #mqtt_domoticzout_topic, #mqtt_idx, #label-mqtt_idx, #sensor_idx, #label-sensor_idx').hide();
    }
  }
  else if (origin == "mqtt_ha_active") {
    if (state == 1) {
      $('#mqtt_ha_topic, #label-mqtt_ha').show();
    }
    else {
      $('#mqtt_ha_topic, #label-mqtt_ha').hide();
    }
  }
  else if (origin == "remote" || origin == "ithoset") {
    $(`[id^=name_${origin}-]`).each(function (index) {
      $(`#name_${origin}-${index}`).prop('readonly', true);
      if (index == state) {
        $(`#name_${origin}-${index}`).prop('readonly', false);
      }
    });
    $(`[id^=type_${origin}-]`).each(function (index) {
      $(`#type_${origin}-${index}`).prop('disabled', true);
      if (index == state) {
        $(`#type_${origin}-${index}`).prop('disabled', false);
      }
    });
    $(`[id^=func_${origin}-]`).each(function (index) {
      $(`#func_${origin}-${index}`).prop('disabled', true);
      if (index == state) {
        $(`#func_${origin}-${index}`).prop('disabled', false);
      }
    });
    $(`[id^=id_${origin}-]`).each(function (index) {
      $(`#id_${origin}-${index}`).prop('readonly', true);
      if (index == state) {
        $(`#id_${origin}-${index}`).prop('readonly', false);
      }
    });
    $(`[id^=bidirect_${origin}-]`).each(function (index) {
      $(`#bidirect_${origin}-${index}`).prop("disabled", true);
      if (index == state) {
        remfunction_validation(index);
      }
    });
    if (origin == "ithoset") {
      $('[id^=ithosetrefresh-]').each(function (index) {
        $(`#ithosetrefresh-${index}, #ithosetupdate-${index}`).removeClass('pure-button-primary');
        if (index == state) {
          $(`#ithosetrefresh-${index}, #ithosetupdate-${index}`).addClass('pure-button-primary');
        }
      });
    }
  }
}

function getSettings(pagevalue) {
  if (websock.readyState === 1) {
    websock_send('{"' + pagevalue + '":true}');
  }
  else {
    if (debug) console.log("websock not open");
    setTimeout(getSettings, 250, pagevalue);
  }
}

//
// Reboot script
//
var secondsRemaining;
var intervalHandle;
function resetPage() {
  window.location.href = ('http://' + hostname + '.local');
}
function tick() {
  var timeDisplay = document.getElementById('time');
  if (timeDisplay == null)
    return;
  var sec = secondsRemaining - (Math.floor(secondsRemaining / 60) * 60);
  if (sec < 10) sec = '0' + sec;
  timeDisplay.innerText = `This page will reload to the start page in ${sec} seconds...`;
  if (secondsRemaining === 0) {
    clearInterval(intervalHandle);
    resetPage();
    return;
  }
  secondsRemaining--;
}
function startCountdown() {
  secondsRemaining = 30;
  intervalHandle = setInterval(tick, 1000);
}
function moveBar(nPer, element) {
  var elem = document.getElementById(element);
  if (elem !== null) elem.style.width = nPer + '%';
}
//
//
//

function updateSlider(value) {
  var val = parseInt(value);
  if (isNaN(val)) val = 0;
  $('#ithotextval').html(val);
  websock_send(JSON.stringify({ 'itho': val }));
}

//function to load html main content
var lastPageReq = "";
function update_page(page) {
  lastPageReq = page;
  clearTimeout(wifistat_to);
  clearTimeout(statustimer_to);
  $('#main').empty();
  $('#main').css('max-width', '768px')
  if (page == 'index') { $('#main').append(html_index); }
  if (page == 'wifisetup') { $('#main').append(html_wifisetup); }
  if (page == 'syslog') { $('#main').append(html_syslog); }
  if (page == 'system') {
    $('#main').append(html_systemsettings_start);
    if (hw_revision == "2" || hw_revision.startsWith('NON-CVE ')) { $('#sys_fieldset').append(html_systemsettings_cc1101); }
    $('#sys_fieldset').append(html_systemsettings_end);
  }
  if (page == 'itho') { $('#main').append(html_ithosettings); }
  if (page == 'status') { $('#main').append(html_ithostatus); }
  if (page == 'remotes') { $('#main').append(html_remotessetup); }
  if (page == 'vremotes') { $('#main').append(html_vremotessetup); }
  if (page == 'mqtt') { $('#main').append(html_mqttsetup); }
  if (page == 'api') { $('#main').append(html_api); }
  if (page == 'help') { $('#main').append(html_help); }
  if (page == 'reset') { $('#main').append(html_reset); }
  if (page == 'update') { $('#main').append(html_update); }
  if (page == 'debug') { $('#main').append(html_debug); $('#main').css('max-width', '1600px') }
  if (page == 'i2cdebug') { $('#main').append(html_i2cdebug); }
  if (page == 'hadiscovery') { $('#main').append(html_hadiscovery); }


}

//handle menu collapse on smaller screens
window.onload = function () {
  (function (window, document) {
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
    menuLink.onclick = function (e) {
      toggleAll(e);
    };
    content.onclick = function (e) {
      if (menu.className.indexOf('active') !== -1) {
        toggleAll(e);
      }
    };
  }(this, this.document));
};

function ValidateIPaddress(ipaddress) {
  if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ipaddress)) {
    return true;
  }
  return false;
}

function isHex(hex) {
  if (typeof hex === 'string') {
    if (hex.length === 1)
      hex = "0" + hex;

    return hex.length === 2
      && !isNaN(Number('0x' + hex))
  }
  return false;
}

function isUnsignedInteger(value) {
  return Number.isInteger(value) && value >= 0;
}

function areAllUnsignedIntegers(array) {
  return array.every(isUnsignedInteger);
}

function isValidJsonArray(input) {
  try {
    const parsed = JSON.parse(input);
    return Array.isArray(parsed);
  } catch (e) {
    return false;
  }
}

function returnMqttState(state) {
  state = state + 5;
  var states = ["Disabled", "Connection Timeout", "Connection Lost", "Connection Failed", "Disconnected", "Connected", "MQTT version unsupported", "Client ID rejected", "Server Unavailable", "Bad Credentials", "Client Unauthorized"];
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
  return { "state": states[state], "button": button };
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
var remtypes = [
  ["RFT CVE", 0x22F1, ['away', 'low', 'medium', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT AUTO", 0x22F3, ['auto', 'autonight', 'low', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT-N", 0x22F5, ['away', 'low', 'medium', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT AUTO-N", 0x22F4, ['auto', 'autonight', 'low', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT DF/QF", 0x22F8, ['low', 'high', 'cook30', 'cook60', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT RV", 0x12A0, ['auto', 'autonight', 'low', 'medium', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT CO2", 0x1298, ['auto', 'autonight', 'low', 'medium', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT PIR", 0x2E10, ['motion_on', 'motion_off', 'join', 'leave']],
  ["RFT Spider", 0x22F2, ['auto', 'autonight', 'low', 'medium', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']]
];

var remfuncs = [
  ["Receive", 1],
  ["Monitor Only", 3],
  ["Send", 5]
];

function addRemoteButtons(selector, remfunc, remtype, vremotenum, seperator) {
  var remfuncname = remfunc == 1 ? "remote" : "vremote";
  for (const item of remtypes) {
    if (remtype == item[1]) {
      var newinner = '';
      for (var i = 0; i < item[2].length; ++i) {
        if (seperator) {
          if (i == 0 || item[2][i] == 'cook30' || item[2][i] == 'timer1' || (item[2].length == 10 && item[2][i] == 'low') || item[2][i] == 'join') { newinner += `<div style="text-align: center;margin: 2em 0 0 0;">`; }
        }

        newinner += `<button value='${item[2][i]}_remote-${vremotenum}' id='button_${remfuncname}-${vremotenum}-${item[2][i]}' class='pure-button pure-button-primary'>${item[2][i].charAt(0).toUpperCase() + item[2][i].slice(1)}</button>\u00A0`;

        if (seperator) {
          if (item[2][i] == 'high' || item[2][i] == 'cook60' || item[2][i] == 'timer3' || (item[2].length == 10 && item[2][i] == 'autonight') || item[2][i] == 'leave') {
            newinner += '</div>';
            $(selector).append(newinner);
            newinner = '';
          }
        }
        else {
          $(selector).append(newinner);
          newinner = '';
        }

      }
    }
  }
}

function addvRemoteInterface(remtype) {

  var elem = $('#reminterface');
  elem.empty();
  addRemoteButtons(elem, 2, remtype, 0, true);

}

function buildHtmlTablePlain(selector, jsonVar) {
  //var columns = addAllColumnHeadersPlain(jsonVar, selector);
  var columns = [];
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr />');

  for (var key in jsonVar[0]) {
    if ($.inArray(key, columnSet) == -1) {
      columnSet.push(key);
      headerTr$.append($('<th />').html(key));
    }
  }

  headerThead$.append(headerTr$);
  $(selector).append(headerThead$);

  for (var i = 0; i < jsonVar.length; i++) {
    var row$ = $('<tr />');
    for (var colIndex = 0; colIndex < columns.length; colIndex++) {
      var cellValue = jsonVar[i][columns[colIndex]];
      if (cellValue == null) cellValue = "";
      row$.append($('<td />').html(cellValue));
    }
    $(selector).append(row$);
  }
}

function remfunction_validation(i) {
  if ($('#func_remote-' + i).val() == 5) $(`#bidirect_remote-${i}`).prop("disabled", false);
  else $(`#bidirect_remote-${i}`).prop("disabled", true);
}

function buildHtmlTableRemotes(selector, remfunc, jsonVar) {

  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr>');
  headerTr$.append($('<th>').html('Select'));

  for (var key in jsonVar[0]) {
    var append = [];
    if (key === "index") {
      append = ["Index", true];
    }
    else if (key === "id") {
      append = ["ID", true];
    }
    else if (key === "name") {
      append = ["Name", true];
    }
    else if (key === "remfunc" && remfunc == 1) { //unly show on rf remote page
      append = ["Remote function", true];
    }
    else if (key === "remtype") {
      append = ["Remote model", true];
    }
    else if (key === "capabilities") {
      append = ["Capabilities", true];
    }
    else if (key === "bidirectional" && remfunc == 1) { //unly show on rf remote page
      append = ["Bidirectional", true];
    }
    if (append[1]) { headerTr$.append($('<th>').html(append[0])); }
  }

  headerThead$.append(headerTr$);

  $(selector).append(headerThead$);

  var headerTbody$ = $('<tbody>');
  remotesCount = jsonVar.length;

  for (const remote of jsonVar) {
    var i = 0;
    if (remote["index"]) i = remote["index"];
    var remtype = 0;
    var remfunction = 0;
    var row$ = $('<tr>');
    row$.append($('<td>').html(`<input type='radio' id='option-select_remote-${i}' name='optionsRemotes' onchange='radio("remote",${i})' value='${i}' />`));

    for (const key in remote) {
      const value = remote[key];

      if (key === "index") {
        row$.append($('<td>').html(value.toString()));
      }
      else if (key === "id") {
        var cellValue = value.toString();
        if (cellValue == null) cellValue = '';
        cellValue = `${value[0].toString(16).toUpperCase()},${value[1].toString(16).toUpperCase()},${value[2].toString(16).toUpperCase()}`;
        if (cellValue == "0,0,0") cellValue = "empty slot";
        var idval = `id_remote-${i}`;
        row$.append($('<td>').html(`<input type='text' id='${idval}' value='${cellValue}' readonly='' />`));
      }
      else if (key === "name") {
        var cellValue = value.toString();
        if (cellValue == null) cellValue = '';
        var idval = `name_remote-${i}`;
        row$.append($('<td>').html(`<input type='text' id='${idval}' value='${cellValue}' readonly='' />`));
      }
      else if (key === "remfunc") {
        remfunction = value;
        if (remfunction != 2) { //do not add remote function is remfunction == virtual remote
          var select = document.createElement('select');
          select.name = remfunction;
          select.id = `func_remote-${i}`;
          select.setAttribute('onChange', `remfunction_validation(${i});`);
          select.disabled = true;
          for (const item of remfuncs) {
            var option = document.createElement('option');
            option.value = item[1];
            option.text = item[0];
            if (item[1] == remfunction) {
              option.selected = true;
              remtype = remfunction;
            }
            select.appendChild(option);
          }
          row$.append($('<td>').html(select));
        }
      }
      else if (key === "remtype") {
        var cellValue = value;
        var select = document.createElement('select');
        select.name = cellValue;
        select.id = `type_remote-${i}`;
        select.disabled = true;
        for (const item of remtypes) {
          var option = document.createElement('option');
          option.value = item[1];
          option.text = item[0];
          if (item[1] == cellValue) {
            option.selected = true;
            remtype = cellValue;
          }
          select.appendChild(option);
        }
        row$.append($('<td>').html(select));
      }
      else if (key === "capabilities") {
        if (remfunction == 2 || remfunction == 5) {
          var td$ = $('<td>');
          addRemoteButtons(td$, remfunc, remtype, i, false);
          row$.append(td$);
        }
        else {
          var str = '';
          var JSONObj = value;
          if (JSONObj != null) {
            for (let value in JSONObj) {
              if (JSONObj.hasOwnProperty(value)) {
                str += value + ':' + JSONObj[value];
                if (value != Object.keys(JSONObj).pop()) str += ', ';
              }
            }
          }
          row$.append($('<td>').html(str));
        }
      }
      else if (key === "bidirectional") {
        if (remfunc != 2) { //do not add remote function is remfunction == virtual remote
          var checkbox = document.createElement('input');
          checkbox.type = 'checkbox';
          checkbox.id = `bidirect_remote-${i}`;
          checkbox.checked = jsonVar[i]["bidirectional"];
          checkbox.disabled = true;
          row$.append($('<td>').html(checkbox));
        }
      }
      else {
        var cellValue = value.toString();
        if (cellValue == null) cellValue = '';
        row$.append(cellValue);
      }
      headerTbody$.append(row$);
    }
  }
  $(selector).append(headerTbody$);
}

function buildHtmlStatusTable(selector, jsonVar) {
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr>');
  headerTr$.append($('<th>').html('Index'));
  headerTr$.append($('<th>').html('Label'));
  headerTr$.append($('<th>').html('Value'));
  headerThead$.append(headerTr$);
  $(selector).append(headerThead$);

  var headerTbody$ = $('<tbody>');

  Object.entries(jsonVar).forEach(([key, value], index) => {
    var row$ = $('<tr>');
    row$.append($('<td>').text(index));
    row$.append($('<td>').text(key));
    row$.append($('<td>').text(value));
    headerTbody$.append(row$);
  });

  $(selector).append(headerTbody$);

}

function addRowTableIthoSettings(selector, jsonVar) {
  var i = jsonVar.Index;
  var row$ = $(`<tr>`);
  row$.append($(`<td class='ithoset' style='text-align: center;vertical-align: middle;'>`).html(`<input type='radio' id='option-select_ithoset-${i}' name='options-ithoset' onchange='radio("ithoset",${i})' value='${i}' />`));
  for (var key in jsonVar) {
    if (key == "update") { continue; }
    if (key == "loop") { continue; }
    if (jsonVar[key] == null) {
      row$.append($(`<td class='ithoset' id='${key}-${i}'>`).html(`<div style='margin: auto;' class='dot-elastic'></div>`));
    }
    else {
      row$.append($(`<td class='ithoset'>`).html(jsonVar[key]));
    }
  }
  row$.append($(`<td class='ithoset'>`).html(`<button id='ithosetupdate-${i}' class='pure-button'>Update</button>`));
  row$.append($(`<td class='ithoset'>`).html(`<button id='ithosetrefresh-${i}' class='pure-button'>Refresh</button>`));

  $(selector).append(row$);
}

function updateRowTableIthoSettings(jsonVar) {
  var i = jsonVar.Index;
  for (var key in jsonVar) {
    if (key == "Current") {
      $(`#${key}-${i}`).html(`<input type='text' id='name_ithoset-${i}' value='${jsonVar[key]}' readonly='' />`);
    }
    if (key == "Minimum" || key == "Maximum") {
      $(`#${key}-${i}`).html(jsonVar[key]);
    }
  }
}

function addColumnHeader(jsonVar, selector, appendRow) {
  var columnSet = [];
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr>');
  headerTr$.append($(`<th class='ithoset'>`).html('Select'));

  for (var key in jsonVar) {
    if (key == "update") { continue; }
    if (key == "loop") { continue; }
    if ($.inArray(key, columnSet) == -1) {
      columnSet.push(key);
      headerTr$.append($(`<th class='ithoset'>`).html(key));
    }
  }
  headerTr$.append($(`<th class='ithoset'>`).html('&nbsp;'));
  headerTr$.append($(`<th class='ithoset'>`).html('&nbsp;'));

  headerThead$.append(headerTr$);
  if (appendRow) {
    $(selector).append(headerThead$);
  }
  return columnSet;
}

function buildHtmlHADiscTable(ithostatusinfo) {
  const advancedToggleId = "advancedModeToggle";

  // Set default value for device type input field
  document.getElementById("hadevicename").value = ha_dev_name;

  const headerThead$ = $("<thead>");
  const headerTr$ = $("<tr>");
  headerTr$.append($("<th>").html("Include"));
  headerTr$.append($("<th>").html("Label"));
  headerTr$.append($("<th>").html("HA Name"));
  headerTr$.append($("<th>").addClass("advanced hidden").html("Device Class"));
  headerTr$.append($("<th>").addClass("advanced hidden").html("Value Template"));
  headerTr$.append($("<th>").addClass("advanced hidden").html("Unit of Measurement"));
  headerThead$.append(headerTr$);
  $("#HADiscTable").append(headerThead$);

  const headerTbody$ = $("<tbody>");

  // Build table rows from ithostatusinfo
  Object.entries(ithostatusinfo).forEach(([key, value], index) => {
    const row$ = $("<tr>");
    const isDisabled = value === "not available";

    // Include checkbox (default unchecked)
    const includeCheckbox$ = $("<input>", {
      type: "checkbox",
      checked: false,
    });
    const includeTd$ = $("<td>").append(includeCheckbox$);
    row$.append(includeTd$);

    // Label (non-editable)
    row$.append($("<td>").text(key));

    // HA Name (editable, unit of measurement removed)
    const unitMatch = key.match(/\(([^)]+)\)$/); // Extract unit in parentheses
    const cleanName = key.replace(/\s*\([^)]+\)$/, ""); // Remove unit from name
    const nameInput$ = $("<input>", {
      type: "text",
      value: cleanName,
      placeholder: "Name",
    });
    row$.append($("<td>").append(nameInput$));

    // Device Class (editable, advanced)
    const deviceClassInput$ = $("<input>", {
      type: "text",
      class: "advanced hidden",
      placeholder: "Device Class",
    });
    row$.append($("<td>").addClass("advanced hidden").append(deviceClassInput$));

    // Value Template (editable, advanced, retains unit)
    const valueTemplateInput$ = $("<input>", {
      type: "text",
      class: "advanced hidden",
      placeholder: `{{ value_json['${key}'] }}`,
      value: `{{ value_json['${key}'] }}`,
      "data-default": `{{ value_json['${key}'] }}`, // Store the default value
    });
    row$.append($("<td>").addClass("advanced hidden").append(valueTemplateInput$));

    // Unit of Measurement (editable, advanced, pre-filled if present)
    const unitInput$ = $("<input>", {
      type: "text",
      class: "advanced hidden",
      placeholder: "Unit of Measurement",
      value: unitMatch ? unitMatch[1] : "",
    });
    row$.append($("<td>").addClass("advanced hidden").append(unitInput$));

    headerTbody$.append(row$);
  });

  $("#HADiscTable").append(headerTbody$);

  // Advanced toggle
  $("#" + advancedToggleId).on("change", function () {
    const showAdvanced = $(this).is(":checked");
    $(".advanced").toggleClass("hidden", !showAdvanced);
  });
}

var webapihtml = `
                                                                            <p>The WebAPI implementation follows the JSend specification.<br>
                                                                              More information about JSend can be found on github: <a href="https://github.com/omniti-labs/jsend" target="_blank" rel="noopener noreferrer">https://github.com/omniti-labs/jsend</a>
                                                                            </p>
                                                                            <p>The WebAPI always returns a JSON which will at least contain a key "status".<br>
                                                                              The value of the status key indicates the result of the API call. This can either be "success", "fail" or "error".
                                                                            </p>
                                                                            <p>In case of "success" or "fail":<br>
                                                                              <ul>
                                                                                <li>the returned JSON will always have a "data" key containing the resulting data of the request</li>
                                                                                <li>the value can be a string or a JSON object/array</li>
                                                                                <li>the returned JSON should contain a key "result" that contains a string with a short human readable API call result</li>
                                                                                <li>the returned JSON should contain a key "cmdkey" that conains a string copy of the given command when a URL encoded key/value pair is present in the API call</li>
                                                                              </ul>
                                                                            </p>
                                                                            <p>In case of "error": <br></p>
                                                                            <p>
                                                                              <ul>
                                                                                <li>the returned JSON will at least contain a key "message" with a value of type string, explaining what went wrong</li>
                                                                                <li>the returned JSON could also include a key "code" which contains a status code that should adhere to rfc9110</li>
                                                                              </ul>
                                                                            </p>
                                                                            `;

//
// HTML string literals
//
