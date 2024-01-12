#ifndef FS_FILE_PRINT_H
#define FS_FILE_PRINT_H

#include <Arduino.h>
#include <FS.h>
#define FILE_WRITE "w"
#define FILE_APPEND "a"
#define FILE_READ "r"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif
#ifndef CURRENT_FILE_POSTFIX
#define CURRENT_FILE_POSTFIX ".current"
#endif
#ifndef LOGFILE_EXTENSION
#define LOGFILE_EXTENSION ".log"
#endif
#ifndef DEFAULT_LOGFILE_NAME
#define DEFAULT_LOGFILE_NAME "/logfile"
#endif

class FSFilePrint : public Print {
	public:
		/**
		 * @param logfileName name of the logfiles without number and extension
		 * @param logfilesCount amount of files to be used for logging
		 * @param targetedFileSize the targeted file size of a single file in bytes. The files will
		 * become slighly bigger than this value. Defaults to the SPIFFS minimum allocation unit size
		 * @param debugPort a Print to send error messages to, usually &Serial
		 */
		FSFilePrint(FS& filesystem, String logfileName, uint8_t logfilesCount = 1,
				unsigned int targetedFileSize = UINT8_MAX, Print *const debugPort = &Serial);
		virtual ~FSFilePrint();
		void open();
		void close();
		virtual inline int availableForWrite();

	protected:
		virtual size_t write(uint8_t n);

	private:
		const String logfileName;
		const uint8_t filesCount;
		const unsigned int targetedFileSize;
		Print *const altPort;

		char buf[BUFFER_SIZE];
		String currentFilename;
		int nextWritePos = 0;
		File file;
		void checkSizeAndAdjustCurrentFilename();
		inline String assembleCurrentFilename(uint8_t index);
		inline String assembleFilename(uint8_t index);

		FS* backend;
};

#endif  // #ifndef FS_FILE_PRINT_H
