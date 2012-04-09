/*                                                                                                                                                    
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All Rights Reserved. 
 *                                                                                 
 * This file is part of quickpanel  
 *
 * Written by Jeonghoon Park <jh1979.park@samsung.com> Youngjoo Park <yjoo93.park@samsung.com>
 *                                                                                 
 * PROPRIETARY/CONFIDENTIAL                                                        
 *                                                                                 
 * This software is the confidential and proprietary information of                
 * SAMSUNG ELECTRONICS ("Confidential Information").                               
 * You shall not disclose such Confidential Information and shall use it only   
 * in accordance with the terms of the license agreement you entered into          
 * with SAMSUNG ELECTRONICS.                                                       
 * SAMSUNG make no representations or warranties about the suitability of          
 * the software, either express or implied, including but not limited to           
 * the implied warranties of merchantability, fitness for a particular purpose, 
 * or non-infringement. SAMSUNG shall not be liable for any damages suffered by 
 * licensee as a result of using, modifying or distributing this software or    
 * its derivatives.                                                                
 *                                                                                 
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
