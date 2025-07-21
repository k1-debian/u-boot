#ifndef LOG_MEMORY_H
#define LOG_MEMORY_H

#include <common.h>
#include <cloner/cloner.h>

static const char* level_strings[] = {
	"DEBUG",
	"INFO",
	"WARNING",
	"ERROR"
};

static const char* level_colors[] = {
	"\033[0;36m", //DEBUG:    Cyan
	"\033[0;32m", //INFO:     Green
	"\033[0;33m", //WARNING:  Yellow
	"\033[0;31m" //ERROR:    Red
};

void log_init(void) {
	*LOG_INDEX_ADDR = 0;
	memset((void*)LOG_BASE_ADDR, 0, LOG_BUFF_SIZE);
}

void log_write(LogLevel level, const char* format, ...) {

	if (debug_args->log_enabled == 0 ||
	    level < MIN_LOG_LEVEL) {
		return;
	}

	uint32_t idx = *LOG_INDEX_ADDR;
	if (idx >= LOG_BUFF_SIZE - 128) {
		*LOG_INDEX_ADDR = 0;
		idx = 0;
	}

	char* buffer = (char*)LOG_BASE_ADDR + idx;
	char* pbuffer = buffer;
	int remaining = LOG_BUFF_SIZE - idx;

	int header_len = snprintf(buffer, remaining, "[%s] ", level_strings[level]);
	if (header_len <= 0) {
		return;
	}

	idx += header_len;
	buffer += header_len;
	remaining -= header_len;

	va_list args;
	va_start(args, format);
	int content_len = vsnprintf(buffer, LOG_BUFF_SIZE - idx, format, args);
	va_end(args);

	if (content_len > 0) {
		idx += content_len;
		buffer += content_len;

//		if (idx < LOG_BUFF_SIZE - 1) {
//			*buffer = '\n';
//			idx++;
//		}
	}

	*LOG_INDEX_ADDR = idx;

	printf("%s", level_colors[level]);
	printf("%s", pbuffer);
	printf("\033[0m");
}

const char* log_buffer(void) {
	return (const char*)LOG_BASE_ADDR;
}

uint32_t log_length(void) {
	return *LOG_INDEX_ADDR;
}

#endif /* LOG_MEMORY_H */
