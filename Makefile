#
# Copyright (c) 2009 HIIT <http://www.hiit.fi/>
# All rights reserved.
#
# Author: Wojciech A. Koszek <wkoszek@FreeBSD.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $Id$
#

#
# netfpga_base_beta_2_0_0/NF2 is the top-level directory of the 
# NetFPGA-beta archive.
#
BITDIR=		NetFPGA/bitfiles

CFLAGS+=	-O0 -g -ggdb -Wall -Wextra

CFLAGS+=	-DXBF_TEST_PROG

all:	regen xbf

xbf:	xbf.c Makefile
	$(CC) $(CFLAGS) xbf.c -o xbf

prog:	xbf.c Makefile
	$(CC) $(CFLAGS) -DXBF_TEST_PROG xbf.c -o xbf

rtest:
	./xbf -d /tmp/_.xbf_tests -r all

fetch:
	git clone https://github.com/insop/NetFPGA.git

test:	prog rtest
	rm -rf tests/libxbf.out
	./xbf $(BITDIR)/cpci.bit >> tests/libxbf.out
	./xbf $(BITDIR)/cpci_reprogrammer.bit >> tests/libxbf.out
	./xbf $(BITDIR)/crypto_nic.bit >> tests/libxbf.out
	./xbf $(BITDIR)/dram_queue_test.bit >> tests/libxbf.out
	./xbf $(BITDIR)/dram_router.bit >> tests/libxbf.out
	./xbf $(BITDIR)/reference_nic.bit >> tests/libxbf.out
	./xbf $(BITDIR)/reference_router.bit >> tests/libxbf.out
	./xbf $(BITDIR)/reference_switch.bit >> tests/libxbf.out
	./xbf $(BITDIR)/router_buffer_sizing.bit >> tests/libxbf.out
	./xbf $(BITDIR)/selftest.bit >> tests/libxbf.out
	./xbf $(BITDIR)/tutorial_router.bit >> tests/libxbf.out
	diff -u tests/libxbf.t tests/libxbf.out

regen:
	@printf '\t/* autogenerated from Makefile! */\n' > _.t
	@grep '^TEST_DECL' xbf.c | cut -d "(" -f 2 | cut -d "," -f 1 |	\
	while read T; do						\
		printf "\tTEST_UNIT(%s)\n" $${T} >> _.t;		\
	done;
	@mv _.t xbf_tests.h
	@echo "# xbf_tests.h regenerated"

testman:
	groff -man -Tascii xbf.3

clean:
	rm -rf xbf

