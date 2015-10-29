# Xilinx Bitstream Format Library

[![Build Status](https://travis-ci.org/wkoszek/libxbf.svg)](https://travis-ci.org/wkoszek/libxbf)

This library lets you open and read the header of the Xilinx `.bit` format.
It has no knowledge of anything inside of the bit-stream. It only knows
what the header is and what it means, how long is the data section and 
how many bytes there are.

# How to build

In order to build the library run:

	make

You can also run the regression test suite:

	make fetch

It'll fetch several `.bit` files from the official NetFPGA repository (be
warned -- it's 82MB by the time of writing this document). And then:

	make test

Which will verify that your changes didn't bring backward compatibility
problems. The check shows a differences between the output files after your
changes and 'golden model' references.

# API explanation

API calls are pretty self-explanatory and are mentioned below.

```
int
xbf_open(struct xbf *xbf, const char *fname);

int
xbf_open_mem(struct xbf *xbf, void *mem, size_t mem_size);

int
xbf_opened(struct xbf *xbf);

int
xbf_close(struct xbf *xbf);

const char *
xbf_errmsg(struct xbf *xbf);

const char *
xbf_get_time(struct xbf *xbf);

const char *
xbf_get_date(struct xbf *xbf);

const char *
xbf_get_partname(struct xbf *xbf);

const char *
xbf_get_ncdname(struct xbf *xbf);

const char *
xbf_get_fname(struct xbf *xbf);

size_t
xbf_get_len(struct xbf *xbf);

const unsigned char *
xbf_get_data(struct xbf *xbf);

void
xbf_print_fp(FILE *fp, struct xbf *xbf);

void
xbf_print(struct xbf *xbf);
```

# Author

- Wojciech Adam Koszek, [wojciech@koszek.com](mailto:wojciech@koszek.com)
- [http://www.koszek.com](http://www.koszek.com)
