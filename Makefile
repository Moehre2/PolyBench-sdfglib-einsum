CHECK_ARGS=-O0 -DPOLYBENCH_DUMP_ARRAYS -DMEDIUM_DATASET -DDATA_TYPE_IS_DOUBLE
RUN_ARGS=-O3 -DPOLYBENCH_TIME -DEXTRALARGE_DATASET -DDATA_TYPE_IS_DOUBLE

all: check run

clean:
	rm -rf bin/ optimized_*/

bin/:
	mkdir -p $@

DIRS= \
	datamining \
	linear-algebra \
	linear-algebra/blas \
	linear-algebra/kernels \
	linear-algebra/solvers \
	medley \
	stencils

define BINDIRS_RULE_INTERNAL
bin/$(1)/check/$(2)/: $(dir bin/$(1)/check/$(2))
	mkdir -p $$@

bin/$(1)/run/$(2)/: $(dir bin/$(1)/run/$(2))
	mkdir -p $$@
endef

define BINDIRS_RULE
bin/$(1)/: bin/
	mkdir -p $$@

bin/$(1)/check/: bin/$(1)/
	mkdir -p $$@

bin/$(1)/run/: bin/$(1)/
	mkdir -p $$@

$(foreach dir,$(DIRS),$(eval $(call BINDIRS_RULE_INTERNAL,$(1),$(dir))))
endef

PHONYLIST=clean check run all
CHECKLIST=
RUNLIST=

include ref.make
include opt_c.make

.PHONY: $(PHONYLIST)

check: $(CHECKLIST)

run: $(RUNLIST)