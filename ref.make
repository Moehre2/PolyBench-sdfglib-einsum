BENCHMARKS_REF= \
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
	stencils/adi \
	stencils/fdtd-2d \
	stencils/heat-3d \
	stencils/jacobi-1d \
	stencils/jacobi-2d \
	stencils/seidel-2d

$(foreach bindirs,check/ref run/ref,$(eval $(call BINDIRS_RULE,$(bindirs))))

define CHECK_RULE_REF
bin/check/ref/$(1): bin/check/ref/$(dir $(1)) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c
	clang $(CHECK_ARGS) -I ref/utilities -I ref/$(1) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c -o $$@ -lm
endef

$(foreach bench,$(BENCHMARKS_REF),$(eval $(call CHECK_RULE_REF,$(bench))))

define RUN_RULE_REF
bin/run/ref/$(1): bin/run/ref/$(dir $(1)) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c
	clang $(RUN_ARGS) -I ref/utilities -I ref/$(1) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c -o $$@ -lm
endef

$(foreach bench,$(BENCHMARKS_REF),$(eval $(call RUN_RULE_REF,$(bench))))

check-ref: $(foreach bench,$(BENCHMARKS_REF),bin/check/ref/$(bench))

run-ref: $(foreach bench,$(BENCHMARKS_REF),bin/run/ref/$(bench))

PHONYLIST+=check-ref run-ref
CHECKLIST+=check-ref
RUNLIST+=run-ref