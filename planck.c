/*
 * Copyright (c) 2020 Brian Callahan <bcallah@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * planck -- text editor
 */

extern void *_syscall(void *n, void *a, void *b, void *c, void *d, void *e);

static char linecol[1024][128];

static void
_exit(int status)
{

	_syscall((void *) 1, (void *) status, (void *) 0, (void *) 0, (void *) 0, (void *) 0);
}

static long
read(int d, void *buf, unsigned long nbytes)
{

	return (long) _syscall((void *) 3, (void *) d, (void *) buf, (void *) nbytes, (void *) 0, (void *) 0);
}

static long
write(int d, const void *buf, unsigned long nbytes)
{

	return (long) _syscall((void *) 4, (void *) d, (void *) buf, (void *) nbytes, (void *) 0, (void *) 0);
}

static int
open(const char *path, int flags, int mode)
{

	return (int) _syscall((void *) 5, (void *) path, (void *) flags, (void *) mode, (void *) 0, (void *) 0);
}

static int
close(int d)
{

	return (int) _syscall((void *) 6, (void *) d, (void *) 0, (void *) 0, (void *) 0, (void *) 0);
}

static unsigned long
strlen(const char *s)
{
	char *t;

	t = (char *) s;
	while (*t != '\0')
		t++;

	return t - s;
}

static int
dgets(char *s, int size, int fd)
{
	int i;

	for (i = 0; i < size - 1; i++) {
		if (read(fd, &s[i], 1) < 1)
			return 0;

		if (s[i] == '\n')
			break;
	}
	s[i] = '\0';

	return strlen(s);
}

static void
dputs(const char *s, int fd)
{

	write(fd, s, strlen(s));
}

static void
dputi(int n, int fd)
{
	char num[4];
	int i = 0;

	do {
		num[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);

	for (i--; i >= 0; i--)
		write(fd, &num[i], 1);
}

static int
dgeti(char *s, int size, int fd)
{
	int i, n = 0;

	for (i = 0; i < size - 1; i++) {
		if (read(fd, &s[i], 1) < 1)
			return 0;

		if (s[i] == '\n')
			break;

		if (s[i] < '0' || s[i] > '9')
			n = 1;
	}
	s[i] = '\0';

	if (s[0] == '\0')
		return 0;

	if (n == 1)
		return 1025;

	n = 0;
	for (i = 0; i < strlen(s); i++)
		n = (n * 10) + (s[i] - '0');

	return n;
}

int
main(int argc, char *argv[])
{
	char file[1024], line[128];
	char buf[5], c;
	int co = 0, fd, i, j, li = 0;

	if (argc > 2) {
		dputs("usage: ", 2);
		dputs(argv[0], 2);
		dputs(" [file]\n", 2);

		_exit(1);
	}

	if (argc == 2) {
		for (i = 0; i < strlen(argv[1]); i++)
			file[i] = argv[1][i];
		file[i] = '\0';

		if ((fd = open(file, 0x0000, 0)) == -1) {
			dputs("planck: error: could not open ", 2);
			dputs(file, 2);
			dputs("\n", 2);

			_exit(1);
		}

		co = 0;
		li = 0;
		while (read(fd, &c, 1) > 0) {
			linecol[li][co] = c;

			if (++co > 126) {
				dputs("plank: error: line ", 2);
				dputi(li, 2);
				dputs(" is longer than 127 characters\n", 2);
			}

			if (c == '\n') {
				if (++li > 1023) {
					dputs("plank: error: ", 2);
					dputs(argv[1], 2);
					dputs(" is greater than 1024 lines\n", 2);

					_exit(1);
				}
				co = 0;
			}
		}

		if (close(fd) == -1) {
			dputs("planck: error: could not close ", 2);
			dputs(file, 2);
			dputs("\n", 2);

			_exit(1);
		}
	}

	dputi(li, 1);
	dputs("\n", 1);

get_command:
	dputs("line: ", 1);

	if ((li = dgeti(buf, sizeof(buf), 0)) > 1024) {
		dputs("?\n", 1);
		goto get_command;
	}

	dputs("command: ", 1);

	(void) dgets(buf, sizeof(buf), 0);

	switch (buf[0]) {
	case 'd':
		for (i = 0; i < 128; i++)
			linecol[li - 1][i] = '\0';
		for (i = li; i < 1024; i++) {
			for (j = 0; j < 128; j++)
				linecol[i - 1][j] = linecol[i][j];
		}
		for (i = 0; i < 128; i++)
			linecol[1023][i] = '\0';
		break;
	case 'i':
		if (li == 0) {
			dputs("?\n", 1);
			break;
		}
		--li;
		if (linecol[li][0] != '\0' && linecol[li][0] != '\n') {
			dputs(linecol[li], 1);
			dputs("\n", 1);
		}
		(void) dgets(line, sizeof(line) - 1, 0);
		for (i = 0; i < 128; i++)
			linecol[li][i] = '\0';
		for (i = 0; i < strlen(line); i++)
			linecol[li][i] = line[i];
		linecol[li][i] = '\n';
		for (i = 0; linecol[i][0] != '\0'; i++)
			;
		if (i < li) {
			for (j = 0; j < 128; j++)
				linecol[i][j] = linecol[li][j];
			for (j = 0; j < 128; j++)
				linecol[li][j] = '\0';
		}
		break;
	case 'n':
		if (linecol[1023][0] != '\0') {
			dputs("planck: error: cannot add line, already at limit\n", 2);
			break;
		}
		for (i = 1022; i > li - 1; i--) {
			if (linecol[i][0] != '\0') {
				for (j = 0; j < 128; j++)
					linecol[i + 1][j] = linecol[i][j];
				for (j = 0; j < 128; j++)
					linecol[i][j] = '\0';
			}
		}
		linecol[li][0] = '\n';
		break;
	case 'p':
		if (li == 0) {
			for (i = 0; linecol[i][0] != '\0' ; i++)
				dputs(linecol[i], 1);
		} else if (linecol[li - 1][0] == '\0') {
			dputs("?\n", 1);
		} else {
			dputs(linecol[li - 1], 1);
		}
		break;
	case 'q':
		goto done;
	case 's':
		if (argc == 1) {
			dputs("File: ", 1);
			dgets(file, sizeof(file), 0);
		}

		if ((fd = open(file, 0x0001 | 0x0200, 000644)) == -1) {
			dputs("planck: error: could not open ", 2);
			dputs(file, 2);
			dputs("\n", 2);
			break;
		}

		for (li = 0; li < 1024; li++) {
			if (linecol[li][0] == '\0')
				break;
			dputs(linecol[li], fd);
		}

		if (close(fd) == -1) {
			dputs("planck: error: could not close ", 2);
			dputs(file, 2);
			dputs("\n", 2);

			_exit(1);
		}
		break;
	default:
		dputs("?\n", 1);
	}

	goto get_command;

done:
	return 0;
}
