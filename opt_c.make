BENCHMARKS_OPT_C= \
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

$(eval $(call BINDIRS_RULE,optimized_c))

define CHECK_RULE_OPT_C
bin/optimized_c/check/$(1): bin/optimized_c/check/$(dir $(1)) ref/utilities/polybench.c optimized_c/check/$(1)/$(notdir $(1)).c optimized_c/check/$(1)/generated.c
	clang $(CHECK_ARGS) -Wno-incompatible-pointer-types -fopenmp -I ref/utilities -I optimized_c/check/$(1) ref/utilities/polybench.c optimized_c/check/$(1)/$(notdir $(1)).c optimized_c/check/$(1)/generated.c -o $$@ -lm -lblas
endef

$(foreach bench,$(BENCHMARKS_OPT_C),$(eval $(call CHECK_RULE_OPT_C,$(bench))))

define RUN_RULE_OPT_C
bin/optimized_c/run/$(1): bin/optimized_c/run/$(dir $(1)) ref/utilities/polybench.c optimized_c/check/$(1)/$(notdir $(1)).c optimized_c/check/$(1)/generated.c
	clang $(RUN_ARGS) -Wno-incompatible-pointer-types -fopenmp -I ref/utilities -I optimized_c/check/$(1) ref/utilities/polybench.c optimized_c/check/$(1)/$(notdir $(1)).c optimized_c/check/$(1)/generated.c -o $$@ -lm -lblas
endef

$(foreach bench,$(BENCHMARKS_OPT_C),$(eval $(call RUN_RULE_OPT_C,$(bench))))

define OPTIMIZE_RULE_OPT_C
optimized_c/check/$(1)/$(notdir $(1)).c: build/optimize
	./build/optimize check $(notdir $(1))
endef

$(foreach bench,$(BENCHMARKS_OPT_C),$(eval $(call OPTIMIZE_RULE_OPT_C,$(bench))))

check-opt_c: $(foreach bench,$(BENCHMARKS_OPT_C),bin/optimized_c/check/$(bench))

run-opt_c: $(foreach bench,$(BENCHMARKS_OPT_C),bin/optimized_c/run/$(bench))

PHONYLIST+=check-opt_c run-opt_c
CHECKLIST+=check-opt_c
RUNLIST+=run-opt_c