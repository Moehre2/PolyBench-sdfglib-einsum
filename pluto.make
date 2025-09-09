PLUTOCC ?= plutocc

BENCHMARKS_PLUTO= \
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
	linear-algebra/solvers/cholesky \
	linear-algebra/solvers/durbin \
	linear-algebra/solvers/gramschmidt \
	linear-algebra/solvers/lu \
	linear-algebra/solvers/ludcmp \
	linear-algebra/solvers/trisolv \
	medley/deriche \
	medley/floyd-warshall \
	medley/nussinov \
	stencils/fdtd-2d \
	stencils/heat-3d \
	stencils/jacobi-1d \
	stencils/jacobi-2d \
	stencils/seidel-2d

$(eval $(call BINDIRS_RULE,pluto))

pluto/:
	mkdir -p $@

define PLUTO_RULE
pluto/$(1)/: pluto/
	mkdir -p $$@

pluto/$(1)/$(notdir $(1)).c: pluto/$(1)/ ref/$(1)/$(notdir $(1)).c
	$(PLUTOCC) --silent --parallel --tile --noprevector --nounrolljam ref/$(1)/$(notdir $(1)).c -o $$@ &> /dev/null && mv $(notdir $(1)).pluto.cloog pluto/$(1)/

bin/pluto/check/$(1): bin/pluto/check/$(dir $(1)) ref/utilities/polybench.c pluto/$(1)/$(notdir $(1)).c
	clang $(CHECK_ARGS) -fopenmp -I ref/utilities -I ref/$(1) ref/utilities/polybench.c pluto/$(1)/$(notdir $(1)).c -o $$@ -lm

bin/pluto/run/$(1): bin/pluto/run/$(dir $(1)) ref/utilities/polybench.c pluto/$(1)/$(notdir $(1)).c
	clang $(RUN_ARGS) -fopenmp -I ref/utilities -I ref/$(1) ref/utilities/polybench.c pluto/$(1)/$(notdir $(1)).c -o $$@ -lm
endef

$(foreach bench,$(BENCHMARKS_PLUTO),$(eval $(call PLUTO_RULE,$(bench))))

check-pluto: $(foreach bench,$(BENCHMARKS_PLUTO),bin/pluto/check/$(bench))

run-pluto: $(foreach bench,$(BENCHMARKS_PLUTO),bin/pluto/run/$(bench))

PHONYLIST+=check-pluto run-pluto
CHECKLIST+=check-pluto
RUNLIST+=run-pluto