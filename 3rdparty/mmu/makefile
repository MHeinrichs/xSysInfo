# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Matthias Heinrichs
#
# mmu.library - gcc preparation
#

SRCP      = src
INCP      = include
LOCP	  = locale
REFP      = reference
DSTP      = distribution
OBJP      = build
RELP      = release
DOCP      = docs



.PHONY : all clean source release check pack

all: $(OBJP) \
		$(REFP)/inline/mmu_protos.h \
		$(REFP)/inline/mmu.h \
		$(REFP)/proto/mmu.h \

clean:
	rm -rf $(OBJP) $(RELP) $(REFP)/inline/mmu_protos.h $(REFP)/inline/mmu.h \
		$(REFP)

#-- pragmas
$(REFP)/inline/mmu_protos.h: $(REFP)/fd/mmu_lib.fd $(REFP)/clib/mmu_protos.h
	mkdir -p $(REFP)/inline/
	fd2pragma --infile $(REFP)/fd/mmu_lib.fd --clib $(REFP)/clib/mmu_protos.h \
		--to $(REFP)/inline/ --special 70 --autoheader --comment

$(REFP)/inline/mmu.h: $(REFP)/fd/mmu_lib.fd $(REFP)/clib/mmu_protos.h
	mkdir -p $(REFP)/inline/
	fd2pragma --infile $(REFP)/fd/mmu_lib.fd --clib $(REFP)/clib/mmu_protos.h \
		--to $(REFP)/inline/ --special 40 --autoheader --comment

$(REFP)/proto/mmu.h: $(REFP)/fd/mmu_lib.fd $(REFP)/clib/mmu_protos.h
	mkdir -p $(REFP)/proto/
	fd2pragma --infile $(REFP)/fd/mmu_lib.fd --clib $(REFP)/clib/mmu_protos.h \
		--to $(REFP)/proto/ --special 38 --autoheader --comment