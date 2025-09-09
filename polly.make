BENCHMARKS_POLLY= \
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

$(eval $(call BINDIRS_RULE,polly))

define POLLY_RULE
bin/polly/check/$(1): bin/polly/check/$(dir $(1)) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c
	clang $(CHECK_ARGS) -mllvm -polly -mllvm -polly-parallel -I ref/utilities -I ref/$(1) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c -o $$@ -lm -lgomp

bin/polly/run/$(1): bin/polly/run/$(dir $(1)) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c
	clang $(RUN_ARGS) -mllvm -polly -mllvm -polly-parallel -I ref/utilities -I ref/$(1) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c -o $$@ -lm -lgomp
endef

$(foreach bench,$(BENCHMARKS_POLLY),$(eval $(call POLLY_RULE,$(bench))))

check-polly: $(foreach bench,$(BENCHMARKS_POLLY),bin/polly/check/$(bench))

run-polly: $(foreach bench,$(BENCHMARKS_POLLY),bin/polly/run/$(bench))

PHONYLIST+=check-polly run-polly
CHECKLIST+=check-polly
RUNLIST+=run-polly