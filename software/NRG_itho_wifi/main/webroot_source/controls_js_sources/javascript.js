
var debug = true;
var remotesRefreshNeeded = false;
var sliderUserInput = 0;
var count = 0;
var itho_low = 0;
var itho_medium = 127;
var itho_high = 255;
var sensor = -1;
var uuid = 0;
var wifistat_to;
var statustimer_to;
var ithostatus_to;
var hastatus_to;
var remotesRefreshInterval;
var rfstatus_to;
var saved_status_count = 0;
var current_status_count = 0;
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
  if (!wizardActive) {
    var savedPage = sessionStorage.getItem('currentPage');
    var page = savedPage || lastPageReq || 'index';
    update_page(page);
    var menuItem = $q('a[href="' + page + '"]');
    if (menuItem) {
      $qa('li.pure-menu-item').forEach(function (el) { el.classList.remove('pure-menu-selected'); });
      menuItem.parentElement.classList.add('pure-menu-selected');
    }
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
  if (!websock || websock.readyState !== WebSocket.OPEN) {
    if (debug) console.log("websock.readyState != open");
    return;
  }
  if (debug) console.log(message);
  websock.send(message);
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

// DOM helper functions (jQuery replacement)
function $id(id) { return document.getElementById(id); }
function $qa(sel) { return document.querySelectorAll(sel); }
function $q(sel) { return document.querySelector(sel); }
function $val(id) { var el = document.getElementById(id); return el ? el.value : undefined; }

var domReady = false;

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", function () {
    domReady = true;
    if (debug) console.info("DOM loaded");
  });
} else {
  domReady = true;
  if (debug) console.info("DOM loaded");
}

// Execute <script> tags after insertAdjacentHTML (which doesn't run them)
function execScripts(container) {
  container.querySelectorAll('script').forEach(function (old) {
    var s = document.createElement('script');
    s.textContent = old.textContent;
    old.parentNode.replaceChild(s, old);
  });
}

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
    var rfo = $id('rflog_outer');
    if (rfo && x.rfloglevel > 0) rfo.classList.remove('hidden');
  },
  wifistat: function (f) {
    // Map WiFi status codes to user-friendly text
    var statusMap = {
      'WL_CONNECTED': 'Connected',
      'WL_DISCONNECTED': 'Disconnected',
      'WL_IDLE_STATUS': 'Idle',
      'WL_NO_SSID_AVAIL': 'Network not found',
      'WL_CONNECT_FAILED': 'Connection failed',
      'WL_CONNECTION_LOST': 'Connection lost',
      'WL_SCAN_COMPLETED': 'Scan completed'
    };
    if (f.wifistat.wificonnstat && statusMap[f.wifistat.wificonnstat]) {
      f.wifistat.wificonnstat = statusMap[f.wifistat.wificonnstat];
    }
    processElements(f.wifistat);
    // Stop wizard connect polling once connected
    if (wizardActive && wizardConnectInterval && f.wifistat.wificonnstat === 'Connected') {
      clearInterval(wizardConnectInterval);
      wizardConnectInterval = null;
    }
  },
  debuginfo: function (f) {
    processElements(f.debuginfo);
  },
  systemsettings: function (f) {
    var x = f.systemsettings;
    processElements(x);
    if (wizardActive) {
      if ("rfInitOK" in x) {
        wizardHasRF = (x.itho_rf_support == 1 && x.rfInitOK == true);
        wizardUpdateIndicators();
      }
      return;
    }
    if ("itho_rf_support" in x) {
      if (x.itho_rf_support == 1 && x.rfInitOK == false) {
        if (confirm("For changes to take effect click 'Ok' to reboot")) {
          var main = $id('main');
          main.innerHTML = '';
          main.insertAdjacentHTML('beforeend', "<br><br><br><br>");
          main.insertAdjacentHTML('beforeend', html_reboot_script);
          execScripts(main);
          websock_send('{"reboot":true}');
        }
      }
      if ("itho_control_interface" in x) {
        localStorage.setItem("itho_control_interface", x.itho_control_interface);
      }
      if (x.itho_rf_support == 1 && x.rfInitOK == true) {
        $id('remotemenu').classList.remove('hidden');
        $id('rfstatusmenu').classList.remove('hidden');
        var rfMqttRow = $id('rfstatus_mqtt_row');
        if (rfMqttRow) rfMqttRow.style.display = '';
      } else {
        $id('remotemenu').classList.add('hidden');
        $id('rfstatusmenu').classList.add('hidden');
      }
    }
    if (x.mqtt_ha_active == 1) $id('hadiscmenu').classList.remove('hidden');
    else $id('hadiscmenu').classList.add('hidden');
    if ("i2cmenu" in x) {
      $id('i2cmenu').classList.toggle('hidden', x.i2cmenu != 1);
    }
  },
  remotes: function (f) {
    var tbl = $id('RemotesTable');
    if (!tbl) return;
    if (tbl.querySelector('tbody tr') && !remotesRefreshNeeded) {
      updateRemoteCapabilities(f.remotes);
    } else {
      tbl.innerHTML = '';
      buildHtmlTableRemotes(tbl, f.remfunc, f.remotes);
      remotesRefreshNeeded = false;
    }
    if (wizardActive && wizardDeviceCategory === 'hru250_300') {
      wizardAutoConfigRFRemote();
    }
  },
  vremotes: function (f) {
    var tbl = $id('vremotesTable');
    if (!tbl) return;
    tbl.innerHTML = '';
    buildHtmlTableRemotes(tbl, f.remfunc, f.vremotes);
  },
  ithostatusinfo: function (f) {
    var x = f.ithostatusinfo;
    var st = $id('StatusTable');
    if (st) st.innerHTML = '';
    if (f.target == "hadisc") {
      var haForm = $id('HADiscForm');
      if (!haForm) return;
      if (f.itho_status_ready) {
        current_status_count = f.count;
        status_items_loaded = true;
        haForm.classList.remove('hidden');
        $id('save_update_had').classList.remove('hidden');
        buildHtmlHADiscTable(x);
        if (f.rfdevices) buildRFDevicesUI(f.rfdevices);
        if (f.vrdevices) buildVirtualRemotesUI(f.vrdevices);
      }
      else {
        $id('ithostatusrdy').innerHTML = "Itho status items not (completely) loaded yet:<br><br><div id='iis'></div><br><br>Reload to try again or disable I2C commands (menu System Settings) which might be unsupported for your devic.<br><br><button id='hadreload' class='pure-button pure-button-primary'>Reload</button><br>";
        $id('ithostatusrdy').classList.remove('hidden');
        haForm.classList.add('hidden');
        $id('save_update_had').classList.add('hidden');
        showItho(f.iis);
      }
    }
    else {
      if (st) buildHtmlStatusTable(st, x);
    }
    // Wizard: detect CO2 sensor for CVE-Silent
    if (wizardActive && wizardDevType === 'CVE-Silent') {
      if ('co2level_ppm' in x && x.co2level_ppm > 0) {
        var det = $id('wiz-co2-detected');
        if (det) det.style.display = '';
      }
    }
  },
  rfstatusinfo: function (f) {
    var x = f.rfstatusinfo;
    var sel = $id('rfStatusSource');
    if (sel && x.sources) {
      var prev = sel.value;
      x.sources.forEach(function (s) {
        var opt = sel.querySelector('option[value="' + s.index + '"]');
        if (!opt) {
          opt = document.createElement('option');
          opt.value = s.index;
          sel.appendChild(opt);
        }
        var cfg = (typeof rfSourcesData !== 'undefined') ? rfSourcesData.find(function (c) { return c.index == s.index; }) : null;
        opt.textContent = (cfg && cfg.tracked) ? ((cfg.name || s.id) + ' *') : s.id;
      });
      if (sel.querySelector('option[value="' + prev + '"]')) sel.value = prev;
      if (sel.value == '-1' && x.sources.length > 0) {
        sel.value = x.sources[0].index;
      }
    }
    var lsEl = $id('rfLastSeen');
    if (lsEl && x.sources && x.selectedSource !== undefined) {
      var src = x.sources.find(function (s) { return s.index == x.selectedSource; });
      if (src && src.lastSeen > 0) lsEl.textContent = new Date(src.lastSeen * 1000).toLocaleString();
      else lsEl.textContent = '-';
    }
    var st = $id('RFStatusTable');
    if (st) {
      if (x.data && Object.keys(x.data).length > 0) { st.innerHTML = ''; buildHtmlStatusTable(st, x.data); }
      else if (x.selectedSource !== undefined) { st.innerHTML = '<tr><td style="padding:1em;">Waiting for data from selected source device...</td></tr>'; }
    }
  },
  rfstatusconfig: function (f) {
    var x = f.rfstatusconfig;
    var sel = $id('rfStatusSource');
    if (sel && x.sources) {
      if (typeof rfSourcesData !== 'undefined') rfSourcesData = x.sources;
      if (typeof rfSelectedSource !== 'undefined' && x.selectedSource !== undefined)
        rfSelectedSource = x.selectedSource;
      if (typeof rfTrackedCount !== 'undefined') {
        rfTrackedCount = x.trackedCount || 0;
        rfMaxTracked = x.maxTracked || 20;
      }
      sel.innerHTML = '';
      if (x.sources.length === 0) {
        sel.insertAdjacentHTML('beforeend', '<option value="-1">No devices detected</option>');
      } else {
        x.sources.forEach(function (s) {
          var opt = document.createElement('option');
          opt.value = s.index;
          opt.textContent = s.tracked ? (s.name || s.id) + ' *' : s.id;
          if (s.index == x.selectedSource) opt.selected = true;
          sel.appendChild(opt);
        });
      }
    }
    if (typeof updateTrackCheckbox === 'function') updateTrackCheckbox();
  },
  hadiscsettings: function (f) {
    var el = $id('ithostatusrdy');
    if (!el) return;
    var x = f.hadiscsettings;
    saved_status_count = x.sscnt;
    if ((current_status_count !== saved_status_count) && (saved_status_count > 0)) {
      localStorage.setItem("ithostatus", JSON.stringify(x));
      el.innerHTML = "The stored number of HA Discovery items does not match the current number of detected Itho status items. Activating/deactivating I2C functions can be a source of this discrepancy. Please check if configured items are still correct and click 'Save and update' to update the stored items.<br><button id='hadignore' class='pure-button pure-button-primary'>Ignore and use config</button><button id='hadusenew' class='pure-button pure-button-primary'>Use new config</button>";
      el.classList.remove('hidden');
    }
    else {
      el.classList.add('hidden');
      el.innerHTML = '';
      updateStatusTableFromCompactJson(x);
    }
  },
  i2cdebuglog: function (f) {
    var tbl = $id('I2CLogTable');
    if (!tbl) return;
    tbl.innerHTML = '';
    buildHtmlTablePlain(tbl, f.i2cdebuglog);
  },
  i2csniffer: function (f) {
    var outer = $id('i2clog_outer');
    if (!outer) return;
    outer.classList.remove('hidden');
    $id('i2clog').insertAdjacentHTML('afterbegin', `${new Date().toLocaleString('nl-NL')}: ${f.i2csniffer}<br>`);
  },
  ithodevinfo: function (f) {
    var x = f.ithodevinfo;
    processElements(x);
    localStorage.setItem("itho_setlen", x.itho_setlen);
    localStorage.setItem("itho_devtype", x.itho_devtype);
    localStorage.setItem("itho_mfr", x.itho_mfr);
    localStorage.setItem("itho_deviceid", x.itho_deviceid);
    localStorage.setItem("itho_fwversion", x.itho_fwversion);
    localStorage.setItem("itho_hwversion", x.itho_hwversion);
    if (wizardActive) {
      wizardOnDevInfo(x.itho_devtype, x.itho_mfr, x.itho_deviceid);
    }
  },
  ithosettings: function (f) {
    var x = f.ithosettings;
    updateSettingsLocStor(x);

    if (x.Index === 0 && x.update === false) {
      clearSettingsLocStor();
      localStorage.setItem("ihto_settings_complete", "false");
      var tbl = $id('SettingsTable');
      tbl.innerHTML = '';
      addColumnHeader(x, tbl, true);
      tbl.insertAdjacentHTML('beforeend', '<tbody>');
    }
    if (x.update === false) {
      addRowTableIthoSettings($id('SettingsTable').querySelector('tbody'), x);
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
    $id('wifiscanresult').insertAdjacentHTML('beforeend', `<div class='ScanResults'><input id='${x.id}' class='pure-input-1-5' name='optionsWifi' value='${x.ssid}' type='radio'>${returnSignalSVG(x.sigval)}${returnWifiSecSVG(x.sec)} ${x.ssid}</label></div>`);
  },
  systemstat: function (f) {
    var x = f.systemstat;
    uuid = x.uuid;
    if ('sensor_temp' in x) {
      var stEl = $id('sensor_temp');
      if (stEl) stEl.innerHTML = `Temperature: ${round(x.sensor_temp, 1)}&#8451;`;
    }
    if ('sensor_hum' in x) {
      var shEl = $id('sensor_hum');
      if (shEl) shEl.innerHTML = `Humidity: ${round(x.sensor_hum, 1)}%`;
    }
    // Update demand slider on index page with actual fan demand from 31DA
    if (typeof x.fan_demand !== 'undefined') {
      var ds = $id('rfco2demandslider');
      if (ds && !ds.matches(':active')) { // don't override while user is dragging
        ds.value = x.fan_demand;
        var dl = $id('rfco2demandval');
        if (dl) dl.textContent = x.fan_demand;
      }
    }
    var memBox = $id('memory_box');
    if (memBox) { memBox.style.display = 'block'; memBox.innerHTML = `<p><b>Memory:</b><p><p>free: <b>${x.freemem}</b></p><p>low: <b>${x.memlow}</b></p>`; }
    var mqttEl = $id('mqtt_conn');
    if (mqttEl) {
      var button = returnMqttState(x.mqqtstatus);
      mqttEl.className = `pure-button ${button.button}`;
      mqttEl.textContent = button.state;
    }
    if (wizardActive && wizardMqttInterval) {
      var mqttState = returnMqttState(x.mqqtstatus);
      var statusEl = $id('wiz-mqtt-status');
      if (statusEl) {
        statusEl.textContent = mqttState.state;
        statusEl.className = mqttState.button;
      }
      if (mqttState.state === 'Connected') {
        clearInterval(wizardMqttInterval);
        wizardMqttInterval = null;
        var haEl = $q('input[name="option-mqtt_ha_active"][value="1"]');
        if (haEl) haEl.checked = true;
        wizardUpdateIndicators();
      }
    }
    if (Date.now() - sliderUserInput > 3000) {
      var slider = $id('ithoslider');
      if (slider) slider.value = x.itho;
      var textval = $id('ithotextval');
      if (textval) textval.innerHTML = x.itho;
    }
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
    var initEl = $id('ithoinit');
    if (initEl) initEl.innerHTML = initstatus;
    if ('sensor' in x) sensor = x.sensor;
    var llmEl = $id('itho_llm');
    if (llmEl) {
      if (x.itho_llm > 0) {
        llmEl.className = "pure-button button-success";
        llmEl.textContent = `On ${x.itho_llm}`;
        remotesRefreshNeeded = true;
      }
      else {
        llmEl.className = "pure-button button-secondary";
        llmEl.textContent = "Off";
      }
    }
    var copyEl = $id('itho_copyid_vremote');
    if (copyEl) {
      if (x.copy_id > 0) {
        copyEl.className = "pure-button button-success";
        copyEl.textContent = `Press join. Time remaining: ${x.copy_id}`;
      }
      else {
        copyEl.className = "pure-button";
        copyEl.textContent = "Copy ID";
      }
    }
    if ('format' in x) {
      var fmtEl = $id('format');
      if (fmtEl) fmtEl.textContent = x.format ? 'Format filesystem' : 'Format failed';
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
    var mb = $id('message_box');
    if (!mb) return;
    mb.style.display = 'block';
    mb.insertAdjacentHTML('beforeend', `<p class='messageP' id='mbox_p${count}'>Message: ${x.message}</p>`);
    removeAfter5secs(count);
  },
  rflog: function (f) {
    var rfo = $id('rflog_outer');
    var rfl = $id('rflog');
    if (!rfo || !rfl) return;
    rfo.classList.remove('hidden');
    rfl.insertAdjacentHTML('afterbegin', `${new Date().toLocaleString('nl-NL')}: ${f.rflog.message}<br>`);
  },
  ota: function (f) {
    var x = f.ota;
    var el = $id('updateprg');
    if (!el) return;
    el.innerHTML = `Firmware update progress: ${x.percent}%`;
    moveBar(x.percent, "updateBar");
  },
  sysmessage: function (f) {
    var x = f.sysmessage;
    var el = $id(x.id);
    if (el) el.textContent = x.message;
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

  try {
    for (var key in messageHandlers) {
      if (key in f) {
        messageHandlers[key](f);
        return;
      }
    }
    processElements(f);
  } catch (error) {
    console.error('Error processing message:', error);
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
        $id('downloadsettingsdiv').classList.remove('hidden');
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
        var tbl = $id('SettingsTable');
        tbl.innerHTML = '';
        addColumnHeader(existingData, tbl, true);
        tbl.insertAdjacentHTML('beforeend', '<tbody>');
      }
      let current_tmp = existingData.Current;
      let maximum_tmp = existingData.Maximum;
      let minimum_tmp = existingData.Minimum;
      existingData.Current = null;
      existingData.Maximum = null;
      existingData.Minimum = null;
      addRowTableIthoSettings($id('SettingsTable').querySelector('tbody'), existingData);
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


document.addEventListener('DOMContentLoaded', function () {
  document.getElementById("layout").style.opacity = 0.3;
  document.getElementById("loader").style.display = "block";
  startWebsock();

  // Setup wizard mode
  if (typeof first_boot !== 'undefined' && first_boot) {
    initWizard(1);
  } else if (typeof wizard_step !== 'undefined' && wizard_step > 0) {
    initWizard(wizard_step);
  }

  // Helper: get value of checked radio by name
  function checkedVal(name) {
    var el = $q('input[name=\'' + name + '\']:checked');
    return el ? el.value : undefined;
  }

  // Wizard navigation buttons
  document.addEventListener('click', function (e) {
    var target = e.target;
    if (target.id === 'wizard-next') { e.preventDefault(); wizardNext(); return; }
    if (target.id === 'wizard-back') { e.preventDefault(); wizardBack(); return; }
    if (target.id === 'wizard-finish') { e.preventDefault(); wizardFinish(); return; }
    if (target.id === 'wizard-abort') { e.preventDefault(); wizardAbort(); return; }
    if (target.id === 'wiz-co2-optima') { e.preventDefault(); wizardSetCO2Choice(true); return; }
    if (target.id === 'wiz-co2-rft') { e.preventDefault(); wizardSetCO2Choice(false); return; }
    if (target.id === 'wiz-wifi-connect') {
      e.preventDefault();
      wizardSaveWifiSettings();
      // Show connecting status immediately
      var statEls = document.querySelectorAll('[name="wificonnstat"]');
      statEls.forEach(function (el) { el.textContent = 'Connecting...'; });
      getSettings('wifistat');
      if (wizardConnectInterval) clearInterval(wizardConnectInterval);
      wizardConnectInterval = setInterval(function () { getSettings('wifistat'); }, 2000);
      return;
    }
    if (target.id === 'wiz-mqtt-connect') {
      e.preventDefault();
      wizardSaveMqttSettings();
      getSettings('systemstat');
      if (wizardMqttInterval) clearInterval(wizardMqttInterval);
      wizardMqttInterval = setInterval(function () { getSettings('systemstat'); }, 2000);
      var statusEl = $id('wiz-mqtt-status');
      if (statusEl) { statusEl.textContent = 'Connecting...'; statusEl.className = ''; }
      return;
    }
  });

  //handle menu clicks
  document.addEventListener('click', function (event) {
    var link = event.target.closest('ul.pure-menu-list li a');
    if (link) {
      var page = link.getAttribute('href');
      update_page(page);
      $qa('li.pure-menu-item').forEach(function (el) { el.classList.remove('pure-menu-selected'); });
      link.parentElement.classList.add('pure-menu-selected');
      event.preventDefault();
      return;
    }
    if (event.target.id === 'headingindex' || event.target.closest('#headingindex')) {
      update_page('index');
      $qa('li.pure-menu-item').forEach(function (el) { el.classList.remove('pure-menu-selected'); });
      event.preventDefault();
      return;
    }
  });
  //handle wifi network select
  document.addEventListener('change', function (e) {
    if (e.target.tagName === 'INPUT' && e.target.getAttribute('name') === 'optionsWifi') {
      $id('ssid').value = e.target.value;
    }
  });
  //handle submit buttons
  document.addEventListener('click', function (e) {
    var btn = e.target.closest('button');
    if (!btn) return;
    var btnId = btn.id;
    if (!btnId) return;

    if (btnId.startsWith('command-')) {
      const items = btnId.split('-');
      websock_send(`{"command":"${items[1]}"}`);
    }
    else if (btnId === 'wifisubmit') {
      hostname = $val('hostname');

      const wifi_passwd = $val('passwd');
      const wifi_appasswd = $val('appasswd');

      // Build secure_credentials for changed passwords
      const secure_credentials = {};
      const enc_passwd = secureCredentials.getEncryptedField(wifi_passwd);
      const enc_appasswd = secureCredentials.getEncryptedField(wifi_appasswd);
      if (enc_passwd) secure_credentials.passwd = enc_passwd;
      if (enc_appasswd) secure_credentials.appasswd = enc_appasswd;

      const wifiMsg = {
        wifisettings: {
          ssid: $val('ssid'),
          passwd: enc_passwd ? '********' : wifi_passwd,
          appasswd: enc_appasswd ? '********' : wifi_appasswd,
          dhcp: checkedVal('option-dhcp'),
          ip: $val('ip'),
          subnet: $val('subnet'),
          gateway: $val('gateway'),
          dns1: $val('dns1'),
          dns2: $val('dns2'),
          hostname: $val('hostname'),
          ntpserver: $val('ntpserver'),
          timezone: $val('timezone'),
          aptimeout: $val('aptimeout')
        }
      };
      if (Object.keys(secure_credentials).length > 0) {
        wifiMsg.wifisettings.secure_credentials = secure_credentials;
      }
      websock_send(JSON.stringify(wifiMsg));

      update_page('wifisetup');
    }
    //syssubmit
    else if (btnId === 'syssumbit') {
      if (!isValidJsonArray($val('api_settings_activated'))) {
        alert("error: Activated settings input value is not a valid JSON array!");
        return;
      }
      else {
        if (!areAllUnsignedIntegers(JSON.parse($val('api_settings_activated')))) {
          alert("error: Activated settings array contains non integer values!");
          return;
        }
      }

      const sys_passwd = $val('sys_password');
      const sys_secure_credentials = {};
      const enc_sys = secureCredentials.getEncryptedField(sys_passwd);
      if (enc_sys) sys_secure_credentials.sys_password = enc_sys;

      const sysMsg = {
        systemsettings: {
          sys_username: $val('sys_username'),
          sys_password: enc_sys ? '********' : sys_passwd,
          syssec_web: checkedVal('option-syssec_web'),
          syssec_api: checkedVal('option-syssec_api'),
          syssec_edit: checkedVal('option-syssec_edit'),
          api_normalize: checkedVal('option-api_normalize'),
          api_settings: checkedVal('option-api_settings'),
          api_reboot: checkedVal('option-api_reboot'),
          api_settings_activated: JSON.parse($val('api_settings_activated')),
          syssht30: checkedVal('option-syssht30'),
          itho_rf_support: checkedVal('option-itho_rf_support'),
          itho_control_interface: checkedVal('option-itho_control_interface'),
          itho_rf_co2_join: checkedVal('option-itho_rf_co2_join'),
          itho_fallback: $val('itho_fallback'),
          itho_low: $val('itho_low'),
          itho_medium: $val('itho_medium'),
          itho_high: $val('itho_high'),
          itho_timer1: $val('itho_timer1'),
          itho_timer2: $val('itho_timer2'),
          itho_timer3: $val('itho_timer3'),
          itho_updatefreq: $val('itho_updatefreq'),
          itho_counter_updatefreq: $val('itho_counter_updatefreq'),
          itho_numvrem: $val('itho_numvrem'),
          //itho_numrfrem: $id('itho_numrfrem').value,
          itho_sendjoin: checkedVal('option-itho_sendjoin'),
          itho_forcemedium: checkedVal('option-itho_forcemedium'),
          itho_vremoteapi: checkedVal('option-itho_vremoteapi'),
          itho_pwm2i2c: checkedVal('option-itho_pwm2i2c'),
          itho_31da: checkedVal('option-itho_31da'),
          itho_31d9: checkedVal('option-itho_31d9'),
          itho_2401: checkedVal('option-itho_2401'),
          itho_4210: checkedVal('option-itho_4210'),
          i2c_safe_guard: checkedVal('option-i2c_safe_guard'),
          i2c_sniffer: checkedVal('option-i2c_sniffer')
        }
      };
      if (Object.keys(sys_secure_credentials).length > 0) {
        sysMsg.systemsettings.secure_credentials = sys_secure_credentials;
      }
      websock_send(JSON.stringify(sysMsg));
      update_page('system');
    }
    else if (btnId === 'syslogsubmit') {
      websock_send(JSON.stringify({
        logsettings: {
          loglevel: $val('loglevel'),
          syslog_active: checkedVal('option-syslog_active'),
          esplog_active: checkedVal('option-esplog_active'),
          webserial_active: checkedVal('option-webserial_active'),
          rfloglevel: $val('rfloglevel'),
          logserver: $val('logserver'),
          logport: $val('logport'),
          logref: $val('logref')
        }
      }));
      update_page('syslog');
    }
    //mqttsubmit
    else if (btnId === 'mqttsubmit') {
      const mqtt_passwd = $val('mqtt_password');
      const mqtt_secure_credentials = {};
      const enc_mqtt = secureCredentials.getEncryptedField(mqtt_passwd);
      if (enc_mqtt) mqtt_secure_credentials.mqtt_password = enc_mqtt;

      const mqttMsg = {
        systemsettings: {
          mqtt_active: checkedVal('option-mqtt_active'),
          mqtt_serverName: $val('mqtt_serverName'),
          mqtt_username: $val('mqtt_username'),
          mqtt_password: enc_mqtt ? '********' : mqtt_passwd,
          mqtt_port: $val('mqtt_port'),
          mqtt_version: $val('mqtt_version'),
          mqtt_base_topic: $val('mqtt_base_topic'),
          mqtt_ha_topic: $val('mqtt_ha_topic'),
          mqtt_domoticzin_topic: $val('mqtt_domoticzin_topic'),
          mqtt_domoticzout_topic: $val('mqtt_domoticzout_topic'),
          mqtt_idx: $val('mqtt_idx'),
          sensor_idx: $val('sensor_idx'),
          mqtt_domoticz_active: checkedVal('option-mqtt_domoticz_active'),
          mqtt_ha_active: checkedVal('option-mqtt_ha_active')
        }
      };
      if (Object.keys(mqtt_secure_credentials).length > 0) {
        mqttMsg.systemsettings.secure_credentials = mqtt_secure_credentials;
      }
      websock_send(JSON.stringify(mqttMsg));
      update_page('mqtt');
    }
    else if (btnId === 'itho_llm') {
      // Auto-save remote config before entering learn/leave mode
      // Try selected remote first, then save all visible remotes (wizard mode)
      var sel = checkedVal('optionsRemotes');
      var indices = [];
      if (sel != null) {
        indices.push(parseInt(sel));
      } else {
        // No radio selected (wizard mode) — save all remotes that have visible dropdowns
        for (var ri = 0; ri < 12; ri++) {
          if ($id('func_remote-' + ri)) indices.push(ri);
        }
      }
      for (var k = 0; k < indices.length; k++) {
        var idx = indices[k];
        var funcEl = $id('func_remote-' + idx);
        var remfunc = (!funcEl || typeof funcEl.value === 'undefined') ? 0 : funcEl.value;
        var typeEl = $id('type_remote-' + idx);
        var remtype = (!typeEl || typeof typeEl.value === 'undefined') ? 0 : typeEl.value;
        var nameEl = $id('name_remote-' + idx);
        var name = nameEl ? nameEl.value : 'remote' + idx;
        var id = $id('id_remote-' + idx) ? $id('id_remote-' + idx).value : '00,00,00';
        if (id == 'empty slot') id = "00,00,00";
        if (isHex(id.split(",")[0]) && isHex(id.split(",")[1]) && isHex(id.split(",")[2])) {
          websock_send(`{"itho_update_remote":${idx},"id":[${parseInt(id.split(",")[0], 16)},${parseInt(id.split(",")[1], 16)},${parseInt(id.split(",")[2], 16)}],"value":"${name}","remtype":${remtype},"remfunc":${remfunc}}`);
        }
      }
      setTimeout(function() { websock_send('{"itho_llm":true}'); }, 500);
    }
    else if (btnId === 'itho_remove_remote' || btnId === 'itho_remove_vremote') {
      var selected = checkedVal('optionsRemotes');
      if (selected == null) {
        alert("Please select a remote.")
      }
      else {
        var val = parseInt(selected, 10) + 1;
        remotesRefreshNeeded = true;
        websock_send('{"' + btnId + '":' + val + '}');
      }
    }
    else if (btnId === 'itho_update_remote' || btnId === 'itho_update_vremote') {
      var i = checkedVal('optionsRemotes');
      if (i == null) {
        alert("Please select a remote.");
      }
      else {
        var funcEl = $id('func_remote-' + i);
        var remfunc = (!funcEl || typeof funcEl.value === 'undefined') ? 0 : funcEl.value;
        var typeEl = $id('type_remote-' + i);
        var remtype = (!typeEl || typeof typeEl.value === 'undefined') ? 0 : typeEl.value;
        var id = $id('id_remote-' + i).value;
        if (id == 'empty slot') id = "00,00,00";
        if (isHex(id.split(",")[0]) && isHex(id.split(",")[1]) && isHex(id.split(",")[2])) {
          remotesRefreshNeeded = true;
          var txpEl = $id('txpower-' + i);
          var txp = txpEl ? parseInt(txpEl.value) : undefined;
          var msg = `{"${btnId}":${i},"id":[${parseInt(id.split(",")[0], 16)},${parseInt(id.split(",")[1], 16)},${parseInt(id.split(",")[2], 16)}],"value":"${$id('name_remote-' + i).value}","remtype":${remtype},"remfunc":${remfunc}`;
          if (txp !== undefined) msg += `,"tx_power":${txp}`;
          msg += '}';
          websock_send(msg);
        }
        else {
          alert("ID error, please use HEX notation separated by ',' (ie. 'A1,34,7F')");
        }
      }
    }
    else if (btnId === 'update_rf_id') {
      var id = $id('module_rf_id_str').value;
      if (isHex(id.split(",")[0]) && isHex(id.split(",")[1]) && isHex(id.split(",")[2])) {
        websock_send(`{"update_rf_id":[${parseInt(id.split(",")[0], 16)},${parseInt(id.split(",")[1], 16)},${parseInt(id.split(",")[2], 16)}]}`);
      }
      else {
        alert("ID error, please use HEX notation separated by ',' (ie. 'A1,34,7F')");
      }
    }
    else if (btnId === 'update_num_rf') {
      websock_send(JSON.stringify({
        systemsettings: {
          itho_numrfrem: $val('itho_numrfrem')
        }
      }));
    }
    else if (btnId === 'itho_copyid_vremote') {
      var i = checkedVal('optionsRemotes');
      if (i == null) {
        alert("Please select a remote.");
      }
      else {
        var val = parseInt(i, 10) + 1;
        websock_send(`{"copy_id":true, "index":${val}}`);
      }
    }
    else if (btnId.substr(0, 15) === 'ithosetrefresh-') {
      var row = parseInt(btnId.substr(15));
      var i = checkedVal('options-ithoset');
      if (i == null) {
        alert("Please select a row.");
      }
      else if (i != row) {
        alert("Please select the correct row.");
      }
      else {
        var checkedRadio = $q('input[name=\'options-ithoset\']:checked');
        if (checkedRadio) checkedRadio.checked = false;
        $qa('[id^=ithosetrefresh-]').forEach(function (el, index) {
          var ref = $id('ithosetrefresh-' + index);
          var upd = $id('ithosetupdate-' + index);
          if (ref) ref.classList.remove('pure-button-primary');
          if (upd) upd.classList.remove('pure-button-primary');
        });
        ['Current', 'Minimum', 'Maximum'].forEach(function (k) {
          var el = $id(k + '-' + i);
          if (el) el.innerHTML = "<div style='margin: auto;' class='dot-elastic'></div>";
        });
        websock_send('{"ithosetrefresh":' + i + '}');
      }
    }
    else if (btnId.substr(0, 14) === 'ithosetupdate-') {
      var row = parseInt(btnId.substr(14));
      var i = parseInt(checkedVal('options-ithoset'));
      if (i == null) {
        alert("Please select a row.");
      }
      else if (i != row) {
        alert("Please select the correct row.");
      }

      else {
        if (Number.isInteger(parseFloat($id('name_ithoset-' + i).value))) {
          websock_send(JSON.stringify({
            ithosetupdate: i,
            value: parseInt($id('name_ithoset-' + i).value)
          }));
        }
        else {
          websock_send(JSON.stringify({
            ithosetupdate: i,
            value: parseFloat($id('name_ithoset-' + i).value)
          }));
        }

        var checkedRadio = $q('input[name=\'options-ithoset\']:checked');
        if (checkedRadio) checkedRadio.checked = false;
        $qa('[id^=ithosetrefresh-]').forEach(function (el, index) {
          var ref = $id('ithosetrefresh-' + index);
          var upd = $id('ithosetupdate-' + index);
          if (ref) ref.classList.remove('pure-button-primary');
          if (upd) upd.classList.remove('pure-button-primary');
        });
        ['Current', 'Minimum', 'Maximum'].forEach(function (k) {
          var el = $id(k + '-' + i);
          if (el) el.innerHTML = "<div style='margin: auto;' class='dot-elastic'></div>";
        });
      }
    }
    else if (btnId === 'resetwificonf') {
      if (confirm("This will reset the wifi config to factory default, are you sure?")) {
        websock_send('{"resetwificonf":true}');
      }
    }
    else if (btnId === 'resetsysconf') {
      if (confirm("This will reset the system configs files to factory default, are you sure?")) {
        websock_send('{"resetsysconf":true}');
      }
    }
    else if (btnId === 'resethadconf') {
      if (confirm("This will reset the HA Discovery configs to factory default, are you sure?")) {
        websock_send('{"resethadconf":true}');
        setTimeout(function () { var main = $id('main'); main.innerHTML = ''; main.insertAdjacentHTML('beforeend', html_hadiscovery); execScripts(main); }, 1000);
      }
    }
    else if (btnId === 'saveallconfigs') {
      websock_send('{"saveallconfigs":true}');
    }
    else if (btnId === 'reboot') {
      if (confirm("This will reboot the device, are you sure?")) {
        var rbs = $id('rebootscript');
        rbs.insertAdjacentHTML('beforeend', html_reboot_script);
        execScripts(rbs);
        websock_send('{"reboot":true}');
      }
    }
    else if (btnId === 'format') {
      if (confirm("This will erase all settings, are you sure?")) {
        websock_send('{"format":true}');
        $id('format').textContent = 'Formatting...';
      }
    }
    else if (btnId === 'wifiscan') {
      $qa('.ScanResults').forEach(function (el) { el.remove(); });
      $qa('.hidden').forEach(function (el) { el.classList.remove('hidden'); });
      websock_send('{"wifiscan":true}');
    }
    else if (btnId.startsWith('button_vremote-')) {
      const items = btnId.split('-');
      websock_send(`{"vremote":${items[1]}, "command":"${items[2]}"}`);
    }
    else if (btnId.startsWith('button_remote-')) {
      const items = btnId.split('-');
      websock_send(`{"remote":${items[1]}, "command":"${items[2]}"}`);
    }
    else if (btnId.startsWith('button_sendco2-')) {
      const idx = btnId.split('-')[1];
      const val = $id('co2val-' + idx);
      if (val && val.value) {
        websock_send(`{"rfco2":${val.value}, "rfremoteindex":${idx}}`);
      }
    }
    else if (btnId.startsWith('button_senddemand-')) {
      const idx = btnId.split('-')[1];
      const val = $id('demandval-' + idx);
      if (val && val.value) {
        websock_send(`{"rfdemand":${val.value}, "rfremoteindex":${idx}}`);
      }
    }
    else if (btnId.startsWith('ithobutton-')) {
      const items = btnId.split('-');
      websock_send(`{"ithobutton":"${items[1]}"}`);
      if (items[1] == 'shtreset') $id('i2c_sht_reset').textContent = "Processing...";
    }
    else if (btnId.startsWith('button-')) {
      const items = btnId.split('-');
      websock_send(`{"button":"${items[1]}"}`);
    }
    else if (btnId.startsWith('rfdebug-')) {
      const items = btnId.split('-');
      if (items[1] == 0) $id('rflog_outer').classList.add('hidden');
      if (items[1] > 0) $id('rflog_outer').classList.remove('hidden');
      if (items[1] == 12762) {
        websock_send(`{"rfdebug":${items[1]}, "faninfo":${$id('rfdebug-12762-faninfo').value}, "timer":${$id('rfdebug-12762-timer').value}}`);
      }
      else if (items[1] == 12761) {
        websock_send(`{"rfdebug":${items[1]}, "status":${$id('rfdebug-12761-status').value}, "fault":${$id('rfdebug-12761-fault').value}, "frost":${$id('rfdebug-12761-frost').value}, "filter":${$id('rfdebug-12761-filter').value}}`);

      }
      else {
        websock_send(`{"rfdebug":${items[1]}}`);
      }
    }
    else if (btnId.startsWith('i2csniffer-')) {
      const items = btnId.split('-');
      if (items[1] == 0) $id('i2clog_outer').classList.add('hidden');
      if (items[1] > 0) $id('i2clog_outer').classList.remove('hidden');
      websock_send(`{"i2csniffer":${items[1]}}`);
    }
    else if (btnId === 'button2410') {
      websock_send(JSON.stringify({
        ithobutton: 2410,
        index: parseInt($id('itho_setting_id').value)
      }));
    }
    else if (btnId === 'button2410set') {
      websock_send(JSON.stringify({
        ithobutton: 24109,
        ithosetupdate: parseInt($id('itho_setting_id_set').value),
        value: parseFloat($id('itho_setting_value_set').value)
      }));
    }
    else if (btnId === 'button4210') {
      websock_send(JSON.stringify({
        ithobutton: 4210
      }));
    }
    else if (btnId === 'buttonCE30') {
      websock_send(JSON.stringify({
        ithobutton: 0xCE30,
        ithotemp: parseFloat($id('itho_ce30_temp').value * 100.),
        ithotemptemp: parseFloat($id('itho_ce30_temptemp').value * 100.),
        ithotimestamp: $id('itho_ce30_timestamp').value
      }));
    }
    else if (btnId === 'buttonC000') { //CO2
      websock_send(JSON.stringify({
        ithobutton: 0xC000,
        itho_c000_speed1: Number($id('itho_c000_speed1').value),
        itho_c000_speed2: Number($id('itho_c000_speed2').value)
      }));
    }
    else if (btnId === 'button9298') { //CO2 value
      websock_send(JSON.stringify({
        ithobutton: 0x9298,
        itho_9298_val: Number($id('itho_9298_val').value)
      }));
    }
    else if (btnId === 'button4030') {
      if ($id('itho_4030_password').value == 'thisisunsafe') {
        websock_send(JSON.stringify({
          ithobutton: 4030,
          idx: Number($id('itho_4030_index').value),
          dt: Number($id('itho_4030_datatype').value),
          val: Number($id('itho_4030_value').value),
          chk: Number($id('itho_4030_checked').value),
        }));
      }
    }
    else if (btnId === 'ithogetsettings') {
      if (localStorage.getItem("ihto_settings_complete") == "true" && localStorage.getItem("uuid") == uuid) {
        loadSettingsLocStor();
        $id('settings_cache_load').classList.remove('hidden');
        $id('downloadsettingsdiv').classList.remove('hidden');
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
    else if (btnId === 'ithoforcerefresh') {
      $id('settings_cache_load').classList.add('hidden');
      $id('downloadsettingsdiv').classList.add('hidden');
      settingIndex = 0;
      websock_send(JSON.stringify({
        ithogetsetting: true,
        index: 0,
        update: false
      }));
    }
    else if (btnId === 'downloadsettings') {
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
    else if (btnId === 'updatesubmit') {
      e.preventDefault();
      var form = $id('updateform');
      var data = new FormData(form);
      let filename = data.get('update')?.name;
      if (!filename || !filename.endsWith(".bin")) {
        count += 1;
        resetTimer();
        $id('message_box').style.display = 'block';
        $id('message_box').insertAdjacentHTML('beforeend', `<p class='messageP' id='mbox_p${count}'>Updater: file name error, please select a *.bin firmware file</p>`);
        removeAfter5secs(count);
        return;
      }
      $id('updatesubmit').classList.add('pure-button-disabled');
      $id('updatesubmit').textContent = 'Update in progress...';
      ['uploadProgress', 'updateProgress', 'uploadprg', 'updateprg'].forEach(function (id) { var el = $id(id); if (el) el.style.display = ''; });
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/update', true);
      xhr.upload.addEventListener('progress', function (evt) {
        if (evt.lengthComputable) {
          var per = Math.round(10 + (((evt.loaded / evt.total) * 100) * 0.9));
          $id('uploadprg').innerHTML = 'File upload progress: ' + per + '%';
          moveBar(per, "uploadBar");
        }
      }, false);
      xhr.onload = function () {
        moveBar(100, "updateBar");
        $id('updateprg').innerHTML = 'Firmware update progress: 100%';
        $id('updatesubmit').textContent = 'Update finished';
        $id('time').style.display = '';
        startCountdown();
      };
      xhr.onerror = function () {
        $id('updatesubmit').textContent = 'Update failed';
        $id('time').style.display = '';
        startCountdown();
      };
      xhr.send(data);
    }
    else if (btnId === 'hadreload') {
      ['ithostatusrdy', 'HADiscForm', 'save_update_had'].forEach(function (id) { $id(id).classList.add('hidden'); });
      $id('ithostatusrdy').innerHTML = '';
      setTimeout(function () { getSettings('hadiscinfo'); }, 1000);
    }
    else if (btnId === 'hadusenew') {
      $id('ithostatusrdy').classList.add('hidden');
      $id('ithostatusrdy').innerHTML = '';
    }
    else if (btnId === 'hadignore') {
      $id('ithostatusrdy').classList.add('hidden');
      $id('ithostatusrdy').innerHTML = '';
      let obj = localStorage.getItem("ithostatus");
      updateStatusTableFromCompactJson(JSON.parse(obj));
      localStorage.removeItem("ithostatus");
    }
    else if (btnId === 'save_update_had') {
      var jsonvar = generateCompactJson();
      websock_send(JSON.stringify({
        hadiscsettings: jsonvar
      }));
    }
    e.preventDefault();
    return false;
  });
  //keep the message box on top on scroll
  window.addEventListener('scroll', function () {
    var el = $id('message_box');
    if (el && window.scrollY > 100 && el.style.position !== 'fixed') {
      el.style.position = 'fixed';
      el.style.top = '0px';
    }
  });
});

var timerHandle = setTimeout(function () {
  $id('message_box').style.display = 'none';
}, 5000);

function resetTimer() {
  window.clearTimeout(timerHandle);
  timerHandle = setTimeout(function () {
    $id('message_box').style.display = 'none';
  }, 5000);
}

function removeID(id) {
  var el = $id(id);
  if (el) el.remove();
}

function processElements(x) {
  for (var key in x) {
    if (x.hasOwnProperty(key)) {
      if (Array.isArray(x[key])) {
        x[key] = JSON.stringify(x[key]);
      }
      var el = $id(key);
      if (el && (el.matches('input') || el.matches('select'))) {
        if (el.value !== x[key]) {
          el.value = x[key];
        }
      }
      else if (el && el.matches('span')) {
        if (el.textContent !== x[key]) {
          el.textContent = x[key];
        }
      }
      else if (el && el.matches('a')) {
        el.href = x[key];
      }
      else {
        var radios = $qa(`input[name='option-${key}']`);
        if (radios[1]) {
          if (![...radios].some(r => r.checked)) {
            var target = document.querySelector(`input[name='option-${key}'][value='${x[key]}']`);
            if (target) target.checked = true;
          }
          radio(key, x[key]);
        }
      }
      $qa(`[name='${key}']`).forEach(function(nameEl) {
        if (nameEl.matches('span')) {
          if (nameEl.textContent !== x[key]) {
            nameEl.textContent = x[key];
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
      $id('dblog').innerHTML = res;
    }
  }
  xhr.onerror = (e) => {
    $id('dblog').innerHTML = xhr.statusText;
  };
  xhr.send(null);
}

function radio(origin, state) {
  if (origin == "dhcp") {
    var readOnly = (state == 'on');
    ['ip', 'subnet', 'gateway', 'dns1', 'dns2'].forEach(function (id) { var el = $id(id); if (el) el.readOnly = readOnly; });
    var portEl = $id('port');
    if (portEl) portEl.readOnly = false;
    var dhcpOn = $id('option-dhcp-on');
    var dhcpOff = $id('option-dhcp-off');
    if (dhcpOn) dhcpOn.disabled = false;
    if (dhcpOff) dhcpOff.disabled = false;
  }
  else if (origin == "mqtt_active") {
    var enabled = (state == 1);
    ['mqtt_serverName', 'mqtt_username', 'mqtt_password', 'mqtt_port', 'mqtt_base_topic', 'mqtt_ha_topic', 'mqtt_domoticzin_topic', 'mqtt_domoticzout_topic', 'mqtt_idx', 'sensor_idx'].forEach(function (id) { var el = $id(id); if (el) el.readOnly = !enabled; });
    ['option-mqtt_domoticz_active-0', 'option-mqtt_domoticz_active-1', 'option-mqtt_ha_active-1', 'option-mqtt_ha_active-0'].forEach(function (id) { var el = $id(id); if (el) el.disabled = !enabled; });
  }
  else if (origin == "mqtt_domoticz_active") {
    var display = (state == 1) ? '' : 'none';
    ['mqtt_domoticzin_topic', 'label-mqtt_domoticzin', 'label-mqtt_domoticzout', 'mqtt_domoticzout_topic', 'mqtt_idx', 'label-mqtt_idx', 'sensor_idx', 'label-sensor_idx'].forEach(function (id) { var el = $id(id); if (el) el.style.display = display; });
  }
  else if (origin == "mqtt_ha_active") {
    var display = (state == 1) ? '' : 'none';
    ['mqtt_ha_topic', 'label-mqtt_ha'].forEach(function (id) { var el = $id(id); if (el) el.style.display = display; });
  }
  else if (origin == "remote" || origin == "ithoset") {
    $qa(`[id^=name_${origin}-]`).forEach(function (el, index) {
      el.readOnly = (index != state);
    });
    $qa(`[id^=type_${origin}-]`).forEach(function (el, index) {
      el.disabled = (index != state);
    });
    $qa(`[id^=func_${origin}-]`).forEach(function (el, index) {
      el.disabled = (index != state);
    });
    $qa(`[id^=id_${origin}-]`).forEach(function (el, index) {
      el.readOnly = (index != state);
    });
    if (origin == "remote") {
      $qa('[id^=txpower-]').forEach(function (el, index) {
        el.disabled = (index != state);
      });
    }
    if (origin == "ithoset") {
      $qa('[id^=ithosetrefresh-]').forEach(function (el, index) {
        var ref = $id('ithosetrefresh-' + index);
        var upd = $id('ithosetupdate-' + index);
        if (ref) ref.classList.remove('pure-button-primary');
        if (upd) upd.classList.remove('pure-button-primary');
        if (index == state) {
          if (ref) ref.classList.add('pure-button-primary');
          if (upd) upd.classList.add('pure-button-primary');
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
  sliderUserInput = Date.now();
  $id('ithotextval').innerHTML = val;
  websock_send(JSON.stringify({ 'itho': val }));
}

//function to load html main content
var lastPageReq = "";
function update_page(page) {
  lastPageReq = page;
  sessionStorage.setItem('currentPage', page);
  clearTimeout(wifistat_to);
  clearTimeout(statustimer_to);
  clearTimeout(ithostatus_to);
  clearTimeout(hastatus_to);
  clearTimeout(rfstatus_to);
  if (remotesRefreshInterval) { clearInterval(remotesRefreshInterval); remotesRefreshInterval = null; }
  var main = $id('main');
  main.innerHTML = '';
  main.style.maxWidth = '768px';
  if (page == 'index') { main.insertAdjacentHTML('beforeend', html_index); }
  if (page == 'wifisetup') { main.insertAdjacentHTML('beforeend', html_wifisetup); }
  if (page == 'syslog') { main.insertAdjacentHTML('beforeend', html_syslog); }
  if (page == 'system') {
    main.insertAdjacentHTML('beforeend', html_systemsettings_start);
    var sf = $id('sys_fieldset');
    if (hw_revision == "2" || hw_revision.startsWith('NON-CVE ')) { sf.insertAdjacentHTML('beforeend', html_systemsettings_cc1101); }
    sf.insertAdjacentHTML('beforeend', html_systemsettings_end);
  }
  if (page == 'itho') { main.insertAdjacentHTML('beforeend', html_ithosettings); }
  if (page == 'status') { main.insertAdjacentHTML('beforeend', html_ithostatus); }
  if (page == 'remotes') { main.insertAdjacentHTML('beforeend', html_remotessetup); }
  if (page == 'rfstatus') { main.insertAdjacentHTML('beforeend', html_rfstatus); }
  if (page == 'vremotes') { main.insertAdjacentHTML('beforeend', html_vremotessetup); }
  if (page == 'mqtt') { main.insertAdjacentHTML('beforeend', html_mqttsetup); }
  if (page == 'api') { main.insertAdjacentHTML('beforeend', html_api); }
  if (page == 'swagger') { main.insertAdjacentHTML('beforeend', html_swagger); main.style.maxWidth = '1600px'; }
  if (page == 'help') { main.insertAdjacentHTML('beforeend', html_help); }
  if (page == 'reset') { main.insertAdjacentHTML('beforeend', html_reset); }
  if (page == 'update') { main.insertAdjacentHTML('beforeend', html_update); }
  if (page == 'debug') { main.insertAdjacentHTML('beforeend', html_debug); main.style.maxWidth = '1600px'; }
  if (page == 'i2cdebug') { main.insertAdjacentHTML('beforeend', html_i2cdebug); }
  if (page == 'hadiscovery') { main.insertAdjacentHTML('beforeend', html_hadiscovery); }
  if (page == 'wizard') { main.insertAdjacentHTML('beforeend', html_wizard); }
  execScripts(main);
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

function addRemoteButtons(container, remfunc, remtype, vremotenum, seperator) {
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
            container.insertAdjacentHTML('beforeend', newinner);
            newinner = '';
          }
        }
        else {
          container.insertAdjacentHTML('beforeend', newinner);
          newinner = '';
        }

      }
    }
  }
}

function addvRemoteInterface(remtype) {
  var elem = $id('reminterface');
  if (!elem) return;
  elem.innerHTML = '';
  var devtype = localStorage.getItem('itho_devtype') || '';
  var remfunc = (devtype === 'HRU 250-300') ? 1 : 2;
  addRemoteButtons(elem, remfunc, remtype, 0, true);
}

function buildHtmlTablePlain(table, jsonVar) {
  if (!jsonVar || jsonVar.length === 0) return;
  var columns = [];
  var thead = document.createElement('thead');
  var headerTr = document.createElement('tr');

  for (var key in jsonVar[0]) {
    columns.push(key);
    var th = document.createElement('th');
    th.innerHTML = key;
    headerTr.appendChild(th);
  }

  thead.appendChild(headerTr);

  var tbody = document.createElement('tbody');
  for (var i = 0; i < jsonVar.length; i++) {
    var row = document.createElement('tr');
    for (var colIndex = 0; colIndex < columns.length; colIndex++) {
      var cellValue = jsonVar[i][columns[colIndex]];
      if (cellValue == null) cellValue = "";
      var td = document.createElement('td');
      td.innerHTML = cellValue;
      row.appendChild(td);
    }
    tbody.appendChild(row);
  }

  table.appendChild(thead);
  table.appendChild(tbody);
}


function generateRemoteID(index) {
  var rfIdEl = $id('module_rf_id_str');
  if (!rfIdEl || !rfIdEl.value) return 'empty slot';
  var parts = rfIdEl.value.split(',');
  if (parts.length < 3) return 'empty slot';
  var b0 = parseInt(parts[0], 16);
  var b1 = parseInt(parts[1], 16);
  var b2 = (parseInt(parts[2], 16) + index) & 0xFF;
  // Check for collisions with existing remotes
  for (var attempt = 0; attempt < 255; attempt++) {
    var candidate = b0.toString(16).toUpperCase() + ',' + b1.toString(16).toUpperCase() + ',' + ((b2 + attempt) & 0xFF).toString(16).toUpperCase();
    var collision = false;
    for (var j = 0; j < remotesCount; j++) {
      var el = $id('id_remote-' + j);
      if (el && el.value === candidate) { collision = true; break; }
    }
    if (!collision) return candidate;
  }
  return 'empty slot';
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
    var capsCell = $id('caps-' + i);
    if (capsCell && remote["capabilities"]) {
      capsCell.innerHTML = formatCapabilities(remote["capabilities"]);
    }
  }
}

function buildHtmlTableRemotes(table, remfunc, jsonVar) {
  if (!jsonVar || jsonVar.length === 0) return;

  var isWizard = wizardActive;
  var data = isWizard ? jsonVar.slice(0, 1) : jsonVar;

  var thead = document.createElement('thead');
  var headerTr = document.createElement('tr');
  if (!isWizard) {
    var selectTh = document.createElement('th');
    selectTh.innerHTML = 'Select';
    headerTr.appendChild(selectTh);
  }

  for (var key in jsonVar[0]) {
    var append = [];
    if (key === "index") {
      append = ["Index", !isWizard];
    }
    else if (key === "id") {
      append = ["ID", true];
    }
    else if (key === "name") {
      append = ["Name", true];
    }
    else if (key === "remfunc" && (remfunc == 1 || isWizard)) {
      append = ["Remote function", true];
    }
    else if (key === "remtype") {
      append = ["Remote model", true];
    }
    else if (key === "capabilities") {
      append = ["Capabilities", !isWizard];
    }
    if (append[1]) {
      var th = document.createElement('th');
      th.innerHTML = append[0];
      headerTr.appendChild(th);
    }
  }

  thead.appendChild(headerTr);
  table.appendChild(thead);

  var tbody = document.createElement('tbody');
  remotesCount = data.length;

  for (const remote of data) {
    let i = 0;
    if (remote["index"]) i = remote["index"];
    var remtype = 0;
    var remfunction = 0;
    var row = document.createElement('tr');
    if (!isWizard) {
      var selectTd = document.createElement('td');
      selectTd.innerHTML = `<input type='radio' id='option-select_remote-${i}' name='optionsRemotes' onchange='radio("remote",${i})' value='${i}' />`;
      row.appendChild(selectTd);
    }

    for (const key in remote) {
      const value = remote[key];

      if (key === "index") {
        if (!isWizard) {
          var td = document.createElement('td');
          td.innerHTML = value.toString();
          row.appendChild(td);
        }
      }
      else if (key === "id") {
        var cellValue = value.toString();
        if (cellValue == null) cellValue = '';
        cellValue = `${value[0].toString(16).toUpperCase()},${value[1].toString(16).toUpperCase()},${value[2].toString(16).toUpperCase()}`;
        if (cellValue == "0,0,0") cellValue = "empty slot";
        var idval = `id_remote-${i}`;
        var td = document.createElement('td');
        td.innerHTML = `<input type='text' id='${idval}' value='${cellValue}' data-saved-id='${cellValue}'${isWizard ? '' : " readonly=''"} />`;
        row.appendChild(td);
      }
      else if (key === "name") {
        var cellValue = value.toString();
        if (cellValue == null) cellValue = '';
        var idval = `name_remote-${i}`;
        var td = document.createElement('td');
        td.innerHTML = `<input type='text' id='${idval}' value='${cellValue}'${isWizard ? '' : " readonly=''"} />`;
        row.appendChild(td);
      }
      else if (key === "remfunc") {
        remfunction = value;
        if (remfunction != 2 || isWizard) { //do not add remote function is remfunction == virtual remote
          var select = document.createElement('select');
          select.name = remfunction;
          select.id = `func_remote-${i}`;
          select.disabled = !isWizard;
          select.addEventListener('change', function () {
            var idEl = $id('id_remote-' + i);
            if (!idEl) return;
            if (this.value == 5) {
              // Send mode: restore saved ID or generate new one
              var savedId = idEl.dataset.savedId;
              if (savedId && savedId !== 'empty slot' && savedId !== '0,0,0') {
                idEl.value = savedId;
              } else if (idEl.value === 'empty slot' || idEl.value === '0,0,0' || idEl.value === '') {
                idEl.value = generateRemoteID(i);
              }
            } else {
              // Receive/Monitor: save current ID and show empty
              if (idEl.value !== 'empty slot' && idEl.value !== '0,0,0' && idEl.value !== '') {
                idEl.dataset.savedId = idEl.value;
              }
              idEl.value = 'empty slot';
            }
          });
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
          var td = document.createElement('td');
          td.appendChild(select);
          row.appendChild(td);
        }
      }
      else if (key === "remtype") {
        var cellValue = value;
        var select = document.createElement('select');
        select.name = cellValue;
        select.id = `type_remote-${i}`;
        select.disabled = !isWizard;
        var wizardHru = isWizard && wizardDeviceCategory === 'hru250_300';
        for (const item of remtypes) {
          if (wizardHru && item[1] !== 0x22F1 && item[1] !== 0x22F3) continue;
          var option = document.createElement('option');
          option.value = item[1];
          option.text = item[0];
          if (item[1] == cellValue) {
            option.selected = true;
            remtype = cellValue;
          }
          select.appendChild(option);
        }
        var td = document.createElement('td');
        td.appendChild(select);
        row.appendChild(td);
      }
      else if (key === "capabilities") {
        if (!isWizard) {
          var td = document.createElement('td');
          if (remfunction == 2 || remfunction == 5) {
            addRemoteButtons(td, remfunc, remtype, i, false);
            if (remfunction == 5 && remtype == 0x1298) {
              td.insertAdjacentHTML('beforeend', `<br><input type="number" id="co2val-${i}" min="0" max="10000" placeholder="CO2 ppm" style="width:90px;margin-top:4px;"> <button id="button_sendco2-${i}" class="pure-button">Send CO2</button>`);
              td.insertAdjacentHTML('beforeend', `<br><label style="font-size:0.85em;">Demand: <span id="demandlabel-${i}">0</span>/200</label><input type="range" id="demandval-${i}" min="0" max="200" value="0" style="width:150px;" oninput="$id('demandlabel-'+${i}).textContent=this.value" onchange="websock_send(JSON.stringify({rfdemand:parseInt(this.value),rfremoteindex:${i}}))">`);

            }
            if (remfunction == 5) {
              var curPower = remote["tx_power"] || 192;
              td.insertAdjacentHTML('beforeend',
                `<br><label style="font-size:0.85em;">TX Power: </label>` +
                `<select id="txpower-${i}" style="font-size:0.85em;">` +
                `<option value="3"${curPower==3?' selected':''}>-30 dBm (min)</option>` +
                `<option value="38"${curPower==38?' selected':''}>-15 dBm</option>` +
                `<option value="81"${curPower==81?' selected':''}>-10 dBm</option>` +
                `<option value="52"${curPower==52?' selected':''}>-6 dBm</option>` +
                `<option value="96"${curPower==96?' selected':''}>0 dBm</option>` +
                `<option value="132"${curPower==132?' selected':''}>+5 dBm</option>` +
                `<option value="197"${curPower==197?' selected':''}>+7 dBm</option>` +
                `<option value="192"${curPower==192?' selected':''}>+10 dBm (max)</option>` +
                `</select>`);
            }
          }
          else {
            td.id = `caps-${i}`;
            td.innerHTML = formatCapabilities(value);
          }
          row.appendChild(td);
        }
      }
      tbody.appendChild(row);
    }
  }
  table.appendChild(tbody);
  // TX power dropdowns disabled until remote is selected
  if (!isWizard) {
    $qa('[id^=txpower-]').forEach(function (el) { el.disabled = true; });
  }
}

function buildHtmlStatusTable(table, jsonVar) {
  var thead = document.createElement('thead');
  var headerTr = document.createElement('tr');
  ['Index', 'Label', 'Value'].forEach(function (text) {
    var th = document.createElement('th');
    th.innerHTML = text;
    headerTr.appendChild(th);
  });
  thead.appendChild(headerTr);
  table.appendChild(thead);

  var tbody = document.createElement('tbody');

  Object.entries(jsonVar).forEach(([key, value], index) => {
    var row = document.createElement('tr');
    [index, key, value].forEach(function (text) {
      var td = document.createElement('td');
      td.textContent = text;
      row.appendChild(td);
    });
    tbody.appendChild(row);
  });

  table.appendChild(tbody);
}

function addRowTableIthoSettings(tbody, jsonVar) {
  var i = jsonVar.Index;
  var row = document.createElement('tr');
  var selectTd = document.createElement('td');
  selectTd.className = 'ithoset';
  selectTd.style.textAlign = 'center';
  selectTd.style.verticalAlign = 'middle';
  selectTd.innerHTML = `<input type='radio' id='option-select_ithoset-${i}' name='options-ithoset' onchange='radio("ithoset",${i})' value='${i}' />`;
  row.appendChild(selectTd);
  for (var key in jsonVar) {
    if (key == "update") { continue; }
    if (key == "loop") { continue; }
    var td = document.createElement('td');
    td.className = 'ithoset';
    if (jsonVar[key] == null) {
      td.id = key + '-' + i;
      td.innerHTML = "<div style='margin: auto;' class='dot-elastic'></div>";
    }
    else {
      td.innerHTML = jsonVar[key];
    }
    row.appendChild(td);
  }
  var updTd = document.createElement('td');
  updTd.className = 'ithoset';
  updTd.innerHTML = `<button id='ithosetupdate-${i}' class='pure-button'>Update</button>`;
  row.appendChild(updTd);
  var refTd = document.createElement('td');
  refTd.className = 'ithoset';
  refTd.innerHTML = `<button id='ithosetrefresh-${i}' class='pure-button'>Refresh</button>`;
  row.appendChild(refTd);

  tbody.appendChild(row);
}

function updateRowTableIthoSettings(jsonVar) {
  var i = jsonVar.Index;
  for (var key in jsonVar) {
    if (key == "Current") {
      var el = $id(key + '-' + i);
      if (el) el.innerHTML = `<input type='text' id='name_ithoset-${i}' value='${jsonVar[key]}' readonly='' />`;
    }
    if (key == "Minimum" || key == "Maximum") {
      var el = $id(key + '-' + i);
      if (el) el.innerHTML = jsonVar[key];
    }
  }
}

function addColumnHeader(jsonVar, table, appendRow) {
  var columnSet = [];
  var thead = document.createElement('thead');
  var headerTr = document.createElement('tr');
  var selectTh = document.createElement('th');
  selectTh.className = 'ithoset';
  selectTh.innerHTML = 'Select';
  headerTr.appendChild(selectTh);

  for (var key in jsonVar) {
    if (key == "update") { continue; }
    if (key == "loop") { continue; }
    if (columnSet.indexOf(key) == -1) {
      columnSet.push(key);
      var th = document.createElement('th');
      th.className = 'ithoset';
      th.innerHTML = key;
      headerTr.appendChild(th);
    }
  }
  var spacer1 = document.createElement('th');
  spacer1.className = 'ithoset';
  spacer1.innerHTML = '&nbsp;';
  headerTr.appendChild(spacer1);
  var spacer2 = document.createElement('th');
  spacer2.className = 'ithoset';
  spacer2.innerHTML = '&nbsp;';
  headerTr.appendChild(spacer2);

  thead.appendChild(headerTr);
  if (appendRow) {
    table.appendChild(thead);
  }
  return columnSet;
}

function buildHtmlHADiscTable(ithostatusinfo) {
  const advancedToggleId = "advancedModeToggle";

  // Set default value for device type input field
  document.getElementById("hadevicename").value = ha_dev_name;

  var thead = document.createElement('thead');
  var headerTr = document.createElement('tr');

  var selectAllCb = document.createElement('input');
  selectAllCb.type = 'checkbox';
  selectAllCb.id = 'selectAllStatus';
  var includeTh = document.createElement('th');
  includeTh.appendChild(selectAllCb);
  includeTh.append(' Include');
  headerTr.appendChild(includeTh);

  ['Label', 'HA Name'].forEach(function (text) {
    var th = document.createElement('th');
    th.innerHTML = text;
    headerTr.appendChild(th);
  });
  ['Device Class', 'State Class', 'Value Template', 'Unit of Measurement'].forEach(function (text) {
    var th = document.createElement('th');
    th.className = 'advanced hidden';
    th.innerHTML = text;
    headerTr.appendChild(th);
  });
  thead.appendChild(headerTr);
  $id('HADiscTable').appendChild(thead);

  // Select all / deselect all
  selectAllCb.addEventListener('change', function () {
    var checked = this.checked;
    $qa("#HADiscTable tbody tr td:first-child input[type='checkbox']").forEach(function (cb) { cb.checked = checked; });
  });

  var tbody = document.createElement('tbody');

  // Build table rows from ithostatusinfo
  Object.entries(ithostatusinfo).forEach(([key, value], index) => {
    var row = document.createElement('tr');

    // Include checkbox (default unchecked)
    var includeCheckbox = document.createElement('input');
    includeCheckbox.type = 'checkbox';
    includeCheckbox.checked = false;
    includeCheckbox.dataset.field = 'include';
    var includeTd = document.createElement('td');
    includeTd.appendChild(includeCheckbox);
    row.appendChild(includeTd);

    // Label (non-editable)
    var labelTd = document.createElement('td');
    labelTd.textContent = key;
    row.appendChild(labelTd);

    // HA Name (editable, unit of measurement removed)
    const unitMatch = key.match(/\(([^)]+)\)$/);
    const cleanName = key.replace(/\s*\([^)]+\)$/, "");
    var nameInput = document.createElement('input');
    nameInput.type = 'text';
    nameInput.value = cleanName;
    nameInput.placeholder = 'Name';
    nameInput.dataset.field = 'name';
    var nameTd = document.createElement('td');
    nameTd.appendChild(nameInput);
    row.appendChild(nameTd);

    // Device Class (editable, advanced)
    var deviceClassInput = document.createElement('input');
    deviceClassInput.type = 'text';
    deviceClassInput.className = 'advanced hidden';
    deviceClassInput.placeholder = 'Device Class';
    deviceClassInput.dataset.field = 'dc';
    var dcTd = document.createElement('td');
    dcTd.className = 'advanced hidden';
    dcTd.appendChild(deviceClassInput);
    row.appendChild(dcTd);

    // State Class (editable, advanced)
    var stateClassInput = document.createElement('input');
    stateClassInput.type = 'text';
    stateClassInput.className = 'advanced hidden';
    stateClassInput.placeholder = 'State Class';
    stateClassInput.dataset.field = 'sc';
    var scTd = document.createElement('td');
    scTd.className = 'advanced hidden';
    scTd.appendChild(stateClassInput);
    row.appendChild(scTd);

    // Value Template (editable, advanced, retains unit)
    var valueTemplateInput = document.createElement('input');
    valueTemplateInput.type = 'text';
    valueTemplateInput.className = 'advanced hidden';
    valueTemplateInput.placeholder = `{{ value_json['${key}'] }}`;
    valueTemplateInput.value = `{{ value_json['${key}'] }}`;
    valueTemplateInput.dataset.default = `{{ value_json['${key}'] }}`;
    valueTemplateInput.dataset.field = 'vt';
    var vtTd = document.createElement('td');
    vtTd.className = 'advanced hidden';
    vtTd.appendChild(valueTemplateInput);
    row.appendChild(vtTd);

    // Unit of Measurement (editable, advanced, pre-filled if present)
    var unitInput = document.createElement('input');
    unitInput.type = 'text';
    unitInput.className = 'advanced hidden';
    unitInput.placeholder = 'Unit of Measurement';
    unitInput.value = unitMatch ? unitMatch[1] : '';
    unitInput.dataset.field = 'um';
    var umTd = document.createElement('td');
    umTd.className = 'advanced hidden';
    umTd.appendChild(unitInput);
    row.appendChild(umTd);

    tbody.appendChild(row);
  });

  $id('HADiscTable').appendChild(tbody);

  // Advanced toggle
  $id(advancedToggleId).addEventListener('change', function () {
    var showAdvanced = this.checked;
    $qa('.advanced').forEach(function (el) { el.classList.toggle('hidden', !showAdvanced); });
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
  const container = $id('rfdevices-container');
  container.innerHTML = '';

  if (!rfdevices || rfdevices.length === 0) {
    $id('rfdevices-header').classList.add('hidden');
    return;
  }

  $id('rfdevices-header').classList.remove('hidden');

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

    container.insertAdjacentHTML('beforeend', deviceHtml);
  });
}

function buildVirtualRemotesUI(vrdevices) {
  vrDevicesData = vrdevices || [];
  const container = $id('vrdevices-container');
  container.innerHTML = '';

  if (!vrdevices || vrdevices.length === 0) {
    $id('vrdevices-header').classList.add('hidden');
    return;
  }

  $id('vrdevices-header').classList.remove('hidden');

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

    container.insertAdjacentHTML('beforeend', deviceHtml);
  });
}

function updateStatusTableFromCompactJson(compactJson) {
  ha_dev_name = compactJson.d;
  document.getElementById("hadevicename").value = ha_dev_name;

  const rows = $qa("#HADiscTable tbody tr");
  if (rows.length == 0) return;

  if (compactJson.c && compactJson.c.length > 0) {
    compactJson.c.forEach((component) => {
      const row = rows[component.i];
      if (!row) return;
      var incl = row.querySelector("input[data-field='include']");
      if (incl) incl.checked = true;
      var nameEl = row.querySelector("input[data-field='name']");
      if (nameEl) nameEl.value = component.n;
      if (component.dc) { var el = row.querySelector("input[data-field='dc']"); if (el) el.value = component.dc; }
      if (component.sc) { var el = row.querySelector("input[data-field='sc']"); if (el) el.value = component.sc; }
      if (component.vt) { var el = row.querySelector("input[data-field='vt']"); if (el) el.value = component.vt; }
      if (component.um) { var el = row.querySelector("input[data-field='um']"); if (el) el.value = component.um; }
    });
  }

  // Restore RF device sensor and fan selections
  if (compactJson.rf) {
    compactJson.rf.forEach((rfConfig) => {
      var idx = rfConfig.idx;
      (rfConfig.sensors || []).forEach(s => { var el = $id('rf_' + idx + '_' + s); if (el) el.checked = true; });
      if (rfConfig.fan) { var el = $id('rf_' + idx + '_fan'); if (el) el.checked = true; }
    });
  }

  // Restore virtual remote fan selections
  if (compactJson.vr) {
    compactJson.vr.forEach((vrConfig) => {
      if (vrConfig.fan) { var el = $id('vr_' + vrConfig.idx + '_fan'); if (el) el.checked = true; }
    });
  }
}

function generateCompactJson() {
  const rows = $qa("#HADiscTable tbody tr");
  const components = [];

  rows.forEach(function (row, index) {
    var incl = row.querySelector("input[data-field='include']");
    if (!incl || !incl.checked) return;

    var vtInput = row.querySelector("input[data-field='vt']");
    var nameInput = row.querySelector("input[data-field='name']");
    var component = { i: index, n: nameInput ? nameInput.value : '' };
    var dcInput = row.querySelector("input[data-field='dc']");
    var scInput = row.querySelector("input[data-field='sc']");
    var dc = dcInput ? dcInput.value : '';
    var sc = scInput ? scInput.value : '';
    var vt = vtInput ? vtInput.value : '';
    var umInput = row.querySelector("input[data-field='um']");
    var um = umInput ? umInput.value : '';
    if (dc) component.dc = dc;
    if (sc) component.sc = sc;
    if (vt && vtInput && vt !== vtInput.dataset.default) component.vt = vt;
    if (um) component.um = um;
    components.push(component);
  });

  // Collect RF device sensor and fan selections
  const rfSelections = [];
  rfDevicesData.forEach(device => {
    var sensors = [];
    $qa(`input[data-rfidx="${device.idx}"][data-sensor]:checked`).forEach(function (el) {
      sensors.push(el.dataset.sensor);
    });
    var fanEl = $id('rf_' + device.idx + '_fan');
    var fanEnabled = fanEl && fanEl.checked;
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
    var fanEl = $id('vr_' + device.idx + '_fan');
    if (fanEl && fanEl.checked) {
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

//
// Setup Wizard
//
var wizardActive = false;
var wizardCurrentStep = 1;
var wizardHasRF = false;
var wizardDeviceCategory = 'other';
var wizardDevType = '';
var wizardIsOptimaInside = false;
var wizardSimDevType = null;
var wizardRemotesInterval = null;
var wizardWifistatInterval = null;
var wizardConnectInterval = null;
var wizardMqttInterval = null;
var wizardRfCo2Device = false;
var wizardHru250_300 = false;

function initWizard(startStep) {
  wizardActive = true;
  $id('menu').style.display = 'none';
  $id('menuLink').style.display = 'none';
  wizardCurrentStep = startStep || 1;
  // Save initial state so wizard persists across reboots
  websock_send('{"wizardsave":' + wizardCurrentStep + '}');
  wizardGoTo(wizardCurrentStep);
}

function wizardGetVisibleSteps() {
  var steps = [1, 2];
  if (wizardHasRF) steps.push(3);
  steps.push(4);
  var haEl = $q('input[name="option-mqtt_ha_active"]:checked');
  if (haEl && haEl.value == '1') steps.push(5);
  return steps;
}

function wizardUpdateIndicators() {
  var visible = wizardGetVisibleSteps();
  $qa('#wizard-indicators li').forEach(function (li) {
    var step = parseInt(li.getAttribute('data-step'));
    if (visible.indexOf(step) === -1) {
      li.style.display = 'none';
    } else {
      li.style.display = '';
      li.classList.remove('active', 'done');
      if (step === wizardCurrentStep) li.classList.add('active');
      else if (step < wizardCurrentStep) li.classList.add('done');
    }
  });
  // Show/hide nav buttons
  var idx = visible.indexOf(wizardCurrentStep);
  var backEl = $id('wizard-back');
  if (backEl) backEl.style.display = (idx > 0) ? '' : 'none';
  var isLast = (idx === visible.length - 1);
  var nextEl = $id('wizard-next');
  if (nextEl) nextEl.style.display = isLast ? 'none' : '';
  var finishEl = $id('wizard-finish');
  if (finishEl) finishEl.style.display = isLast ? '' : 'none';
}

function wizardSaveWifiSettings() {
  var wifi_passwd = $val('passwd');
  var wifi_appasswd = $val('appasswd');
  var secure_credentials = {};
  var enc_passwd = secureCredentials.getEncryptedField(wifi_passwd);
  var enc_appasswd = secureCredentials.getEncryptedField(wifi_appasswd);
  if (enc_passwd) secure_credentials.passwd = enc_passwd;
  if (enc_appasswd) secure_credentials.appasswd = enc_appasswd;

  var dhcpEl = $q('input[name="option-dhcp"]:checked');
  var wifiMsg = {
    wifisettings: {
      ssid: $val('ssid'),
      passwd: enc_passwd ? '********' : wifi_passwd,
      appasswd: enc_appasswd ? '********' : wifi_appasswd,
      dhcp: dhcpEl ? dhcpEl.value : 'on',
      ip: $val('ip'),
      subnet: $val('subnet'),
      gateway: $val('gateway'),
      dns1: $val('dns1'),
      dns2: $val('dns2'),
      hostname: $val('hostname'),
      ntpserver: $val('ntpserver'),
      timezone: $val('timezone'),
      aptimeout: $val('aptimeout')
    }
  };
  if (Object.keys(secure_credentials).length > 0) {
    wifiMsg.wifisettings.secure_credentials = secure_credentials;
  }
  websock_send(JSON.stringify(wifiMsg));
}

function wizardSaveMqttSettings() {
  var mqtt_passwd = $val('mqtt_password');
  var mqtt_secure_credentials = {};
  var enc_mqtt = secureCredentials.getEncryptedField(mqtt_passwd);
  if (enc_mqtt) mqtt_secure_credentials.mqtt_password = enc_mqtt;

  var mqttMsg = {
    systemsettings: {
      mqtt_active: '1',
      mqtt_serverName: $val('mqtt_serverName'),
      mqtt_username: $val('mqtt_username'),
      mqtt_password: enc_mqtt ? '********' : mqtt_passwd,
      mqtt_port: $val('mqtt_port'),
      mqtt_base_topic: $val('mqtt_base_topic')
    }
  };
  if (Object.keys(mqtt_secure_credentials).length > 0) {
    mqttMsg.systemsettings.secure_credentials = mqtt_secure_credentials;
  }
  websock_send(JSON.stringify(mqttMsg));
}

function wizardGoTo(step) {
  // Save WiFi settings when leaving step 1 (so they persist across reboots)
  if (wizardCurrentStep === 1 && step > 1) {
    wizardSaveWifiSettings();
  }
  // Clean up wifistat polling if leaving step 1
  if (wizardCurrentStep === 1) {
    if (wizardWifistatInterval) { clearInterval(wizardWifistatInterval); wizardWifistatInterval = null; }
    if (wizardConnectInterval) { clearInterval(wizardConnectInterval); wizardConnectInterval = null; }
  }
  // Clean up remotes interval if leaving step 3
  if (wizardCurrentStep === 3 && wizardRemotesInterval) {
    clearInterval(wizardRemotesInterval);
    wizardRemotesInterval = null;
  }
  // Clean up MQTT polling if leaving step 4
  if (wizardCurrentStep === 4 && wizardMqttInterval) {
    clearInterval(wizardMqttInterval);
    wizardMqttInterval = null;
  }
  wizardCurrentStep = step;
  // Persist wizard step across reboots
  websock_send('{"wizardsave":' + step + '}');
  $qa('.wizard-step').forEach(function (el) { el.classList.remove('active'); });
  var stepEl = $id('wizard-step-' + step);
  if (stepEl) stepEl.classList.add('active');
  wizardUpdateIndicators();
  // Load data for step
  if (step === 1) {
    getSettings('wifisetup');
    getSettings('wifistat');
    wizardWifistatInterval = setInterval(function () {
      getSettings('wifistat');
    }, 2000);
  } else if (step === 2) {
    getSettings('ithosetup');
    getSettings('syssetup');
  } else if (step === 3) {
    getSettings('rfsetup');
    getSettings('ithoremotes');
    wizardRemotesInterval = setInterval(function () {
      getSettings('rfsetup');
      getSettings('ithoremotes');
    }, 5000);
    if (wizardActive) {
      var nr = $id('wiz-numrfrem-row'); if (nr) nr.style.display = 'none';
      var rt = $id('wiz-rfremtype-row'); if (rt) rt.style.display = 'none';
      var rb = $id('wiz-rf-buttons'); if (rb) rb.style.display = 'none';
      var rl = $id('wiz-rf-later'); if (rl) rl.style.display = '';
    }
  } else if (step === 4) {
    getSettings('mqttsetup');
  }
}

function wizardNext() {
  var visible = wizardGetVisibleSteps();
  var idx = visible.indexOf(wizardCurrentStep);
  if (idx < visible.length - 1) {
    wizardGoTo(visible[idx + 1]);
  }
}

function wizardBack() {
  var visible = wizardGetVisibleSteps();
  var idx = visible.indexOf(wizardCurrentStep);
  if (idx > 0) {
    wizardGoTo(visible[idx - 1]);
  }
}

function wizardAbort() {
  if (!confirm('Are you sure you want to abort the wizard? No settings will be saved.')) return;
  // Clean up any active intervals
  if (wizardWifistatInterval) { clearInterval(wizardWifistatInterval); wizardWifistatInterval = null; }
  if (wizardConnectInterval) { clearInterval(wizardConnectInterval); wizardConnectInterval = null; }
  if (wizardRemotesInterval) { clearInterval(wizardRemotesInterval); wizardRemotesInterval = null; }
  if (wizardMqttInterval) { clearInterval(wizardMqttInterval); wizardMqttInterval = null; }
  // Clear wizard state on device
  websock_send('{"wizardclear":true}');
  wizardActive = false;
  // Restore menu and navigate to index
  $id('menu').style.display = '';
  $id('menuLink').style.display = '';
  update_page('index');
}

function wizardDetectDeviceCategory(devtype) {
  if (!devtype || devtype === 'Unkown device type') return 'other';
  if (devtype === 'Heatpump') return 'wpu';
  if (devtype === 'HRU 250-300') return 'hru250_300';
  if (devtype === 'HRU 350') return 'hru350';
  if (devtype === 'HRU ECO-fan') return 'hru_eco';
  if (devtype === 'CVE-SilentExtPlus') return 'hru200';
  if (devtype.indexOf('CVE') !== -1) return 'cve';
  return 'other';
}

function applyDeviceDefaults() {
  var cat = wizardDeviceCategory;
  console.log("wizardDeviceCategory:" + wizardDeviceCategory)

  // Helper to check a radio by name+value
  function checkRadio(name, value) {
    var el = $q('input[name="' + name + '"][value="' + value + '"]');
    if (el) el.checked = true;
  }

  // PWM2I2C: on for all CVE devices (CVE, CVE ECO2, CVE-Silent, CVE-SilentExtPlus), except Optima Inside
  var isCveDevice = (cat === 'cve' || cat === 'hru200');
  var enablePwm2i2c = isCveDevice && !(wizardDevType === 'CVE-Silent' && wizardIsOptimaInside);
  checkRadio('option-itho_pwm2i2c', enablePwm2i2c ? '1' : '0');
  // Lock PWM2I2C when Optima Inside (must stay off)
  var pwm1 = $id('option-pwm2i2c-1');
  var pwm0 = $id('option-pwm2i2c-0');
  var optimaLock = (wizardDevType === 'CVE-Silent' && wizardIsOptimaInside);
  if (pwm1) pwm1.disabled = optimaLock;
  if (pwm0) pwm0.disabled = optimaLock;

  // 31DA and 31D9: on for most, off for WPU
  var enable31 = (cat === 'cve' || cat === 'hru200' || cat === 'hru_eco' || cat === 'hru350' || cat === 'hru250_300');
  checkRadio('option-itho_31da', enable31 ? '1' : '0');
  checkRadio('option-itho_31d9', enable31 ? '1' : '0');

  // WPU counters (4210): on for WPU, off for others (always visible in wizard)
  checkRadio('option-itho_4210', cat === 'wpu' ? '1' : '0');

  // Virtual remote and related settings
  var useVirtual = (cat !== 'hru250_300' && cat !== 'wpu');
  var numvremEl = $id('itho_numvrem');
  if (useVirtual) {
    if (numvremEl) numvremEl.value = 1;
    checkRadio('option-itho_sendjoin', '1');
    checkRadio('option-itho_forcemedium', '1');
  } else {
    if (numvremEl) numvremEl.value = (cat === 'wpu' ? 0 : 1);
    checkRadio('option-itho_sendjoin', '0');
    checkRadio('option-itho_forcemedium', '0');
  }

  // Hide virtual remote section for HRU250/300, show RF note instead
  var vremSection = $id('wiz-vrem-section');
  if (vremSection) vremSection.style.display = (cat === 'hru250_300') ? 'none' : '';
  var rfNote = $id('wiz-rf-note');
  if (rfNote) rfNote.style.display = (cat === 'hru250_300') ? '' : 'none';

  // Virtual remote type: RFT AUTO for CVE-Silent, HRU 250-300, HRU 350, and other HRU types
  // Exception: Older CVE devices (DG=0x0, ID=0x4 or 0x14) use RFT CVE
  var vremTypeEl = $id('wiz-vremtype');
  var vremTypeRow = $id('wiz-vremtype-row');
  if (vremTypeRow) vremTypeRow.style.display = useVirtual ? '' : 'none';
  if (vremTypeEl) {
    var devGroup = parseInt(localStorage.getItem("itho_mfr") || "0", 10);
    var devId = parseInt(localStorage.getItem("itho_deviceid") || "0", 10);
    var isOlderCVE = (devGroup === 0x0 && (devId === 0x4 || devId === 0x14));
    var useAuto = !isOlderCVE && (cat === 'cve' || cat === 'hru350' || cat === 'hru_eco' || cat === 'hru200' || cat === 'hru250_300');
    vremTypeEl.value = useAuto ? '0x22F3' : '0x22F1';
  }

  // Update frequency
  var deviceTypeQ = (cat === 'cve' || cat === 'hru200' || cat === 'hru_eco' || cat === 'hru350' || cat === 'hru250_300');
  var updatefreqEl = $id('itho_updatefreq');
  if (updatefreqEl) updatefreqEl.value = (deviceTypeQ ? 10 : 60);

  // RF remote type and function defaults:
  // PWM2I2C devices (CVE, HRU200): RFT AUTO, function=receive
  // Non-PWM2I2C HRU devices (HRU350, HRU250-300, HRU ECO): RFT CO2, function=send
  wizardRfCo2Device = (cat === 'hru350' || cat === 'hru250_300' || cat === 'hru_eco');
  wizardHru250_300 = (cat === 'hru250_300');
  var rfRemTypeEl = $id('wiz-rfremtype');
  if (rfRemTypeEl) {
    rfRemTypeEl.value = wizardRfCo2Device ? '0x1298' : '0x22F3';
  }
  // Auto-configure first RF remote as Send+RFTCO2 for non-PWM2I2C HRU devices
  if (wizardRfCo2Device && wizardHasRF) {
    setTimeout(function() { wizardAutoConfigRFRemote(); }, 500);
  }

  // Update device info display
  var hwEl = $id('wiz-hw-revision');
  if (hwEl) hwEl.textContent = (typeof hw_revision !== 'undefined' ? (hw_revision.startsWith('NON-CVE') ? 'Non-CVE' : 'CVE') : 'Unknown');

  // Lock default settings initially
  wizardLockDefaults();
}

function wizardLockDefaults() {
  // List of field IDs to lock
  var fieldsToLock = [
    'option-pwm2i2c-1', 'option-pwm2i2c-0',
    'option-31da-1', 'option-31da-0',
    'option-31d9-1', 'option-31d9-0',
    'option-4210-1', 'option-4210-0',
    'itho_numvrem',
    'option-vremotejoin-1', 'option-vremotejoin-0',
    'option-vremotemedium-1', 'option-vremotemedium-0',
    'wiz-vremtype',
    'itho_updatefreq'
  ];

  fieldsToLock.forEach(function(id) {
    var el = $id(id);
    if (el) {
      el.disabled = true;
      // Don't lock PWM2I2C if it's already locked for Optima Inside
      if ((id === 'option-pwm2i2c-1' || id === 'option-pwm2i2c-0') &&
          wizardDevType === 'CVE-Silent' && wizardIsOptimaInside) {
        // Keep the Optima lock
      }
    }
  });
}

function wizardOverrideDefaults() {
  // Unlock all default fields except Optima-locked PWM2I2C
  var fieldsToUnlock = [
    'option-pwm2i2c-1', 'option-pwm2i2c-0',
    'option-31da-1', 'option-31da-0',
    'option-31d9-1', 'option-31d9-0',
    'option-4210-1', 'option-4210-0',
    'itho_numvrem',
    'option-vremotejoin-1', 'option-vremotejoin-0',
    'option-vremotemedium-1', 'option-vremotemedium-0',
    'wiz-vremtype',
    'itho_updatefreq'
  ];

  fieldsToUnlock.forEach(function(id) {
    var el = $id(id);
    if (el) {
      // Don't unlock PWM2I2C if it's locked for Optima Inside
      var isOptimaLock = ((id === 'option-pwm2i2c-1' || id === 'option-pwm2i2c-0') &&
                          wizardDevType === 'CVE-Silent' && wizardIsOptimaInside);
      if (!isOptimaLock) {
        el.disabled = false;
      }
    }
  });

  // Update button appearance
  var btn = $id('wiz-override-defaults');
  var icon = $id('wiz-override-icon');
  var msg = $id('wiz-override-msg');
  if (btn) {
    btn.disabled = true;
    btn.classList.add('pure-button-disabled');
    btn.innerHTML = '<span id="wiz-override-icon">🔓</span> Defaults Unlocked';
  }
  if (msg) msg.style.display = '';
}

function wizardOnDevInfo(devtype, devgroup, devid) {
  var effectiveType = wizardSimDevType || devtype;
  wizardDeviceCategory = wizardDetectDeviceCategory(effectiveType);
  wizardDevType = effectiveType || '';

  // Store device group and ID if provided
  if (devgroup !== undefined) localStorage.setItem("itho_mfr", devgroup);
  if (devid !== undefined) localStorage.setItem("itho_deviceid", devid);

  var el = $id('wiz-dev-type');
  if (el) el.textContent = (effectiveType || 'Unknown') + (wizardSimDevType ? ' (simulated)' : '');
  // Show CO2 choice for CVE-Silent (device ID 0x1B)
  var co2box = $id('wiz-co2-box');
  if (co2box) co2box.style.display = (effectiveType === 'CVE-Silent') ? '' : 'none';
  // Force RF available when simulating
  if (wizardSimDevType) {
    wizardHasRF = true;
    wizardUpdateIndicators();
  }
  applyDeviceDefaults();
  // Request status to check for CO2 sensor (may not be available on first boot)
  if (effectiveType === 'CVE-Silent') getSettings('ithostatus');
}

function wizardSetCO2Choice(isOptima) {
  wizardIsOptimaInside = isOptima;
  applyDeviceDefaults();
  var msg = $id('wiz-co2-chosen');
  if (msg) {
    msg.style.display = '';
    msg.textContent = isOptima
      ? 'Configured for Optima Inside: PWM2I2C disabled, virtual remote enabled.'
      : 'Configured for standard CVE (no built-in CO2): PWM2I2C enabled.';
  }
}

function wizardAutoConfigRFRemote() {
  var idEl = $id('id_remote-0');
  var rfIdEl = $id('module_rf_id_str');
  if (!idEl || !rfIdEl) return;
  // Only auto-fill if the slot is empty
  if (idEl.value === 'empty slot' || idEl.value === '0,0,0' || idEl.value === '') {
    idEl.value = rfIdEl.value;
  }
  // Set remote function to Send (5)
  var funcEl = $id('func_remote-0');
  if (funcEl) {
    funcEl.value = '5';
    funcEl.disabled = false;
  }
  // Set name
  var nameEl = $id('name_remote-0');
  if (nameEl && (!nameEl.value || nameEl.value === '')) {
    nameEl.value = 'RF Send';
    nameEl.readOnly = false;
  }
}

function wizardFinish() {
  // Helper to get checked radio value
  function checkedVal(name) {
    var el = $q('input[name="' + name + '"]:checked');
    return el ? el.value : undefined;
  }

  // 1. Save WiFi settings (also saved when leaving step 1, but save again with latest values)
  wizardSaveWifiSettings();

  // 2. Save system settings (device defaults from step 2)
  var sendjoinVal = checkedVal('option-itho_sendjoin') || '0';
  var forcemediumVal = checkedVal('option-itho_forcemedium') || '0';
  var sysMsg = {
    systemsettings: {
      itho_pwm2i2c: checkedVal('option-itho_pwm2i2c'),
      itho_31da: checkedVal('option-itho_31da'),
      itho_31d9: checkedVal('option-itho_31d9'),
      itho_numvrem: $val('itho_numvrem'),
      itho_sendjoin: sendjoinVal,
      itho_forcemedium: forcemediumVal,
      itho_updatefreq: $val('itho_updatefreq'),
      itho_4210: checkedVal('option-itho_4210') || '0'
    }
  };
  // For non-PWM2I2C HRU devices: enable RF CO2 join on next boot
  if (wizardRfCo2Device) {
    sysMsg.systemsettings.itho_rf_co2_join = 1;
  }
  websock_send(JSON.stringify(sysMsg));

  // 2b. Save virtual remote type (if virtual remote is configured)
  var vremTypeEl = $id('wiz-vremtype');
  if (vremTypeEl && parseInt($val('itho_numvrem')) > 0) {
    var vremType = parseInt(vremTypeEl.value);
    websock_send(JSON.stringify({itho_update_vremote: 0, remtype: vremType, value: 'remote0'}));
  }

  // 2c. Save RF remote type for all configured RF remotes
  var rfRemTypeEl = $id('wiz-rfremtype');
  if (rfRemTypeEl && remotesCount > 0) {
    var rfRemType = parseInt(rfRemTypeEl.value);
    for (var ri = 0; ri < remotesCount; ri++) {
      var idEl = $id('id_remote-' + ri);
      if (idEl && idEl.value && idEl.value !== 'empty slot') {
        var funcEl = $id('func_remote-' + ri);
        var remfuncVal = funcEl ? parseInt(funcEl.value) : 1;
        websock_send(JSON.stringify({
          itho_update_remote: ri,
          remtype: rfRemType,
          remfunc: remfuncVal,
          value: $id('name_remote-' + ri) ? $id('name_remote-' + ri).value : '',
          id: idEl.value.split(',').map(function(s) { return parseInt(s, 16); })
        }));
      }
    }
  }

  // 3. Save MQTT settings
  var mqtt_passwd = $val('mqtt_password');
  var mqtt_secure_credentials = {};
  var enc_mqtt = secureCredentials.getEncryptedField(mqtt_passwd);
  if (enc_mqtt) mqtt_secure_credentials.mqtt_password = enc_mqtt;

  var mqttMsg = {
    systemsettings: {
      mqtt_active: checkedVal('option-mqtt_active'),
      mqtt_serverName: $val('mqtt_serverName'),
      mqtt_username: $val('mqtt_username'),
      mqtt_password: enc_mqtt ? '********' : mqtt_passwd,
      mqtt_port: $val('mqtt_port'),
      mqtt_base_topic: $val('mqtt_base_topic'),
      mqtt_ha_active: checkedVal('option-mqtt_ha_active')
    }
  };
  if (Object.keys(mqtt_secure_credentials).length > 0) {
    mqttMsg.systemsettings.secure_credentials = mqtt_secure_credentials;
  }
  websock_send(JSON.stringify(mqttMsg));

  // 4. Clear wizard state
  websock_send('{"wizardclear":true}');

  // Determine what action is needed
  var ssidChanged = $val('ssid') !== '';
  var joinEnabled = sendjoinVal !== '0';
  var rfCo2JoinEnabled = wizardRfCo2Device;
  var needsPowerCycle = (joinEnabled && forcemediumVal !== '0') || (rfCo2JoinEnabled && !wizardHru250_300);
  var needsReboot = ssidChanged && !needsPowerCycle;

  // Build target URL — prefer IP address, fall back to hostname.local
  var ipSpan = $q('[name="wifiip"]');
  var ipAddr = ipSpan ? ipSpan.textContent : '';
  var hasIp = (ipAddr && ipAddr !== 'unknown' && ipAddr !== '0.0.0.0');
  var dhcpEl = $q('input[name="option-dhcp"]:checked');
  var dhcp = dhcpEl ? dhcpEl.value : 'on';
  var hostnameUrl;
  if (dhcp === 'off') {
    hostnameUrl = 'http://' + ($val('ip') || '192.168.4.1');
  } else {
    hostnameUrl = 'http://' + ($val('hostname') || 'nrg-itho') + '.local';
  }
  var targetUrl = hasIp ? ('http://' + ipAddr) : hostnameUrl;

  var urlMsg = 'Device will be available at: <a href="' + targetUrl + '">' + targetUrl + '</a>';
  if (hasIp && hostnameUrl !== targetUrl) {
    urlMsg += '<br>or: <a href="' + hostnameUrl + '">' + hostnameUrl + '</a>';
  }

  // Show overlay
  $id('wizard-reboot-overlay').style.display = '';
  $id('wizard-reboot-url').innerHTML = urlMsg;

  if (needsPowerCycle) {
    // Don't reboot — instruct user to power cycle the Itho unit instead
    var pcMsg = $id('wizard-powercycle-msg');
    if (pcMsg) {
      pcMsg.style.display = '';
      if (rfCo2JoinEnabled && !wizardHru250_300) {
        pcMsg.innerHTML = '<strong>Important:</strong> Power cycle your Itho unit. The add-on will automatically send an RF CO2 join command within 2 minutes after boot. After successful pairing, the add-on will control your Itho via RF CO2 demand commands.';
      } else {
        pcMsg.innerHTML = '<strong>Important:</strong> Power cycle your Itho unit to complete the virtual remote pairing. The add-on will restart together with the unit.';
      }
    }
    if (ssidChanged) {
      var wifiMsg = $id('wizard-wifi-reconnect');
      if (wifiMsg) wifiMsg.style.display = '';
    }
    $id('wizard-finish-status').innerHTML = 'Settings saved. <a href="' + targetUrl + '">Go to device</a>';
  } else if (wizardHru250_300 && rfCo2JoinEnabled) {
    // HRU250-300: user must manually put Itho in learn mode, then send join
    var pcMsg = $id('wizard-powercycle-msg');
    if (pcMsg) {
      pcMsg.style.display = '';
      pcMsg.innerHTML = '<strong>HRU 250/300:</strong> Put your Itho in learn mode (refer to your Itho manual).<br>' +
        'Then click the button below to send the RF CO2 join command.<br><br>' +
        '<button id="wizard-send-rf-join" type="button" class="pure-button pure-button-primary">Send RF CO2 Join</button>' +
        '<span id="wizard-join-status" style="margin-left:10px"></span>';
    }
    $id('wizard-finish-status').innerHTML = 'Settings saved. Waiting for RF CO2 join...';
    // Attach handler for the join button
    setTimeout(function() {
      var joinBtn = $id('wizard-send-rf-join');
      if (joinBtn) {
        joinBtn.addEventListener('click', function() {
          websock_send('{"rfremotecmd":"join","rfremoteindex":0}');
          var st = $id('wizard-join-status');
          if (st) st.textContent = 'Join command sent, waiting for response...';
        });
      }
    }, 100);
  } else if (needsReboot) {
    $id('wizard-finish-status').textContent = 'Saving configuration and rebooting...';
    $id('wizard-reboot-msg').style.display = '';
    var wifiMsg = $id('wizard-wifi-reconnect');
    if (wifiMsg) wifiMsg.style.display = '';
    setTimeout(function () {
      websock_send('{"reboot":true}');
    }, 1000);

    var countdown = 30;
    var cdInterval = setInterval(function () {
      countdown--;
      $id('wizard-reboot-countdown').textContent = countdown;
      if (countdown <= 0) {
        clearInterval(cdInterval);
        window.location.href = targetUrl;
      }
    }, 1000);
  } else {
    $id('wizard-finish-status').innerHTML = 'Settings saved successfully. <a href="' + targetUrl + '">Go to device</a>';
  }
}

//
// HTML string literals
//
