BENCHMARKS_OPT_CUBLAS= \
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

$(eval $(call BINDIRS_RULE,optimized_cublas))

define OPT_CUBLAS_RULE
bin/optimized_cublas/check/$(1): bin/optimized_cublas/check/$(dir $(1)) gpu/polybench.cu optimized_cublas/check/$(1)/$(notdir $(1)).cu optimized_cublas/check/$(1)/generated.cu
	nvcc $(CHECK_ARGS) -I gpu -I optimized_cublas/check/$(1) gpu/polybench.cu optimized_cublas/check/$(1)/$(notdir $(1)).cu optimized_cublas/check/$(1)/generated.cu -o $$@ -lcublas

bin/optimized_cublas/run/$(1): bin/optimized_cublas/run/$(dir $(1)) gpu/polybench.cu optimized_cublas/run/$(1)/$(notdir $(1)).cu optimized_cublas/run/$(1)/generated.cu
	nvcc $(RUN_ARGS) -I gpu -I optimized_cublas/run/$(1) gpu/polybench.cu optimized_cublas/run/$(1)/$(notdir $(1)).cu optimized_cublas/run/$(1)/generated.cu -o $$@ -lcublas

optimized_cublas/check/$(1)/$(notdir $(1)).cu: build/optimize_cublas
	./build/optimize_cublas check $(notdir $(1))

optimized_cublas/run/$(1)/$(notdir $(1)).cu: build/optimize_cublas
	./build/optimize_cublas run $(notdir $(1))
endef

$(foreach bench,$(BENCHMARKS_OPT_CUBLAS),$(eval $(call OPT_CUBLAS_RULE,$(bench))))

check-opt_cublas: $(foreach bench,$(BENCHMARKS_OPT_CUBLAS),bin/optimized_cublas/check/$(bench))

run-opt_cublas: $(foreach bench,$(BENCHMARKS_OPT_CUBLAS),bin/optimized_cublas/run/$(bench))

PHONYLIST+=check-opt_cublas run-opt_cublas
CHECKLIST+=check-opt_cublas
RUNLIST+=run-opt_cublas