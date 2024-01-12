#include "FSFilePrint.h"

FSFilePrint::FSFilePrint(FS& filesystem, String logfileName,
		uint8_t logfilesCount, unsigned int targetedFileSize,
		Print *const debugPort)
	: logfileName(logfileName),
	filesCount(logfilesCount),
	targetedFileSize(targetedFileSize),
	altPort(debugPort),
	backend(&filesystem) {}

FSFilePrint::~FSFilePrint() { close(); }

void FSFilePrint::open() {
	close();
	checkSizeAndAdjustCurrentFilename();
	if (!backend->exists(currentFilename)) {
		file = backend->open(currentFilename, FILE_WRITE);
	} else {
		file = backend->open(currentFilename, FILE_APPEND);
	}
	if (!file) {
		this->altPort->print("Error opening file ");
		this->altPort->println(currentFilename);
	}
}

void FSFilePrint::close() {
	if (file) {
		if (nextWritePos) { //flush buffer when closing the logger
			buf[nextWritePos] = '\0';
			file.print(buf);
			nextWritePos = 0;
		}
		file.close();
	}
}

size_t FSFilePrint::write(uint8_t n) {
	char c = n;
	buf[nextWritePos++] = c;
	if (nextWritePos == BUFFER_SIZE - 2 || c == '\n') {
		buf[nextWritePos] = '\0';
		file.print(buf);
		nextWritePos = 0;
		// flush does not work reliably, only close and reopen does
		file.close();
		checkSizeAndAdjustCurrentFilename();
		file = backend->open(currentFilename, FILE_APPEND);
	}
	return 1;
}

inline int FSFilePrint::availableForWrite() {
	return BUFFER_SIZE - nextWritePos - 2;
}


void FSFilePrint::checkSizeAndAdjustCurrentFilename() {
	bool foundFile = false;
	for (int i = 0; i < filesCount; i++) {
		String filename = assembleCurrentFilename(i);
		if (backend->exists(filename)) {
			File currentFile = backend->open(filename, FILE_READ);
			size_t size = 0;
			if (currentFile) {
				size = currentFile.size();
				currentFile.close();
			}
			if (size < targetedFileSize) {
				currentFilename = filename;
			} else {
				backend->rename(filename, assembleFilename(i));
				if (i < filesCount - 1) {
					backend->remove(assembleFilename(i + 1));
					currentFilename = assembleCurrentFilename(i + 1);
				} else {
					backend->remove(assembleFilename(0));
					currentFilename = assembleCurrentFilename(0);
				}
			}
			foundFile = true;
			break;
		}
	}
	if (!foundFile) {
		currentFilename = assembleCurrentFilename(0);
	}
}

String FSFilePrint::assembleCurrentFilename(uint8_t index) {
	return logfileName + index + CURRENT_FILE_POSTFIX + LOGFILE_EXTENSION;
}

String FSFilePrint::assembleFilename(uint8_t index) {
	return logfileName + index + LOGFILE_EXTENSION;
}
