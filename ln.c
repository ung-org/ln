/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2019, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_SEARCH
#define O_SEARCH 0
#endif

static int ln(const char *path, int dirfd, int sym, int force, int flag)
{
	struct stat st;
	if (fstatat(dirfd, path, &st, 0) == 0) {
		if (!force) {
			fprintf(stderr, "ln: %s: %s\n", strerror(EEXIST));
			return 1;
		}
		
		if (unlinkat(dirfd, path, 0) != 0) {
			fprintf(stderr, "ln: %s: %s\n", strerror(errno));
			return 1;
		}
	}

	if (sym) {
		if (symlinkat(path, dirfd, path) != 0) {
			fprintf(stderr, "ln: %s: %s\n", strerror(errno));
			return 1;
		}
	} else {
		if (linkat(AT_FDCWD, path, dirfd, path, flag) != 0) {
			fprintf(stderr, "ln: %s: %s\n", strerror(errno));
			return 1;
		}
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	int flag = 0;
	int symbolic = 0;
	int force = 0;

	int c;
	while ((c = getopt(argc, argv, "fsLP")) != -1) {
		switch (c) {
		case 'L':
			flag = AT_SYMLINK_FOLLOW;
			break;

		case 'P':
			flag = 0;
			break;

		case 's':
			symbolic = 1;
			break;

		case 'f':
			force = 1;
			break;

		default:
			return 1;
		}
	}

	if (argc - optind < 2) {
		fprintf(stderr, "ln: missing operands\n");
		return 1;
	}

	char *target = argv[argc - 1];

	int dirfd = open(target, O_SEARCH | O_DIRECTORY);
	if (dirfd == -1) {
		if (errno == ENOTDIR) {
			/* not a directory */
		} else if (errno == ENOENT) {
			/* doesn't exist */
		} else {
			return 1;
		}
	}

	if (argc - optind == 2) {
		struct stat st;
		if (stat(target, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				return ln(argv[optind], dirfd, symbolic, force, flag);
			}

			if (!force) {
				fprintf(stderr, "ln: %s: %s\n", target, strerror(EEXIST));
				return 1;
			}

			if (unlink(target) != 0) {
				fprintf(stderr, "ln: %s: %s\n", target, strerror(errno));
				return 1;
			}
		}

		int r = symbolic ? symlink(argv[optind], target) :
				link(argv[optind], target);
		if (r != 0) {
			fprintf(stderr, "ln: %s -> %s: %s\n", argv[optind], target, strerror(errno));
		}

		return r;
	}

	int r = 0;
	do {
		r |= ln(argv[optind++], dirfd, symbolic, force, flag);
	} while (optind < argc - 1);
	return r;
}
