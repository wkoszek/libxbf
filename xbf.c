/*-
 * Copyright (c) 2009 HIIT <http://www.hiit.fi/>
 * All rights reserved.
 *
 * Author: Wojciech A. Koszek <wkoszek@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 *
 * This file implements a functions to handle Xilinx .bit file header
 * format. Layout was taken form:
 *
 * http://www.fpga-faq.com/FAQ_Pages/0026_Tell_me_about_bit_files.htm
 *
 * Description contains 2 "Field 4" sections -- and there is 7, not 6
 * fields. It's changed within this file. 
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "xbf.h"

#define ASSERT		assert
#define ARRAY_SIZE(x)	((int)(sizeof(x)/sizeof(x[0])))
static int		xbf_debug = 1;

/*
 * Try to setup correct values for the library further usage.
 */
static int
_xbf_setup(struct xbf *xbf)
{
	uint32_t u32;
	uint16_t u16;
	uint8_t u8;
	char *ptr;
	void *endptr;

	xbf_assert(xbf);

	/*
	 * Helper functions that let me to access a memory with
	 * different granularity easily from a byte pointer.
	 */
#define U32(ptr)			\
	(htonl(*(uint32_t *)ptr))
#define U16(ptr)			\
	(htons(*(uint16_t *)ptr))
#define U8(ptr)				\
	(*(char *)(ptr))

	ptr = xbf->_xbf_mem;
	endptr = ((char *)xbf->_xbf_mem + xbf->_xbf_memsize);
#define LEFT()				\
	 ((unsigned)((void *)endptr - (void *)ptr))

#define WHDR "Wrong header format! "

	/*
	 * Field 1:
	 * 2 bytes          length 0x0009           (big endian) 
	 * 9 bytes          some sort of header
	 */
	u16 = U16(ptr);
	if (u16 != 9)
		return (xbf_erri(xbf, WHDR "Field1's length should be 9, "
		    "but is %d", u16));
	ptr += 2;
	ptr += u16;

	/*
	 * Field 2
	 * 2 bytes          length 0x0001
	 * 1 byte           key 0x61                (The letter "a")
	 */
	u16 = U16(ptr);
	if (u16 != 1)
		return (xbf_erri(xbf, WHDR "Field2's length should be 1, "
		    "but is %d", u16));
	ptr += 2;
	u8 = U8(ptr);
	if (u8 != 'a')
		return (xbf_erri(xbf, WHDR "Magic 'a' missing (%#hhx)", u8));
	ptr += 1;

	/*
	 * Field 3 
	 * 2 bytes          length 0x000a           (value depends on file name length) 
	 * 10 bytes         string design name "xform.ncd" (including a trailing 0x00) 
	 */
	u16 = U16(ptr);
	if (u16 >= LEFT())
		return (xbf_erri(xbf, WHDR "NCD filename seems to be to long"
		    " (%d)", u16));
	ptr += 2;
	if (ptr[u16 - 1] != '\0')
		return (xbf_erri(xbf, WHDR "NCF filename isn't terminated with 0"));
	xbf->xbf_ncdname = ptr;
	ptr += u16;

	/*
	 * Field 4 
	 * 1 byte           key 0x62                (The letter "b") 
	 * 2 bytes          length 0x000c           (value depends on part name length) 
	 * 12 bytes         string part name "v1000efg860" (including a  trailing 0x00)
	 */
	u8 = U8(ptr);
	if (u8 != 'b')
		return (xbf_erri(xbf, WHDR "Magic 'b' missing (%#hhx)", u8));
	ptr += 1;

	u16 = U16(ptr);
	if (u16 >= LEFT())
		return (xbf_erri(xbf, WHDR "Part name to long (%d)", u16));
	ptr += 2;
	if (ptr[u16 - 1] != '\0')
		return (xbf_erri(xbf, WHDR, "Part name isn't terminated with 0"));
	xbf->xbf_partname = ptr;
	ptr += u16;

	/*
	 * Field 5
	 * 1 byte           key 0x63                (The letter "c") 
	 * 2 bytes          length 0x000b 
	 * 11 bytes         string date "2001/08/10"  (including a trailing 0x00)
	 */
	u8 = U8(ptr);
	if (u8 != 'c')
		return (xbf_erri(xbf, WHDR "Magic 'c' missing (%#hhx)", u8));
	ptr += 1;
	u16 = U16(ptr);
	if (u16 != 11)
		return (xbf_erri(xbf, WHDR "Field5's length should be 11, "
		    "but is %d", u16));
	ptr += 2;
	if (ptr[u16 - 1] != '\0')
		return (xbf_erri(xbf, WHDR, "Date isn't terminated with 0"));
	xbf->xbf_date = ptr;
	ptr += u16;

	/*
	 * Field 6
	 * 1 byte           key 0x64                (The letter "d") 
	 * 2 bytes          length 0x0009 
	 * 9 bytes          string time "06:55:04"    (including a trailing 0x00)
	 */
	u8 = U8(ptr);
	if (u8 != 'd')
		return (xbf_erri(xbf, WHDR "Magic 'd' missing (%hhx)", u8));
	ptr += 1;

	u16 = U16(ptr);
	if (u16 != 9)
		return (xbf_erri(xbf, WHDR "6th field length should be 9, "
		    "but is %d", u16));
	ptr += 2;
	if (ptr[u16 - 1] != '\0')
		return (xbf_erri(xbf, WHDR, "Time isn't terminated with 0"));
	xbf->xbf_time = ptr;
	ptr += u16;

	/*
	 * Field 7 
	 * 1 byte           key 0x65                 (The letter "e") 
	 * 4 bytes          length 0x000c9090        (value depends on device type,
	 * and maybe design details) 
	 */
	u8 = U8(ptr);
	if (u8 != 'e')
		return (xbf_erri(xbf, WHDR "Magic 'e' missing (%#hhx)", u8));
	ptr += 1;
	u32 = U32(ptr);
	if (u32 >= LEFT())
		return (xbf_erri(xbf, WHDR "Part name filename seems to be to long")); 
	ptr += 4;
	xbf->xbf_len = u32;
	xbf->xbf_data = ptr;
#undef WHDR
#undef U32
#undef U16
#undef U8
#undef LEFT
	return (0);
}

/*
 * Put error message in internal library's buffer.
 */
static void
xbf_err_fmt(const char *func, int lineno, struct xbf *xbf, const char *fmt, va_list va)
{
	struct _xbf_err *e;

	xbf_assert(xbf);
	ASSERT(fmt != NULL);

	e = &xbf->_xbf_err;
	memset(e, 0, sizeof(*e));

	(void)vsnprintf(e->_xbf_errmsg, sizeof(e->_xbf_errmsg), fmt, va);
	(void)snprintf(e->_xbf_errsrc, sizeof(e->_xbf_errsrc), " [%s(%d)]",
	    func, lineno);
}

/*
 * Fill the error message and return library context.
 */
struct xbf *
_xbf_err(const char *func, int lineno, struct xbf *xbf, const char *fmt, ...)
{
	va_list va;

	xbf_assert(xbf);
	va_start(va, fmt);
	xbf_err_fmt(func, lineno, xbf, fmt, va);
	va_end(va);
	return (xbf);
}

/*
 * Fill the error message and return integer representing error
 * code
 */
int
_xbf_erri(const char *func, int lineno, struct xbf *xbf, const char *fmt, ...)
{
	va_list va;

	xbf_assert(xbf);
	va_start(va, fmt);
	xbf_err_fmt(func, lineno, xbf, fmt, va);
	va_end(va);
	return (-1);
}

/*
 * Fetch the error message.
 */
const char *
xbf_errmsg(struct xbf *xbf)
{
	struct _xbf_err *e;

	xbf_assert(xbf);
	e = &xbf->_xbf_err;
	if (xbf_debug)
		strlcat(e->_xbf_errmsg, e->_xbf_errsrc, sizeof(e->_xbf_errmsg));
	return (e->_xbf_errmsg);
}

/*
 * Was the library initialized correctly?
 */
int
xbf_opened(struct xbf *xbf)
{
	const char *estr;

	xbf_assert(xbf);
	estr = xbf_errmsg(xbf);
	return (estr[0] == '\0');
}

/*
 * Load a bit stream from a memory ``mem'' of length ``mem_size''
 * and try to setup a library context based on a memory contents.
 */
int
xbf_open_mem(struct xbf *xbf, void *mem, size_t mem_size)
{

	xbf_assert(xbf);
	if (!xbf_initialized(xbf))
		return (xbf_erri(xbf, "Call xbf_init() before "
		    "xbf_open_mem()!"));
	xbf->_xbf_mem = mem;
	xbf->_xbf_memsize = mem_size;
	if (xbf->xbf_fname != NULL)
		xbf->xbf_fname = "(memory)";
	return (_xbf_setup(xbf));
}

/*
 * Open a bit stream file and initialize a library context.
 */
int
xbf_open(struct xbf *xbf, const char *fname)
{
	struct stat st;
	int fd;
	int error;
	void *mem;

	xbf_assert(xbf);
	if (!xbf_initialized(xbf))
		return (xbf_erri(xbf, "Call xbf_init() before xbf_open()!"));
	fd = open(fname, O_RDONLY);
	if (fd == -1)
		return (xbf_erri(xbf, "Couldn't open file '%s'", fname));
	memset(&st, 0, sizeof(st));
	error = fstat(fd, &st);
	if (error == -1)
		return (xbf_erri(xbf, "Couldn't check file '%s' information",
		    fname));
	if (st.st_size < XBF_HDR_SIZE)
		return (xbf_erri(xbf, "File '%s' doesn't contain valid data",
		    fname));
	mem = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE,
	    MAP_PRIVATE, fd, 0);
	if (xbf->_xbf_mem == MAP_FAILED)
		return (xbf_erri(xbf, "Couldn't map file '%s' to memory", fname));
	xbf->xbf_fname = fname;
	xbf->_xbf_flags |= XBF_FLAG_MMAPED;
	error = xbf_open_mem(xbf, mem, st.st_size);
	return (error);
}

/*
 * Close a bit stream file
 */
int
xbf_close(struct xbf *xbf)
{
	int error;

	xbf_assert(xbf);
	ASSERT(xbf->_xbf_mem != NULL);
	if (xbf->_xbf_flags & XBF_FLAG_MMAPED)
		error = munmap(xbf->_xbf_mem, xbf->_xbf_memsize);
	ASSERT(error == 0);
	/*
	 * Initialize a state but clear all possible flags
	 */
	xbf_init(xbf);
	xbf->_xbf_flags = 0;
	return (error);
}

/*
 * Self-explanatory accessor functions below
 */
const char *
xbf_get_time(struct xbf *xbf)
{

	xbf_assert(xbf);
	return (xbf->xbf_time);
}

const char *
xbf_get_date(struct xbf *xbf)
{

	xbf_assert(xbf);
	return (xbf->xbf_date);
}

const char *
xbf_get_partname(struct xbf *xbf)
{

	xbf_assert(xbf);
	return (xbf->xbf_partname);
}

const char *
xbf_get_ncdname(struct xbf *xbf)
{

	xbf_assert(xbf);
	return (xbf->xbf_ncdname);
}

const char *
xbf_get_fname(struct xbf *xbf)
{

	xbf_assert(xbf);
	return (xbf->xbf_fname);
}

size_t
xbf_get_len(struct xbf *xbf)
{

	xbf_assert(xbf);
	return (xbf->xbf_len);
}

const unsigned char *
xbf_get_data(struct xbf *xbf)
{

	xbf_assert(xbf);
	return (xbf->xbf_data);
}

/*
 * Print information about bit stream file to the descriptor ``fp''
 */
void
xbf_print_fp(FILE *fp, struct xbf *xbf)
{

	ASSERT(fp != NULL);
	xbf_assert(xbf);

	(void)fprintf(fp, "NCD filename: %s\n", xbf_get_ncdname(xbf));
	(void)fprintf(fp, "   Part name: %s\n", xbf_get_partname(xbf));
	(void)fprintf(fp, "        Date: %s\n", xbf_get_date(xbf));
	(void)fprintf(fp, "        Time: %s\n", xbf_get_time(xbf));
	(void)fprintf(fp, "Image lenght: %d\n", xbf_get_len(xbf));
}

/*
 * Print information about bit stream file, but to the most popular file
 * descriptor -- stdout.
 */
void
xbf_print(struct xbf *xbf)
{

	xbf_assert(xbf);
	xbf_print_fp(stdout, xbf);
}

#ifdef XBF_TEST_PROG
static int flag_v = 0;
static int flag_r = 0;
const char *test_dir = NULL;

struct bf {
	/* Field 1 */
	int16_t	len1;
	char	hdr[9];

	/* Field 2 */
	int16_t	len2;
	int8_t	a;

	/* Field 3 */
	int16_t	len3;
	char	ncdname[10];
	
	/* Field 4 */
	int8_t	b;
	int16_t	len4;
	char	partname[12];

	/* Field 5 */
	int8_t	c;
	int16_t	len5;
	char	date[11];

	/* Field 6 */
	int8_t	d;
	int16_t	len6;
	char	time[9];

	/* Field 7 */
	int8_t	e;
	int32_t	len7;

	char	data[];
} __attribute__((packed));

typedef enum {
	TEST_OK,
	TEST_ER
} test_exerr_t;

struct test {
	struct bf	 *t_bf;
	test_exerr_t	  t_experr;
	const char	 *t_desc;
	int		 _t_num;
	const char	*_t_name;
};

#define TEST_DECL(bf, errcode, desc)		\
	static struct test test_##bf = {	\
		.t_bf = &(bf),			\
		.t_experr = (errcode),		\
		.t_desc = (desc),		\
		._t_num = __LINE__,		\
		._t_name = #bf,			\
	};

struct bf f1_nob = {
	.len1 = 9,
	.hdr = "__--__--|",
	.a = 'a',
	.len2 = 1,
};
TEST_DECL(f1_nob, TEST_ER, "Correct 1st field");

struct bf f1_ncdnonull = {
	.len1 = 9,
	.hdr = "__--__--|",

	.len2 = 1,
	.a = 'a',

	.len3 = 10,
	.ncdname = "0123456789",
	.b = 'b',


	.c = 'c',
	.d = 'd',
};
TEST_DECL(f1_ncdnonull, TEST_ER, "No null termination of the header");

struct bf f1_ncdnull = {
	.len1 = 9,
	.hdr = "__--__--|",

	.len2 = 1,
	.a = 'a',

	.len3 = 10,
	.ncdname = "012345678\0",
	.b = 'b',


	.c = 'c',
	.d = 'd',
};
TEST_DECL(f1_ncdnull, TEST_ER, "Is null, but ...");

struct bf f1_neglen = {
	.len1 = -1,
	.hdr = "12345678\0",
};
TEST_DECL(f1_neglen, TEST_ER, "Negative length in the header");

struct bf f1_lentoobig = {
	.len1 = 1000,
	.hdr = "12345678\0",
};
TEST_DECL(f1_lentoobig, TEST_ER, "Too long length in the header");

static void
bf_serialize(struct bf *raw, struct bf *b)
{

	/* Field */
	raw->len1 = ntohs(b->len1);
	memcpy(raw->hdr, b->hdr, sizeof(raw->hdr));

	/* Field 2 */
	raw->len2 = ntohs(b->len2);
	raw->a = b->a;

	/* Field 3 */
	raw->len3 = ntohs(b->len3);
	memcpy(raw->ncdname,  b->ncdname, sizeof(raw->ncdname));
	
	/* Field 4 */
	raw->b = b->b;
	raw->len4 = ntohs(b->len4);
	memcpy(raw->partname, b->partname, sizeof(raw->partname));

	/* Field 5 */
	raw->c = b->c;
	raw->len5 = ntohs(b->len5);
	memcpy(raw->date, b->date, sizeof(raw->date));

	/* Field 6 */
	raw->d = b->d;
	raw->len6 = ntohs(b->len6);
	memcpy(raw->time, b->time, sizeof(raw->time));

	/* Field 7 */
	raw->e = b->e;
	raw->len7 = ntohs(b->len7);
}

static test_exerr_t
bf_test(const char *dir_test, struct test *t, char **e)
{
	struct xbf xbf;
	char path[512];
	struct bf raw;
	int error;
	int fd;
	int l;

	ASSERT(dir_test != NULL);
	ASSERT(t != NULL);
	ASSERT(e != NULL);
	ASSERT(*e == NULL);

	error = mkdir(dir_test, 0700);
	ASSERT((error == 0 || errno == EEXIST) && "couldn't create directory");
	(void)snprintf(path, sizeof(path), "%s/%s.out", dir_test, t->_t_name);
	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
	ASSERT(fd != -1 && "couldn't create file? maybe it exists?");

	bf_serialize(&raw, t->t_bf);
	l = write(fd, &raw, sizeof(raw));
	ASSERT(l == sizeof(raw) && "didn't write whole structure");
	error = close(fd);
	ASSERT(error != -1 && "couldn't close a file");

	xbf_init(&xbf);
	error = xbf_open(&xbf, path);
	if (error != 0) {
		*e = strdup(xbf_errmsg(&xbf));
		ASSERT(*e != NULL);
		return (TEST_ER);
	}
	error = xbf_close(&xbf);
	ASSERT(error == 0 && "couldn't close xbf file");
	return (TEST_OK);
}

struct test *tests[] = {
#define	TEST_UNIT(bf)	&test_##bf,
#include "xbf_tests.h"
#undef TEST_UNIT
};

static void
usage(const char *prog)
{

	printf("%s [-vh] <filename>\n", prog);
	printf("%s -d <directory> -r all | <number>\n", prog);
	exit(EXIT_SUCCESS);
}

static int
regression_test(int argc, char **argv)
{
	struct test *t;
	int tnum;
	int i;
	test_exerr_t experr;
	const char *neg;
	char *e = NULL;
	unsigned indent;

	if (argc < 1) {
		usage(argv[0]);
		exit(1);
	}
	if (strcmp(argv[0], "all") == 0) {
		tnum = -1;
		printf("0..%d\n", ARRAY_SIZE(tests));
	} else
		tnum = atoi(argv[1]);

	for (i = 0, indent = 0; i < ARRAY_SIZE(tests); i++)
		if (strlen(tests[i]->t_desc) > indent)
			indent = strlen(tests[i]->t_desc);

	for (i = 0; i < ARRAY_SIZE(tests); i++) {
		if (tnum != -1 && tnum != i)
			continue;
		t = tests[i];
		experr = bf_test(test_dir, t, &e);
		if (experr == t->t_experr)
			neg = NULL;
		else
			neg = " not";
		printf("%d%s ok", i, (neg == NULL) ? "" : (neg));
		if (neg != NULL || (neg == NULL && flag_v)) {
			printf(" # Test: %-*s : Error: %s\n", indent,
			    t->t_desc, e);
		} else {
			printf("\n");
		}
		if (e != NULL)
			free(e);
		e = NULL;
		if (neg != NULL)
			exit(EXIT_FAILURE);

	}
	exit(EXIT_SUCCESS);
}

/*
 * Small program that tests functionality of xbf library
 */
int
main(int argc, char **argv)
{
	struct xbf xbf;
	char *fname = NULL;
	int o = -1;
	char *prog = NULL;

	prog = argv[0];
	while ((o = getopt(argc, argv, "d:rv")) != -1)
		switch (o) {
		case 'd':
			test_dir = optarg;
			break;
		case 'v':
			flag_v++;
			break;
		case 'r':
			flag_r++;
			break;
		case 'h':
			usage(prog);
			break;
		default:
			usage(prog);
			break;
		}

	argc -= optind;
	argv += optind;

	if (flag_r) {
		if (test_dir == NULL) {
			fprintf(stderr, "Temporary directory has to be "
			    "specified with -d <dir> argument.\n");
			usage(prog);
		}
		regression_test(argc, argv);
	}

	if (argc == 0)
		usage(prog);
	fname = argv[0];

	xbf_init(&xbf);
	if (xbf_open(&xbf, fname) != 0)
		errx(EXIT_FAILURE, "%s:", xbf_errmsg(&xbf));
	xbf_print(&xbf);
	xbf_close(&xbf);

	exit(EXIT_SUCCESS);
}
#endif /* XBF_TEST_PROG */
