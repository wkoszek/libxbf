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
 */

#ifndef _XBF_H_
#define _XBF_H_

/* Debugging */
#define ASSERT	assert

#define _XBF_ERRMSG_LEN	1024
struct _xbf_err {
	char	 _xbf_errmsg[_XBF_ERRMSG_LEN];
	char	 _xbf_errsrc[_XBF_ERRMSG_LEN / 8];
};

/*
 * Structure for representing Xilinx Bitstream File Header
 */
struct xbf {
	void		*_xbf_mem;
	size_t		 _xbf_memsize;
	struct _xbf_err	 _xbf_err;
	unsigned	 _xbf_flags;
	const char	*xbf_fname;
	const char	*xbf_ncdname;
	const char	*xbf_partname;
	const char	*xbf_time;
	const char	*xbf_date;
	uint32_t	 xbf_len;
	const char	*xbf_data;
};
#define XBF_FLAG_INITIALIZED	(1 << 0)
#define XBF_FLAG_MMAPED		(1 << 1)

/* Typical size of a header */
#define XBF_HDR_SIZE 72

/*
 * Keep this function in here and don't forget to modify it
 * if 'struct xbf' gets modified.
 */
static inline void
xbf_init(struct xbf *xbf)
{

	xbf->_xbf_mem = NULL;
	xbf->_xbf_memsize = 0;
	memset(&xbf->_xbf_err, 0, sizeof(xbf->_xbf_err));
	xbf->_xbf_flags = XBF_FLAG_INITIALIZED;

	xbf->xbf_fname = NULL;
	xbf->xbf_ncdname = NULL;
	xbf->xbf_partname = NULL;
	xbf->xbf_time = NULL;
	xbf->xbf_date = NULL;
	xbf->xbf_len = 0;
	xbf->xbf_data = NULL;
}

/*
 * Was structure initialized with xbf_init()?
 */
static inline int
xbf_initialized(struct xbf *xbf)
{

	return ((xbf->_xbf_flags & XBF_FLAG_INITIALIZED) != 0);
}

#define xbf_assert(xbf) do {						\
	ASSERT(xbf != NULL && "xbf can't be NULL here");		\
	ASSERT(xbf_initialized(xbf) != 0 &&				\
	    "xbf_initialized() must be called");			\
} while (0)

int xbf_open_mem(struct xbf *xbf, void *mem, size_t mem_size);
int xbf_open(struct xbf *xbf, const char *fname);
int xbf_close(struct xbf *xbf);
const char *xbf_errmsg(struct xbf *xbf);
struct xbf *_xbf_err(const char *func, int lineno, struct xbf *xbf,
    const char *fmt, ...);
int _xbf_erri(const char *func, int lineno, struct xbf *xbf,
    const char *fmt, ...);
size_t xbf_get_len(struct xbf *xbf);
const void *xbf_get_data(struct xbf *xbf);
const char *xbf_get_partname(struct xbf *xbf);
const char *xbf_get_date(struct xbf *xbf);
const char *xbf_get_time(struct xbf *xbf);
const char *xbf_get_ncdname(struct xbf *xbf);
const char *xbf_get_fname(struct xbf *xbf);
void xbf_print_fp(FILE *fp, struct xbf *xp);
void xbf_print(struct xbf *xbf);
int xbf_opened(struct xbf *xbf);

#define xbf_err(xbf, fmt, ...)						\
	(_xbf_err((__func__), (__LINE__), (xbf), (fmt), ##__VA_ARGS__))
#define xbf_erri(xbf, fmt, ...)						\
	(_xbf_erri((__func__), (__LINE__), (xbf), (fmt), ##__VA_ARGS__))

#ifndef strlcat
size_t strlcat(char * __restrict dst, const char * __restrict src, size_t siz);
#endif

#endif /* _XBF_H_ */
