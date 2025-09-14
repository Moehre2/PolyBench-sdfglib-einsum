BENCHMARKS_OPT_MKL= \
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

$(eval $(call BINDIRS_RULE,optimized_mkl))

define OPT_MKL_RULE
bin/optimized_mkl/check/$(1): bin/optimized_mkl/check/$(dir $(1)) ref/utilities/polybench.c optimized_mkl/check/$(1)/$(notdir $(1)).c optimized_mkl/check/$(1)/generated.c
	clang $(CHECK_ARGS) -Wno-incompatible-pointer-types -DMKL_ILP64 -m64 -I$(MKLROOT)/include -fopenmp -I ref/utilities -I optimized_mkl/check/$(1) ref/utilities/polybench.c optimized_mkl/check/$(1)/$(notdir $(1)).c optimized_mkl/check/$(1)/generated.c -o $$@ -L$(MKLROOT)/lib -lmkl_rt -Wl,--no-as-needed -lpthread -lm -ldl

bin/optimized_mkl/run/$(1): bin/optimized_mkl/run/$(dir $(1)) ref/utilities/polybench.c optimized_mkl/run/$(1)/$(notdir $(1)).c optimized_mkl/run/$(1)/generated.c
	clang $(RUN_ARGS) -Wno-incompatible-pointer-types -DMKL_ILP64 -m64 -I$(MKLROOT)/include -fopenmp -I ref/utilities -I optimized_mkl/run/$(1) ref/utilities/polybench.c optimized_mkl/run/$(1)/$(notdir $(1)).c optimized_mkl/run/$(1)/generated.c -o $$@ -L$(MKLROOT)/lib -lmkl_rt -Wl,--no-as-needed -lpthread -lm -ldl

optimized_mkl/check/$(1)/$(notdir $(1)).c: build/optimize_mkl
	./build/optimize_mkl check $(notdir $(1))

optimized_mkl/run/$(1)/$(notdir $(1)).c: build/optimize_mkl
	./build/optimize_mkl run $(notdir $(1))
endef

$(foreach bench,$(BENCHMARKS_OPT_MKL),$(eval $(call OPT_MKL_RULE,$(bench))))

check-opt_mkl: $(foreach bench,$(BENCHMARKS_OPT_MKL),bin/optimized_mkl/check/$(bench))

run-opt_mkl: $(foreach bench,$(BENCHMARKS_OPT_MKL),bin/optimized_mkl/run/$(bench))

PHONYLIST+=check-opt_mkl run-opt_mkl
CHECKLIST+=check-opt_mkl
RUNLIST+=run-opt_mkl