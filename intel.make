BENCHMARKS_INTEL= \
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

$(eval $(call BINDIRS_RULE,intel))

define INTEL_RULE
bin/intel/check/$(1): bin/intel/check/$(dir $(1)) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c
	icx $(CHECK_ARGS) -xSAPPHIRERAPIDS -ipo -I ref/utilities -I ref/$(1) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c -o $$@ -lm

bin/intel/run/$(1): bin/intel/run/$(dir $(1)) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c
	icx $(RUN_ARGS) -xSAPPHIRERAPIDS -ipo -I ref/utilities -I ref/$(1) ref/utilities/polybench.c ref/$(1)/$(notdir $(1)).c -o $$@ -lm
endef

$(foreach bench,$(BENCHMARKS_INTEL),$(eval $(call INTEL_RULE,$(bench))))

check-intel: $(foreach bench,$(BENCHMARKS_INTEL),bin/intel/check/$(bench))

run-intel: $(foreach bench,$(BENCHMARKS_INTEL),bin/intel/run/$(bench))

PHONYLIST+=check-intel run-intel
CHECKLIST+=check-intel
RUNLIST+=run-intel