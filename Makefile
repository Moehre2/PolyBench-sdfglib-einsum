.PHONY: clean check run all

VERSIONS=ref optimized_c

BENCHMARKS= \
	datamining/correlation \
	datamining/covariance \
	linear-algebra/blas/gemm \
	linear-algebra/blas/gemver \
	linear-algebra/blas/gesummv \
	linear-algebra/blas/symm \
	linear-algebra/blas/syr2k \
	linear-algebra/blas/syrk \
	linear-algebra/blas/trmm \
	linear-algebra/kernels/2mm \
	linear-algebra/kernels/3mm \
	linear-algebra/kernels/atax \
	linear-algebra/kernels/bicg \
	linear-algebra/kernels/doitgen \
	linear-algebra/kernels/mvt \
	linear-algebra/solvers/gramschmidt \
	linear-algebra/solvers/trisolv \
	medley/deriche \
	stencils/adi \
	stencils/fdtd-2d \
	stencils/heat-3d \
	stencils/jacobi-1d \
	stencils/jacobi-2d \
	stencils/seidel-2d
#	linear-algebra/solvers/cholesky \
#	linear-algebra/solvers/durbin \
#	linear-algebra/solvers/lu \
#	linear-algebra/solvers/ludcmp \
#	medley/floyd-warshall \
#	medley/nussinov \

all: check run

check: $(foreach v,$(VERSIONS),$(foreach bench,$(BENCHMARKS),bin/check/$(v)/$(bench)))

run: $(foreach v,$(VERSIONS),$(foreach bench,$(BENCHMARKS),bin/run/$(v)/$(bench)))

clean:
	rm -rf bin/ optimized_c/

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

$(foreach v,$(VERSIONS),$(foreach bindirs,check/$(v) run/$(v),$(eval $(call BINDIRS_RULE,$(bindirs)))))

define CHECK_RULE
bin/check/$(1)/$(2): bin/check/$(1)/$(dir $(2)) ref/utilities/polybench.c $(1)/$(2)/$(notdir $(2)).c
	clang -Wno-incompatible-pointer-types -O0 -I ref/utilities -I $(1)/$(2) ref/utilities/polybench.c $(1)/$(2)/*.c -DPOLYBENCH_DUMP_ARRAYS -DMEDIUM_DATASET -DDATA_TYPE_IS_DOUBLE -o $$@ -lm -lblas
endef

$(foreach v,$(VERSIONS),$(foreach bench,$(BENCHMARKS),$(eval $(call CHECK_RULE,$(v),$(bench)))))

define RUN_RULE
bin/run/$(1)/$(2): bin/run/$(1)/$(dir $(2)) ref/utilities/polybench.c $(1)/$(2)/$(notdir $(2)).c
	clang -Wno-incompatible-pointer-types -O3 -I ref/utilities -I $(1)/$(2) ref/utilities/polybench.c $(1)/$(2)/*.c -DPOLYBENCH_TIME -DMEDIUM_DATASET -DDATA_TYPE_IS_DOUBLE -o $$@ -lm -lblas
endef

$(foreach v,$(VERSIONS),$(foreach bench,$(BENCHMARKS),$(eval $(call RUN_RULE,$(v),$(bench)))))

define OPTIMIZE_RULE
optimized_c/$(1)/$(notdir $(1)).c: build/optimize
	./build/optimize $(notdir $(1))
endef

$(foreach bench,$(BENCHMARKS),$(eval $(call OPTIMIZE_RULE,$(bench))))
