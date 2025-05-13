/*
    _   ___ ___  _   _ ___ _  _  ___  _    ___   ___
   /_\ | _ \   \| | | |_ _| \| |/ _ \| |  / _ \ / __|
  / _ \|   / |) | |_| || || .` | (_) | |_| (_) | (_ |
 /_/ \_\_|_\___/ \___/|___|_|\_|\___/|____\___/ \___|

 Log library for Arduino

 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
*/

#ifndef _ARDUINO_LOG_H_
#define _ARDUINO_LOG_H_
#include <inttypes.h>
#include <stdarg.h>

// Mutex is supported only on ESP32 boards.
#ifdef ESP32
#include <mutex>
#endif

// Non standard: Arduino.h also chosen if ARDUINO is not defined.
// To facilitate use in non-Arduino test environments
#if ARDUINO < 100
#include "WProgram.h"
#else
#include "Arduino.h"
#endif

// PGM stubs to facilitate use in non-Arduino test environments
#ifndef PGM_P
#define PGM_P  const char *
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define PSTR(str) (str)
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
#endif
typedef void (*printfunction)(Print*, int);

// This allows Arduino-Log to output the stream to multiple outputs.
// If you need more than 5 outputs at a time, re-define this constant.
#define LOG_MAX_HANDLERS 5


// *************************************************************************
//  Uncomment line below to fully disable logging, and reduce project size
// ************************************************************************
//#define DISABLE_LOGGING

#define LOG_LEVEL_SILENT  0
#define LOG_LEVEL_FATAL   1
#define LOG_LEVEL_ERROR   2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_INFO    4
#define LOG_LEVEL_NOTICE  4
#define LOG_LEVEL_TRACE   5
#define LOG_LEVEL_VERBOSE 6

#define LOG_COLOR_BOLD_RED F("\033[1;31m")
#define LOG_COLOR_BOLD_WHITE F("\033[1;37m")
#define LOG_COLOR_RED "\033[;31m"
#define LOG_COLOR_YELLOW "\033[;33m"
#define LOG_COLOR_GREEN "\033[;32m"
#define LOG_COLOR_WHITE "\033[;37m"
#define LOG_COLOR_BLACK "\033[;38;5;240m"
#define LOG_COLOR_END "\033[0m"
#define CR "\n"
#define LF "\r"
#define NL "\n\r"

/**
 * ArduinoLog is a minimalistic framework to help the programmer output log statements to an output of choice, 
 * fashioned after extensive logging libraries such as log4cpp ,log4j and log4net. In case of problems with an
 * application, it is helpful to enable logging so that the problem can be located. ArduinoLog is designed so 
 * that log statements can remain in the code with minimal performance cost. In order to facilitate this the 
 * loglevel can be adjusted, and (if your code is completely tested) all logging code can be compiled out.
 * 
 * ---- Wildcards
 * 
 * %s	display as string (char*)
 * %S    display as string from flash memory (__FlashStringHelper* or char[] PROGMEM)
 * %c	display as single character
 * %C    display as single character or as hexadecimal value (prefixed by `0x`) if not a printable character
 * %d	display as integer value
 * %l	display as long value
 * %u	display as unsigned long value
 * %x	display as hexadecimal value
 * %X	display as hexadecimal value prefixed by `0x` and leading zeros
 * %b	display as binary number
 * %B	display as binary number, prefixed by `0b`
 * %t	display as boolean value "t" or "f"
 * %T	display as boolean value "true" or "false"
 * %D,%F display as double value
 * %p    display a  printable object 
 * 
 * ---- Loglevels
 * 
 * 0 - LOG_LEVEL_SILENT     no output
 * 1 - LOG_LEVEL_FATAL      fatal errors
 * 2 - LOG_LEVEL_ERROR      all errors
 * 3 - LOG_LEVEL_WARNING    errors and warnings
 * 4 - LOG_LEVEL_INFO       errors, warnings and notices
 * 4 - LOG_LEVEL_NOTICE     Same as INFO, kept for backward compatibility
 * 5 - LOG_LEVEL_TRACE      errors, warnings, notices, traces
 * 6 - LOG_LEVEL_VERBOSE    all
 */

class Logging {
public:
    explicit Logging();

    /**
     * Initializing, must be called as first. Note that if you use
     * this variant of Init, you need to initialize the baud rate
     * yourself, if printer happens to be a serial port.
     * 
     * \param level - logging levels <= this will be logged.
     * \param printer - place that logging output will be sent to.
     * \param showLevel - if true, the level will be shown in the output.
     * \param showColors - if true, the color will be shown in the output.
     * \return void
     *
     */
    void begin(int level, Print *output, bool showLevel = true, bool showColors = false);

    /**
     * Set the log level.
     * 
     * \param level - The new log level.
     * \return void
     */
    void setLevel(int level);

    /**
     * Get the log level.
     *
     * \return The current log level.
     */
    int getLevel() const;

    /**
     * Set whether to show the log level.
     * 
     * \param showLevel - true if the log level should be shown for each log
     *                    false otherwise.
     * \return void
     */
    void setShowLevel(bool showLevel);

    /**
     * Get whether the log level is shown during logging
     * 
     * \return true if the log level is be shown for each log
     *         false otherwise.
     */
    bool getShowLevel() const;

    /**
     * Set whether to show the log colors.
     *
     * \param showColors - true if the log colors should be shown for each log
     *                    false otherwise.
     * \return void
     */
    void setShowColors(bool showColors);

    /**
     * Get whether the log colors are shown during logging
     *
     * \return true if the log colors are shown for each log
     *         false otherwise.
     */
    bool getShowColors() const;

    /**
     * Sets a function to be called before each log command.
     * 
     * \param f - The function to be called
     * \return void
     */
    void setPrefix(printfunction f);

    /**
     * clears prefix.
     *
     * \return void
     */
    void clearPrefix();

    /**
     * Sets a function to be called after each log command.
     * 
     * \param f - The function to be called
     * \return void
     */
    void setSuffix(printfunction f);

    /**
     * clears suffix.
     *
     * \return void
     */
    void clearSuffix();

    /**
     * Adds a new handler which outputs the stream to another source.
     * For example, you can output to Serial and into a file at the same time.
     */
    void addHandler(Print* handler);

    /**
     * Removes stream handler and stops outputting the data.
     */
    void removeHandler(Print* handler);

    /**
     * Output a fatal error message. Output message contains
     * F: followed by original message
     * Fatal error messages are printed out at
     * loglevels >= LOG_LEVEL_FATAL
     * 
     * \param msg format string to output
     * \param ... any number of variables
     * \return void
     */
    template <class T, typename... Args> void fatal(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_FATAL, false, msg, args...);
#endif
    }

    template <class T, typename... Args> void fatalln(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_FATAL, true, msg, args...);
#endif
    }

    /**
     * Output an error message. Output message contains
     * E: followed by original message
     * Error messages are printed out at
     * loglevels >= LOG_LEVEL_ERROR
     * 
     * \param msg format string to output
     * \param ... any number of variables
     * \return void
     */
    template <class T, typename... Args> void error(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_ERROR, false, msg, args...);
#endif
    }
  
    template <class T, typename... Args> void errorln(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_ERROR, true, msg, args...);
#endif
    }

    /**
     * Output a warning message. Output message contains
     * W: followed by original message
     * Warning messages are printed out at
     * loglevels >= LOG_LEVEL_WARNING
     * 
     * \param msg format string to output
     * \param ... any number of variables
     * \return void
     */
    template <class T, typename... Args> void warning(T msg, Args...args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_WARNING, false, msg, args...);
#endif
    }
  
    template <class T, typename... Args> void warningln(T msg, Args...args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_WARNING, true, msg, args...);
#endif
    }

    /**
     * Output a notice message. Output message contains
     * I: followed by original message
     * Notice messages are printed out at
     * loglevels >= LOG_LEVEL_NOTICE
     * 
     * \param msg format string to output
     * \param ... any number of variables
     * \return void
     */
    template <class T, typename... Args> void notice(T msg, Args...args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_NOTICE, false, msg, args...);
#endif
    }
  
    template <class T, typename... Args> void noticeln(T msg, Args...args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_NOTICE, true, msg, args...);
#endif
    }

    template <class T, typename... Args> void info(T msg, Args...args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_INFO, false, msg, args...);
#endif
    }

    template <class T, typename... Args> void infoln(T msg, Args...args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_INFO, true, msg, args...);
#endif
    }

    /**
     * Output a trace message. Output message contains
     * N: followed by original message
     * Trace messages are printed out at
     * loglevels >= LOG_LEVEL_TRACE
     * 
     * \param msg format string to output
     * \param ... any number of variables
     * \return void
    */
    template <class T, typename... Args> void trace(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_TRACE, false, msg, args...);
#endif
    }

    template <class T, typename... Args> void traceln(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_TRACE, true, msg, args...);
#endif
    }

    /**
     * Output a verbose message. Output message contains
     * V: followed by original message
     * Debug messages are printed out at
     * loglevels >= LOG_LEVEL_VERBOSE
     * 
     * \param msg format string to output
     * \param ... any number of variables
     * \return void
     */
    template <class T, typename... Args> void verbose(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_VERBOSE, false, msg, args...);
#endif
    }

    template <class T, typename... Args> void verboseln(T msg, Args... args) {
#ifndef DISABLE_LOGGING
        printLevel(LOG_LEVEL_VERBOSE, true, msg, args...);
#endif
    }

private:
    void print(const char *format, va_list args);

    void print(const __FlashStringHelper *format, va_list args);

    void print(const Printable& obj, va_list args) {
#ifndef DISABLE_LOGGING
        for (int i = 0; i < _handlerCount; i++) {
            if (_logOutputs[i]) {
                _logOutputs[i]->print(obj);
            }
        }
#endif
    }

    void printFormat(const char format, va_list *args);

    template <class T> void printLevel(int level, bool cr, T msg, ...) {
#ifndef DISABLE_LOGGING
        if (level > _level) {
            return;
        }
        if (level < LOG_LEVEL_SILENT) {
            level = LOG_LEVEL_SILENT;
        }

#ifdef ESP32
        if (xSemaphoreTake(_semaphore, (TickType_t) 10 )) {
#endif
            if (_showColors) {
                switch (level) {
                case LOG_LEVEL_FATAL:
                    writeLog(LOG_COLOR_BOLD_RED);
                    break;
                case LOG_LEVEL_ERROR:
                    writeLog(LOG_COLOR_RED);
                    break;
                case LOG_LEVEL_WARNING:
                    writeLog(LOG_COLOR_YELLOW);
                    break;
                default:
                    break;
                }
            }

            if (_prefix != NULL && _showLevel) {
                for (int i = 0; i < _handlerCount; i++) {
                    if (_logOutputs[i]) {
                        _prefix(_logOutputs[i], level);
                    }
                }
            }

            if (_showLevel) {
                static const char levels[] = "FEWITV";
                writeLog(levels[level - 1]);
                writeLog(": ");
            }

            va_list args;
            va_start(args, msg);
            print(msg, args);

            if (_suffix != NULL && _showLevel) {
                for (int i = 0; i < _handlerCount; i++) {
                    if (_logOutputs[i]) {
                        _suffix(_logOutputs[i], level);
                    }
                }
            }

            if (cr)	{
                writeLog(CR);
            }

            if (_showColors) {
                writeLog(LOG_COLOR_END);
            }

            va_end(args);
#ifdef ESP32
            xSemaphoreGive(_semaphore);
        }
#endif
#endif
    }

#ifndef DISABLE_LOGGING
    int _level;
    bool _showLevel;
    bool _showColors;
    Print* _logOutputs[LOG_MAX_HANDLERS];
    int _handlerCount;

    template<typename... Args> void writeLog(Args... args);
#ifdef ESP32
    SemaphoreHandle_t _semaphore;
#endif

    printfunction _prefix = NULL;
    printfunction _suffix = NULL;
#endif
};

#ifndef __DO_NOT_INSTANTIATE__
extern Logging Log;
#endif
#endif