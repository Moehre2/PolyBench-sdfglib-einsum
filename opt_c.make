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

$(foreach bindirs,check/optimized_c run/optimized_c,$(eval $(call BINDIRS_RULE,$(bindirs))))

define CHECK_RULE_OPT_C
bin/check/optimized_c/$(1): bin/check/optimized_c/$(dir $(1)) ref/utilities/polybench.c optimized_c/$(1)/$(notdir $(1)).c optimized_c/$(1)/generated.c
	clang $(CHECK_ARGS) -Wno-incompatible-pointer-types -fopenmp -I ref/utilities -I optimized_c/$(1) ref/utilities/polybench.c optimized_c/$(1)/$(notdir $(1)).c optimized_c/$(1)/generated.c -o $$@ -lm -lblas
endef

$(foreach bench,$(BENCHMARKS_OPT_C),$(eval $(call CHECK_RULE_OPT_C,$(bench))))

define RUN_RULE_OPT_C
bin/run/optimized_c/$(1): bin/run/optimized_c/$(dir $(1)) ref/utilities/polybench.c optimized_c/$(1)/$(notdir $(1)).c optimized_c/$(1)/generated.c
	clang $(RUN_ARGS) -Wno-incompatible-pointer-types -fopenmp -I ref/utilities -I optimized_c/$(1) ref/utilities/polybench.c optimized_c/$(1)/$(notdir $(1)).c optimized_c/$(1)/generated.c -o $$@ -lm -lblas
endef

$(foreach bench,$(BENCHMARKS_OPT_C),$(eval $(call RUN_RULE_OPT_C,$(bench))))

define OPTIMIZE_RULE_OPT_C
optimized_c/$(1)/$(notdir $(1)).c: build/optimize
	./build/optimize $(notdir $(1))
endef

$(foreach bench,$(BENCHMARKS_OPT_C),$(eval $(call OPTIMIZE_RULE_OPT_C,$(bench))))

check-opt_c: $(foreach bench,$(BENCHMARKS_OPT_C),bin/check/optimized_c/$(bench))

run-opt_c: $(foreach bench,$(BENCHMARKS_OPT_C),bin/run/optimized_c/$(bench))

PHONYLIST+=check-opt_c run-opt_c
CHECKLIST+=check-opt_c
RUNLIST+=run-opt_c