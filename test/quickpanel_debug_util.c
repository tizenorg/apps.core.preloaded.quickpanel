/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifndef LOGFILE
#define LOGFILE		DATADIR"/quickpanel.log"
#endif

#define MAXSIZE	(1 << 17)

static char buf[512];

void debug_printf(const char *msg, ...)
{
	int fd;
	va_list arg_list;
	int len;
	struct tm ts;
	time_t ctime;
	int status;
	struct stat buffer;

	/* Set time */
	ctime = time(NULL);
	localtime_r(&ctime, &ts);

	snprintf(buf, 64, "[%04d/%02d/%02d %02d:%02d:%02d] ",
		 ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday,
		 ts.tm_hour, ts.tm_min, ts.tm_sec);
	len = strlen(buf);

	va_start(arg_list, msg);

	fd = open(LOGFILE, O_WRONLY | O_CREAT | O_APPEND,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		fprintf(stderr, msg, arg_list);
		return;
	}

	status = fstat(fd, &buffer);
	if (!status && (buffer.st_size > MAXSIZE)) {
		ftruncate(fd, 0);
	}

	len = vsnprintf(&buf[len], 511, msg, arg_list);
	/* fix for flawfinder warnings: check string length */
	if (len < 0) {
		return;
	}

	write(fd, buf, strlen(buf));

	close(fd);
	va_end(arg_list);
}
