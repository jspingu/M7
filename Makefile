rwildcard = $(foreach d,$1,$(wildcard $d/$2) $(call rwildcard,$(wildcard $d/*),$2))

PREFIX ?= /usr/local
CC ?= gcc
CFLAGS += -Iinclude -Wall -Wextra -Wpedantic -std=c23
OPTFLAGS += -O3 -ffast-math
DEPFLAGS += -MMD -MP
LDFLAGS += -lSDL3

SRCDIR = src
BLDDIR = build

SRCS = $(call rwildcard,$(SRCDIR),*.c)
OBJS = $(SRCS:%.c=$(BLDDIR)/%.o)
DEPS = $(SRCS:%.c=$(BLDDIR)/%.d)
BIN = out

SRCS_VECTORIZE += $(SRCDIR)/Components/3D/M7_Rasterization.c
SRCS_VECTORIZE += $(SRCDIR)/Components/3D/M7_Geometry.c
SRCS_VECTORIZE += $(SRCDIR)/Components/3D/M7_Canvas.c
SRCS_VECTORIZE += $(SRCDIR)/Components/3D/M7_Xform.c

OBJS_VECTORIZE_AVX2 = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_avx2.o)
OBJS_VECTORIZE_SSE2 = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_sse2.o)

DEPS_VECTORIZE_AVX2 = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_avx2.d)
DEPS_VECTORIZE_SSE2 = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_sse2.d)

VECTORIZATION ?= dynamic
PREDEFINED_MACROS = $(shell $(CC) $(CFLAGS) -E -dM - < /dev/null)

ifneq ($(findstring x86_64,$(PREDEFINED_MACROS)),)
TARGET_ARCH = x86_64
POSSIBLE_SIMD_EXTENSIONS = AVX2
endif

ifneq ($(findstring i386,$(PREDEFINED_MACROS)),)
TARGET_ARCH = i386
POSSIBLE_SIMD_EXTENSIONS = SSE2
endif

AVAILABLE_SIMD_EXTENSIONS = $(findstring AVX2,$(PREDEFINED_MACROS)) $(findstring SSE2,$(PREDEFINED_MACROS))
BASE_SIMD_EXTENSION = $(firstword $(AVAILABLE_SIMD_EXTENSIONS))

SIMD_VARIANTS = $(filter-out $(AVAILABLE_SIMD_EXTENSIONS),$(POSSIBLE_SIMD_EXTENSIONS))
OBJS_VECTORIZE = $(foreach v,$(SIMD_VARIANTS),$(OBJS_VECTORIZE_$v))
DEPS_VECTORIZE = $(foreach v,$(SIMD_VARIANTS),$(DEPS_VECTORIZE_$v))

# ifeq ($(VECTORIZATION),dynamic)
# $(info Building with dynamic dispatch vectorization)
# $(info Object variants will be built for the following SIMD extensions: $(SIMD_VARIANTS))
# BASE_VECTORIZATION_FLAGS = -DSD_DISPATCH_DYNAMIC
# else
# ifeq ($(VECTORIZATION),static)
# $(info Building with static dispatch vectorization)
# BASE_VECTORIZATION_FLAGS = -DSD_DISPATCH_STATIC
# else
# $(error Invalid vectorization option '$(VECTORIZATION)'. Valid options are 'static' or 'dynamic')
# endif
# endif

.PHONY: all
all: buildinfo $(BIN)

.PHONY: buildinfo
buildinfo:
	$(info Target architecture: $(firstword $(TARGET_ARCH) unknown))
	$(info Base SIMD extension: $(firstword $(BASE_SIMD_EXTENSION) none))

$(BIN): $(OBJS_VECTORIZE) $(OBJS) $(BLDDIR)/gamma.o
	$(CC) $(OBJS) $(OBJS_VECTORIZE) $(BLDDIR)/gamma.o $(LDFLAGS) -o $@

$(OBJS): $(BLDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -DSD_BASE $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(OBJS_VECTORIZE_AVX2): $(BLDDIR)/%_avx2.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -mavx2 -mfma $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(OBJS_VECTORIZE_SSE2): $(BLDDIR)/%_sse2.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -msse2 $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(BLDDIR)/gamma.o: $(BLDDIR)/gamma.c
	@mkdir -p $(BLDDIR)
	$(CC) -c $< -o $@

$(BLDDIR)/gamma.c: scripts/gengamma.c
	@mkdir -p $(BLDDIR)
	cc $< -lm -o $(BLDDIR)/gengamma
	$(BLDDIR)/gengamma > $@

.PHONY: clean
clean:
	find $(BLDDIR) -type f \( -name *.c -o -name *.o -o -name *.d \) -exec rm -f {} +
	rm -f $(BLDDIR)/gengamma
	rm -f $(BIN)

-include $(DEPS_VECTORIZE) $(DEPS)
