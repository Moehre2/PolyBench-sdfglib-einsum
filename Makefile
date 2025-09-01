CHECK_ARGS=-O0 -DPOLYBENCH_DUMP_ARRAYS -DMEDIUM_DATASET -DDATA_TYPE_IS_DOUBLE
RUN_ARGS=-O3 -DPOLYBENCH_TIME -DMEDIUM_DATASET -DDATA_TYPE_IS_DOUBLE

all: check run

clean:
	rm -rf bin/ optimized_*/

bin/:
	mkdir -p $@

bin/check/: bin/
	mkdir -p $@

bin/run/: bin/
	mkdir -p $@

define BINDIRS_RULE
bin/$(1)/: bin/$(dir $(1))
	mkdir -p $$@

bin/$(1)/datamining/: bin/$(1)/
	mkdir -p $$@

bin/$(1)/linear-algebra/: bin/$(1)/
	mkdir -p $$@

bin/$(1)/linear-algebra/blas/: bin/$(1)/linear-algebra/
	mkdir -p $$@

bin/$(1)/linear-algebra/kernels/: bin/$(1)/linear-algebra/
	mkdir -p $$@

bin/$(1)/linear-algebra/solvers/: bin/$(1)/linear-algebra/
	mkdir -p $$@

bin/$(1)/medley/: bin/$(1)/
	mkdir -p $$@

bin/$(1)/stencils/: bin/$(1)/
	mkdir -p $$@
endef

PHONYLIST=clean check run all
CHECKLIST=
RUNLIST=

include ref.make
include opt_c.make

.PHONY: $(PHONYLIST)

check: $(CHECKLIST)

run: $(RUNLIST)