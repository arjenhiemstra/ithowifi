
var debug = true;
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
let connectTimeout = null;
let pingTimeout = null;
let websock = null;

let pingIntervalId = null;
let reconnectTimer = null;
let isConnecting = false;

function scheduleReconnect(delayMs = 5000) {
  // If a reconnect is already scheduled, do nothing.
  if (reconnectTimer) return;

  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    startWebsock();
  }, delayMs);
}

function cleanupSocketAndTimers() {
  clearTimeout(connectTimeout);
  connectTimeout = null;

  clearTimeout(pingTimeout);
  pingTimeout = null;

  clearInterval(pingIntervalId);
  pingIntervalId = null;

  // Close the socket safely
  try {
    if (websock != null && (websock.readyState === WebSocket.CONNECTING || websock.readyState === WebSocket.OPEN)) {
      websock.close();
    }
  } catch (e) { }

  websock = null;
}

function startWebsock() {
  // Guard: if already connecting, don't start another attempt
  if (isConnecting) return;
  isConnecting = true;

  messageQueue = [];
  cleanupSocketAndTimers();

  websock = new WebSocket(websocketServerLocation);

  const startTime = Date.now();
  const MAX_WAIT_MS = 5000;
  const CHECK_INTERVAL_MS = 100;

  const readyStateCheck = setInterval(() => {
    if (!websock) {
      if (debug) console.log("no webscok");
      clearInterval(readyStateCheck);
      isConnecting = false;
      return;
    }

    // If OPEN: attach handlers and finish
    if (websock.readyState === WebSocket.OPEN) {
      if (debug) console.log("websock OPEN");
      clearInterval(readyStateCheck);
      attachWebSocketHandlers();
      isConnecting = false;
      secureCredentials.init().catch(err => {
        if (debug) console.error('[SECURE] Initialization failed:', err);
      });
      return;
    }
    else {
      if (debug) console.log("websock not OPEN");

    }

    // If socket already CLOSED/CLOSING: stop and retry
    if (websock.readyState === WebSocket.CLOSING || websock.readyState === WebSocket.CLOSED) {
      if (debug) console.log("websock CLOSING ||  CLOSED");
      clearInterval(readyStateCheck);
      isConnecting = false;
      scheduleReconnect(5000);
      return;
    }

    // Timeout exceeded: abort and retry in 5s
    if (Date.now() - startTime >= MAX_WAIT_MS) {
      clearInterval(readyStateCheck);

      if (debug) console.log("websock open timeout, retrying in 5s");

      cleanupSocketAndTimers();
      isConnecting = false;
      scheduleReconnect(5000);
    }
  }, CHECK_INTERVAL_MS);
}

function attachWebSocketHandlers() {
  websock.addEventListener("open", (event) => {
    if (debug) console.log('websock open');
    clearTimeout(connectTimeout);
  });

  websock.addEventListener("message", (event) => {
    "pong" == event.data ? (clearTimeout(pingTimeout), pingTimeout = null) : messageQueue.push(event.data);
  });

  websock.addEventListener("close", () => {
    if (debug) console.log("websock close");

    document.getElementById("layout").style.opacity = 0.3;
    document.getElementById("loader").style.display = "block";

    scheduleReconnect(1000);
  });

  websock.addEventListener("error", (event) => {
    if (debug) console.log("websock error", event);
    scheduleReconnect(1000);
  });

  document.getElementById("layout").style.opacity = 1;
  document.getElementById("loader").style.display = "none";
  if (lastPageReq !== "") {
    update_page(lastPageReq);
  }
  getSettings('syssetup');

  if (!pingIntervalId) {
    pingIntervalId = setInterval(() => {
      if (!websock || websock.readyState !== WebSocket.OPEN) return;
      if (pingTimeout) return;

      pingTimeout = setTimeout(() => {
        if (debug) console.log("websock ping timeout");
        cleanupSocketAndTimers();
        scheduleReconnect(0);
      }, 6e4);

      websock_send("ping");
    }, 5.5e4);
  }
}

function websock_send(message) {
  if (websock.readyState === WebSocket.OPEN) {
    if (debug) console.log(message);
    websock.send(message);
  }
  else {
    if (debug) console.log("websock.readyState != open");
  }
}

/**
 * SECURITY LAYER 2: AES-256-CTR Encryption (Pure JavaScript - HTTP compatible)
 *
 * Works WITHOUT Web Crypto API - compatible with plain HTTP access.
 * Uses session key exchange + AES-256-CTR encryption.
 *
 * Security note: On HTTP, the session key exchange is visible to network observers.
 * This provides protection against:
 * - Credentials appearing in plaintext in logs/debug output
 * - Casual inspection of WebSocket traffic
 * - Simple replay attacks (random IV per message)
 *
 * For stronger security, use HTTPS.
 */

// Minimal AES-256 implementation (pure JavaScript, no dependencies)
const AES256 = (function () {
  // AES S-box
  const SBOX = new Uint8Array([
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
  ]);

  // Rcon for key expansion
  const RCON = new Uint8Array([0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36]);

  // Galois field multiplication
  function gmul(a, b) {
    let p = 0;
    for (let i = 0; i < 8; i++) {
      if (b & 1) p ^= a;
      const hi = a & 0x80;
      a = (a << 1) & 0xff;
      if (hi) a ^= 0x1b;
      b >>= 1;
    }
    return p;
  }

  // Key expansion for AES-256 (14 rounds, 60 words)
  function expandKey(key) {
    const w = new Uint8Array(240); // 60 * 4 bytes
    w.set(key.slice(0, 32));

    for (let i = 8; i < 60; i++) {
      let t = w.slice((i - 1) * 4, i * 4);
      if (i % 8 === 0) {
        // RotWord + SubWord + Rcon
        t = new Uint8Array([SBOX[t[1]], SBOX[t[2]], SBOX[t[3]], SBOX[t[0]]]);
        t[0] ^= RCON[(i / 8) - 1];
      } else if (i % 8 === 4) {
        t = new Uint8Array([SBOX[t[0]], SBOX[t[1]], SBOX[t[2]], SBOX[t[3]]]);
      }
      for (let j = 0; j < 4; j++) {
        w[i * 4 + j] = w[(i - 8) * 4 + j] ^ t[j];
      }
    }
    return w;
  }

  // AES block encryption (single 16-byte block)
  function encryptBlock(block, expandedKey) {
    const s = new Uint8Array(block);

    // Initial round key addition
    for (let i = 0; i < 16; i++) s[i] ^= expandedKey[i];

    // 14 rounds for AES-256
    for (let round = 1; round <= 14; round++) {
      // SubBytes
      for (let i = 0; i < 16; i++) s[i] = SBOX[s[i]];

      // ShiftRows
      let t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
      t = s[2]; s[2] = s[10]; s[10] = t; t = s[6]; s[6] = s[14]; s[14] = t;
      t = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = s[3]; s[3] = t;

      // MixColumns (skip in final round)
      if (round < 14) {
        for (let c = 0; c < 4; c++) {
          const i = c * 4;
          const a = [s[i], s[i + 1], s[i + 2], s[i + 3]];
          s[i] = gmul(a[0], 2) ^ gmul(a[1], 3) ^ a[2] ^ a[3];
          s[i + 1] = a[0] ^ gmul(a[1], 2) ^ gmul(a[2], 3) ^ a[3];
          s[i + 2] = a[0] ^ a[1] ^ gmul(a[2], 2) ^ gmul(a[3], 3);
          s[i + 3] = gmul(a[0], 3) ^ a[1] ^ a[2] ^ gmul(a[3], 2);
        }
      }

      // AddRoundKey
      const rk = round * 16;
      for (let i = 0; i < 16; i++) s[i] ^= expandedKey[rk + i];
    }

    return s;
  }

  // AES-CTR mode encryption
  function ctr(key, nonce, data) {
    const expandedKey = expandKey(key);
    const result = new Uint8Array(data.length);
    const counter = new Uint8Array(16);
    counter.set(nonce.slice(0, 12)); // 12-byte nonce + 4-byte counter

    for (let offset = 0; offset < data.length; offset += 16) {
      const keystream = encryptBlock(counter, expandedKey);
      const blockLen = Math.min(16, data.length - offset);

      for (let i = 0; i < blockLen; i++) {
        result[offset + i] = data[offset + i] ^ keystream[i];
      }

      // Increment counter (big-endian)
      for (let i = 15; i >= 12; i--) {
        if (++counter[i] !== 0) break;
      }
    }

    return result;
  }

  return { encrypt: ctr, decrypt: ctr }; // CTR mode: encrypt = decrypt
})();

class SecureCredentialManager {
  constructor() {
    this.sessionKey = null;
    this.isReady = false;
    this.initPromise = null;
    this.keyExchangeCallback = null;
  }

  async init() {
    if (this.initPromise) return this.initPromise;

    this.initPromise = (async () => {
      try {
        if (debug) console.log('[SECURE] Requesting session key...');

        // Request session key from device
        const sessionKeyB64 = await this.requestSessionKey();
        this.sessionKey = this.base64ToBytes(sessionKeyB64);

        if (this.sessionKey.length !== 32) {
          throw new Error('Invalid session key length');
        }

        this.isReady = true;
        if (debug) console.log('[SECURE] Session established (AES-256-CTR)');

      } catch (error) {
        if (debug) console.error('[SECURE] Init failed:', error.message);
        if (debug) console.warn('[SECURE] Falling back to Layer 1 (password masking only)');
        this.isReady = false;
      }
    })();

    return this.initPromise;
  }

  requestSessionKey() {
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.keyExchangeCallback = null;
        reject(new Error('Timeout waiting for session key'));
      }, 5000);

      this.keyExchangeCallback = (data) => {
        clearTimeout(timeout);
        this.keyExchangeCallback = null;
        if (data.session_key) {
          resolve(data.session_key);
        } else {
          reject(new Error('Invalid session key response'));
        }
      };

      websock_send(JSON.stringify({ command: 'get_session_key' }));
    });
  }

  // Called by processMessage for session key response
  handleKeyExchangeResponse(data) {
    if (data.session_key && this.keyExchangeCallback) {
      this.keyExchangeCallback(data);
    }
  }

  // Generate random bytes (works without crypto.getRandomValues)
  getRandomBytes(length) {
    // Try Web Crypto first (works in most contexts)
    if (window.crypto && window.crypto.getRandomValues) {
      return window.crypto.getRandomValues(new Uint8Array(length));
    }
    // Fallback: Math.random (less secure but works everywhere)
    const bytes = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      bytes[i] = Math.floor(Math.random() * 256);
    }
    return bytes;
  }

  // Encrypt using AES-256-CTR (pure JavaScript)
  encryptCredential(plaintext) {
    if (!this.isReady || !this.sessionKey) {
      if (debug) console.warn('[SECURE] Encryption not available');
      return { encrypted: false };
    }

    try {
      // Generate random 12-byte nonce
      const nonce = this.getRandomBytes(12);
      const plaintextBytes = new TextEncoder().encode(plaintext);

      // Encrypt with AES-256-CTR
      const ciphertext = AES256.encrypt(this.sessionKey, nonce, plaintextBytes);

      return {
        encrypted: true,
        ciphertext: this.bytesToBase64(ciphertext),
        nonce: this.bytesToBase64(nonce)
      };

    } catch (error) {
      if (debug) console.error('[SECURE] Encryption failed:', error);
      return { encrypted: false };
    }
  }

  getEncryptedField(password) {
    if (!password || password === '********' || password === '') {
      return null;
    }

    const encrypted = this.encryptCredential(password);
    if (!encrypted.encrypted) {
      return null;
    }

    return { ct: encrypted.ciphertext, nonce: encrypted.nonce };
  }

  // Utility: bytes to base64
  bytesToBase64(bytes) {
    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }

  // Utility: base64 to bytes
  base64ToBytes(base64) {
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    return bytes;
  }
}

// Initialize secure credential manager
const secureCredentials = new SecureCredentialManager();

var domReady = false;
$(function () { domReady = true; });

(async function processMessages() {
  while (messageQueue.length > 0) {
    if (!domReady) break;
    const message = messageQueue.shift();
    processMessage(message);
  }
  await new Promise(resolve => setTimeout(resolve, 0));
  processMessages();
})();

const messageHandlers = {
  session_key: function (f) {
    secureCredentials.handleKeyExchangeResponse(f);
  },
  wifisettings: function (f) {
    processElements(f.wifisettings);
  },
  logsettings: function (f) {
    var x = f.logsettings;
    processElements(x);
    if (x.rfloglevel > 0) $('#rflog_outer').removeClass('hidden');
  },
  wifistat: function (f) {
    processElements(f.wifistat);
  },
  debuginfo: function (f) {
    processElements(f.debuginfo);
  },
  systemsettings: function (f) {
    var x = f.systemsettings;
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
      $('#i2cmenu').toggleClass('hidden', x.i2cmenu != 1);
    }
    if ("api_version" in x) {
      localStorage.setItem("api_version", x.api_version);
    }
  },
  remotes: function (f) {
    if ($('#RemotesTable tbody tr').length > 0) {
      updateRemoteCapabilities(f.remotes);
    } else {
      $('#RemotesTable').empty();
      buildHtmlTableRemotes('#RemotesTable', f.remfunc, f.remotes);
    }
  },
  vremotes: function (f) {
    $('#vremotesTable').empty();
    buildHtmlTableRemotes('#vremotesTable', f.remfunc, f.vremotes);
  },
  ithostatusinfo: function (f) {
    var x = f.ithostatusinfo;
    $('#StatusTable').empty();
    if (f.target == "hadisc") {
      if (f.itho_status_ready) {
        current_status_count = f.count;
        status_items_loaded = true;
        $("#HADiscForm, #save_update_had").removeClass('hidden');
        buildHtmlHADiscTable(x);
        if (f.rfdevices) buildRFDevicesUI(f.rfdevices);
        if (f.vrdevices) buildVirtualRemotesUI(f.vrdevices);
      }
      else {
        $("#ithostatusrdy").html("Itho status items not (completely) loaded yet:<br><br><div id='iis'></div><br><br>Reload to try again or disable I2C commands (menu System Settings) which might be unsupported for your devic.<br><br><button id='hadreload' class='pure-button pure-button-primary'>Reload</button><br>");
        $("#ithostatusrdy").removeClass('hidden');
        $("#HADiscForm, #save_update_had").addClass('hidden');
        showItho(f.iis);
      }
    }
    else {
      buildHtmlStatusTable('#StatusTable', x);
    }
  },
  hadiscsettings: function (f) {
    var x = f.hadiscsettings;
    saved_status_count = x.sscnt;
    if ((current_status_count !== saved_status_count) && (saved_status_count > 0)) {
      localStorage.setItem("ithostatus", JSON.stringify(x));
      $("#ithostatusrdy").html("The stored number of HA Discovery items does not match the current number of detected Itho status items. Activating/deactivating I2C functions can be a source of this discrepancy. Please check if configured items are still correct and click 'Save and update' to update the stored items.<br><button id='hadignore' class='pure-button pure-button-primary'>Ignore and use config</button><button id='hadusenew' class='pure-button pure-button-primary'>Use new config</button>");
      $("#ithostatusrdy").removeClass('hidden');
    }
    else {
      $("#ithostatusrdy").addClass('hidden').html('');
      updateStatusTableFromCompactJson(x);
    }
  },
  i2cdebuglog: function (f) {
    $('#I2CLogTable').empty();
    buildHtmlTablePlain('#I2CLogTable', f.i2cdebuglog);
  },
  i2csniffer: function (f) {
    $('#i2clog_outer').removeClass('hidden');
    $('#i2clog').prepend(`${new Date().toLocaleString('nl-NL')}: ${f.i2csniffer}<br>`);
  },
  ithodevinfo: function (f) {
    var x = f.ithodevinfo;
    processElements(x);
    localStorage.setItem("itho_setlen", x.itho_setlen);
    localStorage.setItem("itho_devtype", x.itho_devtype);
    localStorage.setItem("itho_fwversion", x.itho_fwversion);
    localStorage.setItem("itho_hwversion", x.itho_hwversion);
  },
  ithosettings: function (f) {
    var x = f.ithosettings;
    updateSettingsLocStor(x);

    if (x.Index === 0 && x.update === false) {
      clearSettingsLocStor();
      localStorage.setItem("ihto_settings_complete", "false");
      $('#SettingsTable').empty();
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
  },
  wifiscanresult: function (f) {
    var x = f.wifiscanresult;
    $('#wifiscanresult').append(`<div class='ScanResults'><input id='${x.id}' class='pure-input-1-5' name='optionsWifi' value='${x.ssid}' type='radio'>${returnSignalSVG(x.sigval)}${returnWifiSecSVG(x.sec)} ${x.ssid}</label></div>`);
  },
  systemstat: function (f) {
    var x = f.systemstat;
    uuid = x.uuid;
    if ('sensor_temp' in x) {
      $('#sensor_temp').html(`Temperature: ${round(x.sensor_temp, 1)}&#8451;`);
    }
    if ('sensor_hum' in x) {
      $('#sensor_hum').html(`Humidity: ${round(x.sensor_hum, 1)}%`);
    }
    $('#memory_box').show().html(`<p><b>Memory:</b><p><p>free: <b>${x.freemem}</b></p><p>low: <b>${x.memlow}</b></p>`);
    var mqttEl = $('#mqtt_conn');
    var button = returnMqttState(x.mqqtstatus);
    mqttEl.removeClass().addClass(`pure-button ${button.button}`).text(button.state);
    $('#ithoslider').val(x.itho);
    $('#ithotextval').html(x.itho);
    if ('itho_low' in x) itho_low = x.itho_low;
    if ('itho_medium' in x) itho_medium = x.itho_medium;
    if ('itho_high' in x) itho_high = x.itho_high;
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
    if ('sensor' in x) sensor = x.sensor;
    var llmEl = $('#itho_llm');
    if (x.itho_llm > 0) {
      llmEl.removeClass().addClass("pure-button button-success").text(`On ${x.itho_llm}`);
    }
    else {
      llmEl.removeClass().addClass("pure-button button-secondary").text("Off");
    }
    var copyEl = $('#itho_copyid_vremote');
    if (x.copy_id > 0) {
      copyEl.removeClass().addClass("pure-button button-success").text(`Press join. Time remaining: ${x.copy_id}`);
    }
    else {
      copyEl.removeClass().addClass("pure-button").text("Copy ID");
    }
    if ('format' in x) {
      $('#format').text(x.format ? 'Format filesystem' : 'Format failed');
    }
  },
  remtypeconf: function (f) {
    var x = f.remtypeconf;
    if (hw_revision.startsWith('NON-CVE ') || itho_pwm2i2c == 0) {
      addvRemoteInterface(x.remtype);
    }
  },
  messagebox: function (f) {
    var x = f.messagebox;
    count += 1;
    resetTimer();
    $('#message_box').show().append(`<p class='messageP' id='mbox_p${count}'>Message: ${x.message}</p>`);
    removeAfter5secs(count);
  },
  rflog: function (f) {
    $('#rflog_outer').removeClass('hidden');
    $('#rflog').prepend(`${new Date().toLocaleString('nl-NL')}: ${f.rflog.message}<br>`);
  },
  ota: function (f) {
    var x = f.ota;
    $('#updateprg').html(`Firmware update progress: ${x.percent}%`);
    moveBar(x.percent, "updateBar");
  },
  sysmessage: function (f) {
    var x = f.sysmessage;
    $(`#${x.id}`).text(x.message);
  }
};

function processMessage(message) {
  if (debug) console.log(message);
  let f;
  try {
    f = JSON.parse(decodeURIComponent(message));
  } catch (error) {
    f = JSON.parse(message);
  }

  for (var key in messageHandlers) {
    if (key in f) {
      messageHandlers[key](f);
      return;
    }
  }
  processElements(f);
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
  startWebsock();

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

      const wifi_passwd = $('#passwd').val();
      const wifi_appasswd = $('#appasswd').val();

      // Build secure_credentials for changed passwords
      const secure_credentials = {};
      const enc_passwd = secureCredentials.getEncryptedField(wifi_passwd);
      const enc_appasswd = secureCredentials.getEncryptedField(wifi_appasswd);
      if (enc_passwd) secure_credentials.passwd = enc_passwd;
      if (enc_appasswd) secure_credentials.appasswd = enc_appasswd;

      const wifiMsg = {
        wifisettings: {
          ssid: $('#ssid').val(),
          passwd: enc_passwd ? '********' : wifi_passwd,
          appasswd: enc_appasswd ? '********' : wifi_appasswd,
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
      };
      if (Object.keys(secure_credentials).length > 0) {
        wifiMsg.wifisettings.secure_credentials = secure_credentials;
      }
      websock_send(JSON.stringify(wifiMsg));

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

      const sys_passwd = $('#sys_password').val();
      const sys_secure_credentials = {};
      const enc_sys = secureCredentials.getEncryptedField(sys_passwd);
      if (enc_sys) sys_secure_credentials.sys_password = enc_sys;

      const sysMsg = {
        systemsettings: {
          sys_username: $('#sys_username').val(),
          sys_password: enc_sys ? '********' : sys_passwd,
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
      };
      if (Object.keys(sys_secure_credentials).length > 0) {
        sysMsg.systemsettings.secure_credentials = sys_secure_credentials;
      }
      websock_send(JSON.stringify(sysMsg));
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
      const mqtt_passwd = $('#mqtt_password').val();
      const mqtt_secure_credentials = {};
      const enc_mqtt = secureCredentials.getEncryptedField(mqtt_passwd);
      if (enc_mqtt) mqtt_secure_credentials.mqtt_password = enc_mqtt;

      const mqttMsg = {
        systemsettings: {
          mqtt_active: $('input[name=\'option-mqtt_active\']:checked').val(),
          mqtt_serverName: $('#mqtt_serverName').val(),
          mqtt_username: $('#mqtt_username').val(),
          mqtt_password: enc_mqtt ? '********' : mqtt_passwd,
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
      };
      if (Object.keys(mqtt_secure_credentials).length > 0) {
        mqttMsg.systemsettings.secure_credentials = mqtt_secure_credentials;
      }
      websock_send(JSON.stringify(mqttMsg));
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
        var bidirectional = (typeof $('input[id=\'bidirect_remote-' + i + '\']:checked').val() === 'undefined') ? false : true;
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
      var i = parseInt($('input[name=\'options-ithoset\']:checked').val());
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
    else if ($(this).attr('id') == 'resethadconf') {
      if (confirm("This will reset the HA Discovery configs to factory default, are you sure?")) {
        websock_send('{"resethadconf":true}');
        setTimeout(function () { $('#main').empty(); $('#main').append(html_hadiscovery); }, 1000);
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
    else if ($(this).attr('id') == 'hadusenew') {
      $("#ithostatusrdy").addClass('hidden');
      $("#ithostatusrdy").html('');
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
    var enabled = (state == 1);
    $('#mqtt_serverName, #mqtt_username, #mqtt_password, #mqtt_port, #mqtt_base_topic, #mqtt_ha_topic, #mqtt_domoticzin_topic, #mqtt_domoticzout_topic, #mqtt_idx, #sensor_idx').prop('readonly', !enabled);
    $('#option-mqtt_domoticz_active-0, #option-mqtt_domoticz_active-1, #option-mqtt_ha_active-1, #option-mqtt_ha_active-0').prop('disabled', !enabled);
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
  if (websock != null && websock.readyState === WebSocket.OPEN) {
    websock_send('{"' + pagevalue + '":true}');
  }
  else {
    if (debug) console.log("websock not open, rerequested: " + pagevalue);
    setTimeout(() => { getSettings(pagevalue); }, 250);
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
  var c = function (threshold) { return signalVal > threshold ? '000000' : 'cccccc'; };
  return `<svg id='svgSignalImage' width=28 height=28 xmlns='http://www.w3.org/2000/svg' viewBox='0 0 96 120' style='enable-background:new 0 0 96 96;' xml:space='preserve'><rect x='5' y='70.106' width='13.404' height='13.404' rx='2' ry='2' fill='#${c(0)}'/><rect x='24.149' y='56.702' width='13.404' height='26.809' rx='2' ry='2' fill='#${c(1)}'/><rect x='43.298' y='43.298' width='13.404' height='40.213' rx='2' ry='2' fill='#${c(2)}'/><rect x='62.447' y='29.894' width='13.403' height='53.617' rx='2' ry='2' fill='#${c(3)}'/><rect x='81.596' y='16.489' width='13.404' height='67.021' rx='2' ry='2' fill='#${c(4)}'/></svg>`;
}

function returnWifiSecSVG(secVal) {
  var inner = '';
  if (secVal == 2) {
    inner = '<g><path d=\'M64,41h-4v-8.2c0-6.6-5.4-12-12-12s-12,5.4-12,12V41h-4c-2.2,0-4,2-4,4.2v22c0,4.4,3.6,7.8,8,7.8h24c4.4,0,8-3.3,8-7.8v-22   C68,43,66.2,41,64,41z M40,32.8c0-4.4,3.6-8,8-8s8,3.6,8,8V41H40V32.8z M64,67.2c0,2.2-1.8,3.8-4,3.8H36c-2.2,0-4-1.5-4-3.8V45h32   V67.2z\'/><g><path d=\'M48,62c-1.1,0-2-0.9-2-2v-2c0-1.1,0.9-2,2-2s2,0.9,2,2v2C50,61.1,49.1,62,48,62z\'/></g></g>';
  } else if (secVal == 1) {
    inner = '<g><path d=\'M66,20.8c-6.6,0-12,5.4-12,12V41H32c-2.2,0-4,2-4,4.2v22c0,4.4,3.6,7.8,8,7.8h24c4.4,0,8-3.3,8-7.8v-22   c0-2.2-1.8-4.2-4-4.2h-6v-8.2c0-4.4,3.6-8,8-8s8,3.6,8,8V36h4v-3.2C78,26.1,72.6,20.8,66,20.8z M64,67.2c0,2.2-1.8,3.8-4,3.8H36   c-2.2,0-4-1.5-4-3.8V45h32V67.2z\'/><g><path d=\'M48,62c-1.1,0-2-0.9-2-2v-2c0-1.1,0.9-2,2-2s2,0.9,2,2v2C50,61.1,49.1,62,48,62z\'/></g></g>';
  } else {
    inner = '<path d=\'M52.7,18.9c-9-0.9-19.1,4.5-21.4,18.3l6.9,1.2c1.7-10,8.4-13.1,14-12.6c4.8,0.4,9.8,3.6,9.8,9c0,4.4-2.5,6.8-6.6,10.1  c-3.9,3.2-8.7,7.3-8.7,14.6v6.1h6.9v-6.1c0-3.9,2.3-6,6.2-9.2c4-3.4,9.1-7.7,9.1-15.5C68.8,26.3,62.1,19.7,52.7,18.9z\'/><rect x=\'46.6\' y=\'73.4\' width=\'6.9\' height=\'7.8\'/>';
  }
  return `<svg id='svgSignalImage' width=30 height=30 xmlns='http://www.w3.org/2000/svg' viewBox='0 0 96 120' style='enable-background:new 0 0 96 96;' xml:space='preserve'>${inner}</svg>`;
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
  if (!jsonVar || jsonVar.length === 0) return;
  var columns = [];
  var headerThead$ = $('<thead>');
  var headerTr$ = $('<tr />');

  for (var key in jsonVar[0]) {
    if ($.inArray(key, columnSet) == -1) {
      columnSet.push(key);
      columns.push(key);
      headerTr$.append($('<th />').html(key));
    }
  }

  headerThead$.append(headerTr$);

  var tbody$ = $('<tbody>');
  for (var i = 0; i < jsonVar.length; i++) {
    var row$ = $('<tr />');
    for (var colIndex = 0; colIndex < columns.length; colIndex++) {
      var cellValue = jsonVar[i][columns[colIndex]];
      if (cellValue == null) cellValue = "";
      row$.append($('<td />').html(cellValue));
    }
    tbody$.append(row$);
  }

  $(selector).append(headerThead$).append(tbody$);
}

function remfunction_validation(i) {
  if ($('#func_remote-' + i).val() == 5) $(`#bidirect_remote-${i}`).prop("disabled", false);
  else $(`#bidirect_remote-${i}`).prop("disabled", true);
}

function formatCapabilities(JSONObj) {
  var str = '';
  if (JSONObj != null) {
    for (let value in JSONObj) {
      if (JSONObj.hasOwnProperty(value)) {
        str += value + ':' + JSONObj[value];
        if (value != Object.keys(JSONObj).pop()) str += ', ';
      }
    }
  }
  return str;
}

function updateRemoteCapabilities(jsonVar) {
  for (const remote of jsonVar) {
    var i = remote["index"] || 0;
    var capsCell = $(`#caps-${i}`);
    if (capsCell.length && remote["capabilities"]) {
      capsCell.html(formatCapabilities(remote["capabilities"]));
    }
  }
}

function buildHtmlTableRemotes(selector, remfunc, jsonVar) {
  if (!jsonVar || jsonVar.length === 0) return;

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
          var td$ = $('<td>').attr('id', `caps-${i}`);
          td$.html(formatCapabilities(value));
          row$.append(td$);
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
      // else {
      //   var cellValue = value.toString();
      //   if (cellValue == null) cellValue = '';
      //   row$.append(cellValue);
      // }
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
  const selectAllCb$ = $("<input>", { type: "checkbox", id: "selectAllStatus" });
  headerTr$.append($("<th>").append(selectAllCb$).append(" Include"));
  headerTr$.append($("<th>").html("Label"));
  headerTr$.append($("<th>").html("HA Name"));
  headerTr$.append($("<th>").addClass("advanced hidden").html("Device Class"));
  headerTr$.append($("<th>").addClass("advanced hidden").html("State Class"));
  headerTr$.append($("<th>").addClass("advanced hidden").html("Value Template"));
  headerTr$.append($("<th>").addClass("advanced hidden").html("Unit of Measurement"));
  headerThead$.append(headerTr$);
  $("#HADiscTable").append(headerThead$);

  // Select all / deselect all
  selectAllCb$.on("change", function () {
    const checked = $(this).is(":checked");
    $("#HADiscTable tbody tr td:first-child input[type='checkbox']").prop("checked", checked);
  });

  const headerTbody$ = $("<tbody>");

  // Build table rows from ithostatusinfo
  Object.entries(ithostatusinfo).forEach(([key, value], index) => {
    const row$ = $("<tr>");
    const isDisabled = value === "not available";

    // Include checkbox (default unchecked)
    const includeCheckbox$ = $("<input>", {
      type: "checkbox",
      checked: false,
      "data-field": "include",
    });
    const includeTd$ = $("<td>").append(includeCheckbox$);
    row$.append(includeTd$);

    // Label (non-editable)
    row$.append($("<td>").text(key));

    // HA Name (editable, unit of measurement removed)
    const unitMatch = key.match(/\(([^)]+)\)$/);
    const cleanName = key.replace(/\s*\([^)]+\)$/, "");
    const nameInput$ = $("<input>", {
      type: "text",
      value: cleanName,
      placeholder: "Name",
      "data-field": "name",
    });
    row$.append($("<td>").append(nameInput$));

    // Device Class (editable, advanced)
    const deviceClassInput$ = $("<input>", {
      type: "text",
      class: "advanced hidden",
      placeholder: "Device Class",
      "data-field": "dc",
    });
    row$.append($("<td>").addClass("advanced hidden").append(deviceClassInput$));

    // State Class (editable, advanced)
    const stateClassInput$ = $("<input>", {
      type: "text",
      class: "advanced hidden",
      placeholder: "State Class",
      "data-field": "sc",
    });
    row$.append($("<td>").addClass("advanced hidden").append(stateClassInput$));

    // Value Template (editable, advanced, retains unit)
    const valueTemplateInput$ = $("<input>", {
      type: "text",
      class: "advanced hidden",
      placeholder: `{{ value_json['${key}'] }}`,
      value: `{{ value_json['${key}'] }}`,
      "data-default": `{{ value_json['${key}'] }}`,
      "data-field": "vt",
    });
    row$.append($("<td>").addClass("advanced hidden").append(valueTemplateInput$));

    // Unit of Measurement (editable, advanced, pre-filled if present)
    const unitInput$ = $("<input>", {
      type: "text",
      class: "advanced hidden",
      placeholder: "Unit of Measurement",
      value: unitMatch ? unitMatch[1] : "",
      "data-field": "um",
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

// RF Devices for HA Discovery
var rfDevicesData = [];
var vrDevicesData = [];

// Sensor display names and units
const sensorInfo = {
  lastcmd: { name: 'Last Command', unit: '', deviceClass: '' },
  temp: { name: 'Temperature', unit: '°C', deviceClass: 'temperature' },
  hum: { name: 'Humidity', unit: '%', deviceClass: 'humidity' },
  dewpoint: { name: 'Dew Point', unit: '°C', deviceClass: 'temperature' },
  co2: { name: 'CO2', unit: 'ppm', deviceClass: 'carbon_dioxide' },
  battery: { name: 'Battery', unit: '%', deviceClass: 'battery' },
  pir: { name: 'Motion', unit: '', deviceClass: 'motion' }
};

function buildRFDevicesUI(rfdevices) {
  rfDevicesData = rfdevices || [];
  const container = $('#rfdevices-container');
  container.empty();

  if (!rfdevices || rfdevices.length === 0) {
    $('#rfdevices-header').addClass('hidden');
    return;
  }

  $('#rfdevices-header').removeClass('hidden');

  rfdevices.forEach((device) => {
    const availableSensors = device.available ? device.available.split(',') : [];
    const availablePresets = device.presets ? device.presets.split(',') : [];
    const allSensors = ['lastcmd', 'temp', 'hum', 'dewpoint', 'co2', 'battery', 'pir'];

    let sensorsHtml = '';
    allSensors.forEach(sensor => {
      const isAvailable = availableSensors.includes(sensor);
      const info = sensorInfo[sensor] || { name: sensor, unit: '' };
      const checkboxId = `rf_${device.idx}_${sensor}`;

      if (isAvailable) {
        sensorsHtml += `
          <div class="rf-sensor-item">
            <input type="checkbox" id="${checkboxId}" data-rfidx="${device.idx}" data-sensor="${sensor}">
            <label for="${checkboxId}">${info.name}</label>
          </div>`;
      } else {
        sensorsHtml += `
          <div class="rf-sensor-item rf-sensor-unavailable">
            <input type="checkbox" id="${checkboxId}" disabled>
            <label for="${checkboxId}">${info.name}</label>
          </div>`;
      }
    });

    // Add fan checkbox if presets are available
    let fanHtml = '';
    if (availablePresets.length > 0) {
      const fanCheckboxId = `rf_${device.idx}_fan`;
      fanHtml = `
        <div class="rf-fan-section">
          <div class="rf-sensor-item rf-fan-item">
            <input type="checkbox" id="${fanCheckboxId}" data-rfidx="${device.idx}" data-fan="true" data-type="${device.type}">
            <label for="${fanCheckboxId}"><strong>Create Fan Device</strong></label>
          </div>
          <div class="rf-presets-info">Presets: ${availablePresets.join(', ')}</div>
        </div>`;
    }

    const deviceHtml = `
      <div class="rf-device-section" data-rfidx="${device.idx}" data-type="${device.type}">
        <div class="rf-device-title">${device.name} (Index: ${device.idx})</div>
        <div class="rf-sensor-list">
          ${sensorsHtml}
        </div>
        ${fanHtml}
      </div>`;

    container.append(deviceHtml);
  });
}

function buildVirtualRemotesUI(vrdevices) {
  vrDevicesData = vrdevices || [];
  const container = $('#vrdevices-container');
  container.empty();

  if (!vrdevices || vrdevices.length === 0) {
    $('#vrdevices-header').addClass('hidden');
    return;
  }

  $('#vrdevices-header').removeClass('hidden');

  vrdevices.forEach((device) => {
    const availablePresets = device.presets ? device.presets.split(',') : [];
    if (availablePresets.length === 0) return;

    const fanCheckboxId = `vr_${device.idx}_fan`;
    const deviceHtml = `
      <div class="rf-device-section" data-vridx="${device.idx}" data-type="${device.type}">
        <div class="rf-device-title">${device.name} (Index: ${device.idx})</div>
        <div class="rf-sensor-item rf-fan-item">
          <input type="checkbox" id="${fanCheckboxId}" data-vridx="${device.idx}" data-fan="true" data-type="${device.type}">
          <label for="${fanCheckboxId}"><strong>Create Fan Device</strong></label>
        </div>
        <div class="rf-presets-info">Presets: ${availablePresets.join(', ')}</div>
      </div>`;

    container.append(deviceHtml);
  });
}

function updateStatusTableFromCompactJson(compactJson) {
  ha_dev_name = compactJson.d;
  document.getElementById("hadevicename").value = ha_dev_name;

  const rows = $("#HADiscTable tbody tr");
  if (rows.length == 0) return;

  if (compactJson.c && compactJson.c.length > 0) {
    compactJson.c.forEach((component) => {
      const row$ = rows.eq(component.i);
      row$.find("input[data-field='include']").prop("checked", true);
      row$.find("input[data-field='name']").val(component.n);
      if (component.dc) row$.find("input[data-field='dc']").val(component.dc);
      if (component.sc) row$.find("input[data-field='sc']").val(component.sc);
      if (component.vt) row$.find("input[data-field='vt']").val(component.vt);
      if (component.um) row$.find("input[data-field='um']").val(component.um);
    });
  }

  // Restore RF device sensor and fan selections
  if (compactJson.rf) {
    compactJson.rf.forEach((rfConfig) => {
      var idx = rfConfig.idx;
      (rfConfig.sensors || []).forEach(s => $(`#rf_${idx}_${s}`).prop('checked', true));
      if (rfConfig.fan) $(`#rf_${idx}_fan`).prop('checked', true);
    });
  }

  // Restore virtual remote fan selections
  if (compactJson.vr) {
    compactJson.vr.forEach((vrConfig) => {
      if (vrConfig.fan) $(`#vr_${vrConfig.idx}_fan`).prop('checked', true);
    });
  }
}

function generateCompactJson() {
  const rows = $("#HADiscTable tbody tr");
  const components = [];

  rows.each(function (index) {
    var row$ = $(this);
    if (!row$.find("input[data-field='include']").is(":checked")) return;

    var vtInput = row$.find("input[data-field='vt']");
    var component = { i: index, n: row$.find("input[data-field='name']").val() };
    var dc = row$.find("input[data-field='dc']").val();
    var sc = row$.find("input[data-field='sc']").val();
    var vt = vtInput.val();
    var um = row$.find("input[data-field='um']").val();
    if (dc) component.dc = dc;
    if (sc) component.sc = sc;
    if (vt && vt !== vtInput.data("default")) component.vt = vt;
    if (um) component.um = um;
    components.push(component);
  });

  // Collect RF device sensor and fan selections
  const rfSelections = [];
  rfDevicesData.forEach(device => {
    var sensors = [];
    $(`input[data-rfidx="${device.idx}"][data-sensor]:checked`).each(function () {
      sensors.push($(this).data('sensor'));
    });
    var fanEnabled = $(`#rf_${device.idx}_fan`).is(':checked');
    if (sensors.length > 0 || fanEnabled) {
      var rfEntry = { idx: device.idx, name: device.name, type: device.type };
      if (sensors.length > 0) rfEntry.sensors = sensors;
      if (fanEnabled) rfEntry.fan = true;
      rfSelections.push(rfEntry);
    }
  });

  // Collect virtual remote fan selections
  const vrSelections = [];
  vrDevicesData.forEach(device => {
    if ($(`#vr_${device.idx}_fan`).is(':checked')) {
      vrSelections.push({ idx: device.idx, name: device.name, type: device.type, fan: true });
    }
  });

  var compactJson = {
    sscnt: current_status_count,
    d: document.getElementById("hadevicename").value.trim(),
    c: components,
    rf: rfSelections,
    vr: vrSelections
  };
  if (debug) console.log(JSON.stringify(compactJson));
  return compactJson;
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
