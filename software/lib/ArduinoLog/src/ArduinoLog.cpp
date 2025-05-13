/*
    _   ___ ___  _   _ ___ _  _  ___  _    ___   ___ 
   /_\ | _ \   \| | | |_ _| \| |/ _ \| |  / _ \ / __|
  / _ \|   / |) | |_| || || .` | (_) | |_| (_) | (_ |
 /_/ \_\_|_\___/ \___/|___|_|\_|\___/|____\___/ \___|
                                                     
 Log library for Arduino

 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
*/

#include "ArduinoLog.h"

Logging::Logging()
#ifndef DISABLE_LOGGING
    : _level(LOG_LEVEL_SILENT),
      _showLevel(true),
      _showColors(false),
      _handlerCount(0)
{
    for (int i = 0; i < LOG_MAX_HANDLERS; i++) {
        _logOutputs[i] = nullptr;
    }
}
#else
{}
#endif

void Logging::begin(int level, Print* logOutput, bool showLevel, bool showColors) {
#ifndef DISABLE_LOGGING
    setLevel(level);
    setShowLevel(showLevel);
    setShowColors(showColors);

    _handlerCount = 0;
    for (int i = 0; i < LOG_MAX_HANDLERS; i++) {
        _logOutputs[i] = nullptr;
    }

    addHandler(logOutput);
#ifdef ESP32
    _semaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(_semaphore);
#endif
#endif
}

void Logging::setLevel(int level) {
#ifndef DISABLE_LOGGING
    _level = constrain(level, LOG_LEVEL_SILENT, LOG_LEVEL_VERBOSE);
#endif
}

int Logging::getLevel() const {
#ifndef DISABLE_LOGGING
    return _level;
#else
    return 0;
#endif
}

void Logging::setShowLevel(bool showLevel) {
#ifndef DISABLE_LOGGING
    _showLevel = showLevel;
#endif
}

bool Logging::getShowLevel() const {
#ifndef DISABLE_LOGGING
    return _showLevel;
#else
    return false;
#endif
}

void Logging::setShowColors(bool showColors) {
#ifndef DISABLE_LOGGING
    _showColors = showColors;
#endif
}

bool Logging::getShowColors() const {
#ifndef DISABLE_LOGGING
    return _showColors;
#else
    return false;
#endif
}

void Logging::setPrefix(printfunction f) {
#ifndef DISABLE_LOGGING
    _prefix = f;
#endif
}

void Logging::clearPrefix() {
#ifndef DISABLE_LOGGING
    _prefix = nullptr;
#endif
}

void Logging::setSuffix(printfunction f) {
#ifndef DISABLE_LOGGING
    _suffix = f;
#endif
}

void Logging::clearSuffix() {
#ifndef DISABLE_LOGGING
    _suffix = nullptr;
#endif
}

void Logging::addHandler(Print* handler) {
#ifndef DISABLE_LOGGING
    for (int i = 0; i < _handlerCount; i++) {
        if (_logOutputs[i] == handler) {
            return; // Handler already exists, do not add again
        }
    }

    if (_handlerCount < LOG_MAX_HANDLERS) {
        _logOutputs[_handlerCount++] = handler;
    }
#endif
}

void Logging::removeHandler(Print* handler) {
#ifndef DISABLE_LOGGING
    for (int i = 0; i < _handlerCount; i++) {
        if (_logOutputs[i] == handler) {
            for (int j = i; j < _handlerCount - 1; j++) {
                _logOutputs[j] = _logOutputs[j + 1];
            }
            _logOutputs[_handlerCount - 1] = nullptr;
            _handlerCount--;
            break;
        }
    }
#endif
}

void Logging::print(const __FlashStringHelper *format, va_list args) {
#ifndef DISABLE_LOGGING	  	
    PGM_P p = reinterpret_cast<PGM_P>(format);
// This copy is only necessary on some architectures (x86) to change a passed
// array in to a va_list.
#ifdef __x86_64__
    va_list args_copy;
    va_copy(args_copy, args);
#endif
    char c = pgm_read_byte(p++);
    for(;c != 0; c = pgm_read_byte(p++)) {
        if (c == '%') {
            c = pgm_read_byte(p++);
#ifdef __x86_64__
            printFormat(c, &args_copy);
#else
            printFormat(c, &args);
#endif
        }
        else {
            writeLog(c);
        }
    }
#ifdef __x86_64__
    va_end(args_copy);
#endif
#endif
}

void Logging::print(const char *format, va_list args) {
#ifndef DISABLE_LOGGING	  	
// This copy is only necessary on some architectures (x86) to change a passed
// array in to a va_list.
#ifdef __x86_64__
    va_list args_copy;
    va_copy(args_copy, args);
#endif
    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;
#ifdef __x86_64__
            printFormat(*format, &args_copy);
#else
            printFormat(*format, &args);
#endif
        }
        else {
            writeLog(*format);
        }
    }
#ifdef __x86_64__
    va_end(args_copy);
#endif
#endif
}

void Logging::printFormat(const char format, va_list *args) {
#ifndef DISABLE_LOGGING
    if (format == '\0') return;
    if (format == '%') {
        writeLog(format);
    }
    else if (format == 's') {
        char *s = va_arg(*args, char *);
        writeLog(s);
    }
    else if (format == 'S')	{
        __FlashStringHelper *s = va_arg(*args, __FlashStringHelper *);
        writeLog(s);
    }
    else if (format == 'd' || format == 'i') {
        writeLog(va_arg(*args, int), DEC);
    }
    else if (format == 'D' || format == 'F') {
        writeLog(va_arg(*args, double));
    }
    else if (format == 'x') {
        writeLog(va_arg(*args, int), HEX);
    }
    else if (format == 'X') {
        writeLog("0x");
        uint16_t h = (uint16_t) va_arg( *args, int );
        if (h<0xFFF) writeLog('0');
        if (h<0xFF ) writeLog('0');
        if (h<0xF  ) writeLog('0');
        writeLog(h,HEX);
    }
    else if (format == 'p') {
        Printable *obj = (Printable *) va_arg(*args, int);
        for (int i = 0; i < _handlerCount; i++) {
           if (_logOutputs[i]) {
                _logOutputs[i]->print(*obj);
            }
        }
    }
    else if (format == 'b') {
        writeLog(va_arg(*args, int), BIN);
    }
    else if (format == 'B') {
        writeLog("0b");
        writeLog(va_arg(*args, int), BIN);
    }
    else if (format == 'l') {
        writeLog(va_arg(*args, long), DEC);
    }
    else if (format == 'u') {
        writeLog(va_arg(*args, unsigned long), DEC);
    }
    else if (format == 'c') {
        writeLog((char) va_arg(*args, int));
    }
    else if( format == 'C' ) {
        char c = (char) va_arg( *args, int );
        if (c>=0x20 && c<0x7F) {
            writeLog(c);
        } else {
            writeLog("0x");
            if (c<0xF) writeLog('0');
            writeLog(c, HEX);
        }
    }
    else if(format == 't') {
        if (va_arg(*args, int) == 1) {
            writeLog("T");
        }
        else {
            writeLog("F");
        }
    }
    else if (format == 'T') {
        if (va_arg(*args, int) == 1) {
            writeLog(F("true"));
        } else {
            writeLog(F("false"));
        }
    }
#endif
}

#ifndef DISABLE_LOGGING
template<typename... Args> void Logging::writeLog(Args... args) {
    for (int i = 0; i < _handlerCount; i++) {
        if (_logOutputs[i]) {
            _logOutputs[i]->print(args...);
        }
    }
}
#endif

#ifndef __DO_NOT_INSTANTIATE__
Logging Log = Logging();
#endif
