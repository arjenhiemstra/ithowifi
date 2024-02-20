
var debug = false;
var count = 0;
var itho_low = 0;
var itho_medium = 127;
var itho_high = 255;
var sensor = -1;
var uuid = 0;
var wifistat_to;
var statustimer_to;

localStorage.setItem("statustimer", 0);
localStorage.setItem("wifistat", 0);
var settingIndex = -1;

var websocketServerLocation = location.protocol.indexOf("https") > -1 ? 'wss://' + window.location.hostname + ':8000/ws' : 'ws://' + window.location.hostname + ':8000/ws';

let messageQueue = [];
let websock;

function startWebsock(websocketServerLocation) {
  console.log(websocketServerLocation);
  messageQueue = [];
  websock = new WebSocket(websocketServerLocation);
  websock.addEventListener('message', event => {
    // Add message to the queue
    messageQueue.push(event.data);
  });
  websock.onopen = function (a) {
    console.log('websock open');
    document.getElementById("layout").style.opacity = 1;
    document.getElementById("loader").style.display = "none";
    if (lastPageReq !== "") {
      update_page(lastPageReq);
    }
    getSettings('syssetup');
  };

  websock.onclose = function (a) {
    console.log('websock close');
    // Try to reconnect in 200 milliseconds
    websock = null;
    document.getElementById("layout").style.opacity = 0.3;
    document.getElementById("loader").style.display = "block";
    setTimeout(function () { startWebsock(websocketServerLocation) }, 200);
  };

  websock.onerror = function (a) {
    try {
      console.log(a);
    } catch (error) {
      console.warn(error);
    }
  };
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
  let f;
  try {
    f = JSON.parse(decodeURIComponent(message));
  } catch (error) {
    f = JSON.parse(message);
  }
  if (debug) {
    console.log(f);
  }
  let g = document.body;
  if (f.wifisettings) {
    let x = f.wifisettings;
    processElements(x);
  }
  else if (f.logsettings) {
    let x = f.logsettings;
    processElements(x);
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
      if (x.itho_rf_support == 1 && x.rfInitOK == true) {
        $('#remotemenu').removeClass('hidden');
      }
      else {
        $('#remotemenu').addClass('hidden');
      }
    }
    if ("i2cmenu" in x) {
      if (x.i2cmenu == 1) {
        $('#i2cmenu').removeClass('hidden');
      }
      else {
        $('#i2cmenu').addClass('hidden');
      }
    }
  }
  else if (f.remotes) {
    let x = f.remotes;
    let remfunc = f.remfunc;
    $('#RemotesTable').empty();
    buildHtmlTable('#RemotesTable', remfunc, x);
  }
  else if (f.vremotes) {
    let x = f.vremotes;
    let remfunc = f.remfunc;
    $('#vremotesTable').empty();
    buildHtmlTable('#vremotesTable', remfunc, x);
  }
  else if (f.ithostatusinfo) {
    let x = f.ithostatusinfo;
    $('#StatusTable').empty();
    buildHtmlStatusTable('#StatusTable', x);
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
    console.log("error: loadSettingsLocStor setting length unavailable");
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
      console.log("error: no cached setting info for index:" + index);
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
    else if ($(this).attr('id') == 'syslogsumbit') {
      websock_send(JSON.stringify({
        logsettings: {
          loglevel: $('#loglevel').val(),
          syslog_active: $('input[name=\'option-syslog_active\']:checked').val(),
          esplog_active: $('input[name=\'option-esplog_active\']:checked').val(),
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
        var id = $('#id_remote-' + i).val();
        if (id == 'empty slot') id = "0,0,0";
        if (isHex(id.split(",")[0]) && isHex(id.split(",")[1]) && isHex(id.split(",")[2])) {
          websock_send(`{"${$(this).attr('id')}":${i},"id":[${parseInt(id.split(",")[0], 16)},${parseInt(id.split(",")[1], 16)},${parseInt(id.split(",")[2], 16)}],"value":"${$('#name_remote-' + i).val()}","remtype":${remtype},"remfunc":${remfunc}}`);
        }
        else {
          alert("ID error, please use HEX notation separated by ',' (ie. 'A1,34,7F')");
        }
      }
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
      if (confirm("This will reset the system config to factory default, are you sure?")) {
        websock_send('{"resetsysconf":true}');
      }
    }
    else if ($(this).attr('id') == 'reboot') {
      if (confirm("This will reboot the device, are you sure?")) {
        $('#rebootscript').append(html_reboot_script);
        if (document.getElementById("dontsaveconf") !== null) {
          websock_send('{"reboot":true,"dontsaveconf":' + document.getElementById("dontsaveconf").checked + '}');
        }
        else {
          websock_send('{"reboot":true}');
        }
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
      websock_send(`{"rfdebug":${items[1]}}`);
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
        ithosetupdate: $('#itho_setting_id_set').val(),
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
    else if ($(this).attr('id') == 'button4030') {
      websock.send(JSON.stringify({
        ithobutton: 4030,
        idx: Number($('#itho_4030_index').val()),
        dt: Number($('#itho_4030_datatype').val()),
        val: Number($('#itho_4030_value').val()),
        chk: Number($('#itho_4030_checked').val()),
        dryrun: ($('#itho_4030_password').val() == 'thisisunsafe') ? false : true,
      }));
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

function websock_send(message) {
  websock.send(message);
  if (debug) {
    console.log(message);
  }
}

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
    websock_send('{"' + pagevalue + '":1}');
  }
  else {
    console.log("websock not open");
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
  var message = `This page will reload to the start page in ${sec} seconds...`;
  timeDisplay.innerHTML = message;
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
  elem.style.width = nPer + '%';
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
  return typeof hex === 'string'
    && hex.length === 2
    && !isNaN(Number('0x' + hex))
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
  ["RFT AUTO-N", 0x22F4, ['auto', 'autonight', 'low', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT DF/QF", 0x22F8, ['low', 'high', 'cook30', 'cook60', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT RV", 0x12A0, ['auto', 'autonight', 'low', 'medium', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT CO2", 0x1298, ['auto', 'autonight', 'low', 'medium', 'high', 'timer1', 'timer2', 'timer3', 'join', 'leave']],
  ["RFT PIR", 0x2E10, ['motion_on', 'motion_off', 'join', 'leave']]
];

var remfuncs = [
  ["Receive", 1],
  ["Monitor Only", 3],
  ["Send", 5],
  ["Bidirectional", 7]
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
  var columns = addAllColumnHeadersPlain(jsonVar, selector);

  for (var i = 0; i < jsonVar.length; i++) {
    var row$ = $('<tr/>');
    for (var colIndex = 0; colIndex < columns.length; colIndex++) {
      var cellValue = jsonVar[i][columns[colIndex]];
      if (cellValue == null) cellValue = "";
      row$.append($('<td/>').html(cellValue));
    }
    $(selector).append(row$);
  }
}

function buildHtmlTable(selector, remfunc, jsonVar) {
  var columns = addAllColumnHeaders(jsonVar, selector, true, remfunc);
  var headerTbody$ = $('<tbody>');
  remotesCount = jsonVar.length;
  for (var i = 0; i < remotesCount; i++) {
    var remtype = 0;
    var remfunction = 0;
    var row$ = $('<tr>');
    row$.append($('<td>').html(`<input type='radio' id='option-select_remote-${i}' name='optionsRemotes' onchange='radio("remote",${i})' value='${i}'/>`));
    //colIndex 0 = index
    //colIndex 1 = id
    //colIndex 2 = name
    //colIndex 3 = remfunc
    //colIndex 4 = remtype
    //colIndex 5 = capabilities
    for (var colIndex = 0; colIndex < columns.length; colIndex++) {
      if (colIndex == 3) {
        remfunction = jsonVar[i][columns[colIndex]];
        if (remfunction != 2) { //do not add remote function is remfunction == virtual remote
          var select = document.createElement('select');
          select.name = remfunction;
          select.id = `func_remote-${i}`;
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
      else if (colIndex == 4) {
        var cellValue = jsonVar[i][columns[colIndex]];
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
      else if (colIndex == 5) {
        if (remfunction == 2 || remfunction == 5) {
          var td$ = $('<td>');
          addRemoteButtons(td$, remfunc, remtype, i, false);
          row$.append(td$);
        }
        else {
          var str = '';
          var JSONObj = jsonVar[i][columns[colIndex]];
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
      else {
        var cellValue = jsonVar[i][columns[colIndex]].toString();
        if (cellValue == null) cellValue = '';
        if (colIndex == 1) {
          cellValue = `${jsonVar[i][columns[colIndex]][0].toString(16).toUpperCase()},${jsonVar[i][columns[colIndex]][1].toString(16).toUpperCase()},${jsonVar[i][columns[colIndex]][2].toString(16).toUpperCase()}`;
          if (cellValue == "0,0,0") cellValue = "empty slot";
        }
        if (colIndex == 1 || colIndex == 2) {
          var idval = `name_remote-${i}`;
          if (colIndex == 1) {
            idval = `id_remote-${i}`;
          }
          row$.append($('<td>').html(`<input type='text' id='${idval}' value='${cellValue}' readonly=''/>`));
        }
        else {
          row$.append($('<td>').html(cellValue));
        }
      }
    }
    headerTbody$.append(row$);
  }
  $(selector).append(headerTbody$);
}

function buildHtmlStatusTable(selector, jsonVar) {
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr>');
  headerTr$.append($('<th>').html('Label'));
  headerTr$.append($('<th>').html('Value'));
  headerThead$.append(headerTr$);
  $(selector).append(headerThead$);

  var headerTbody$ = $('<tbody>');

  for (var key in jsonVar) {
    var row$ = $('<tr>');
    row$.append($('<td>').html(key));
    row$.append($('<td>').html(jsonVar[key]));
    headerTbody$.append(row$);
  }

  $(selector).append(headerTbody$);

}

function addRowTableIthoSettings(selector, jsonVar) {
  var i = jsonVar.Index;
  var row$ = $(`<tr>`);
  row$.append($(`<td class='ithoset' style='text-align: center;vertical-align: middle;'>`).html(`<input type='radio' id='option-select_ithoset-${i}' name='options-ithoset' onchange='radio("ithoset",${i})' value='${i}'/>`));
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

function addAllColumnHeadersPlain(jsonVar, selector) {
  var columnSet = [];
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr/>');

  for (var i = 0; i < jsonVar.length; i++) {
    var rowHash = jsonVar[i];
    for (var key in rowHash) {
      if ($.inArray(key, columnSet) == -1) {
        columnSet.push(key);
        headerTr$.append($('<th/>').html(key));
      }
    }
  }
  headerThead$.append(headerTr$);
  $(selector).append(headerThead$);

  return columnSet;
}

function addAllColumnHeaders(jsonVar, selector, appendRow, remfunc) {
  var columnSet = [];
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr>');
  headerTr$.append($('<th>').html('select'));

  for (var i = 0; i < jsonVar.length; i++) {
    var rowHash = jsonVar[i];
    for (var key in rowHash) {
      if ($.inArray(key, columnSet) == -1) {
        columnSet.push(key);
        if (key == "remfunc" & remfunc == 1) {
          headerTr$.append($('<th id="remfunc">').html(key));
        }
        else if (key != "remfunc") {
          headerTr$.append($('<th>').html(key));
        }
      }
    }
  }
  headerThead$.append(headerTr$);
  if (appendRow) {
    $(selector).append(headerThead$);
  }
  return columnSet;
}



//
// HTML string literals
//
