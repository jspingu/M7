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
OBJS_VECTORIZE_NEON = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_neon.o)

DEPS_VECTORIZE_AVX2 = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_avx2.d)
DEPS_VECTORIZE_SSE2 = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_sse2.d)
DEPS_VECTORIZE_NEON = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%_neon.d)

VECTORIZATION ?= dynamic
PREDEFINED_MACROS := $(shell $(CC) $(CFLAGS) -E -dM - < /dev/null)

TARGET_ARCH = $(findstring x86_64,$(PREDEFINED_MACROS)) $(findstring i386,$(PREDEFINED_MACROS)) \
              $(findstring aarch64,$(PREDEFINED_MACROS)) $(findstring arm,$(PREDEFINED_MACROS))

ifeq ($(strip $(TARGET_ARCH)),x86_64)
POSSIBLE_SIMD_EXTENSIONS = AVX2
endif

ifeq ($(strip $(TARGET_ARCH)),i386)
POSSIBLE_SIMD_EXTENSIONS = SSE2
endif

ifeq ($(strip $(TARGET_ARCH)),arm)
POSSIBLE_SIMD_EXTENSIONS = NEON
endif

AVAILABLE_SIMD_EXTENSIONS = $(findstring AVX2,$(PREDEFINED_MACROS)) \
							$(findstring SSE2,$(PREDEFINED_MACROS)) \
							$(findstring NEON,$(PREDEFINED_MACROS))

BASE_SIMD_EXTENSION = $(firstword $(AVAILABLE_SIMD_EXTENSIONS))
SIMD_VARIANTS = $(filter-out $(AVAILABLE_SIMD_EXTENSIONS),$(POSSIBLE_SIMD_EXTENSIONS))

ifeq ($(VECTORIZATION),dynamic)
BASE_VECTORIZATION_FLAGS = -DSD_DISPATCH_DYNAMIC
OBJS_VECTORIZE = $(foreach v,$(SIMD_VARIANTS),$(OBJS_VECTORIZE_$v))
DEPS_VECTORIZE = $(foreach v,$(SIMD_VARIANTS),$(DEPS_VECTORIZE_$v))
else
ifeq ($(VECTORIZATION),static)
BASE_VECTORIZATION_FLAGS = -DSD_DISPATCH_STATIC
else
$(error Invalid vectorization option '$(VECTORIZATION)'. Valid options are 'static' or 'dynamic')
endif
endif

.PHONY: all
all: buildinfo $(BIN)

.PHONY: buildinfo
buildinfo:
	$(info Target architecture: $(firstword $(TARGET_ARCH) unknown))
	$(info Base SIMD extension: $(firstword $(BASE_SIMD_EXTENSION) none))
ifeq ($(VECTORIZATION),dynamic)
	$(info Configured with dynamic dispatch vectorization)
	$(info Object variants will be built for the following SIMD extensions: $(SIMD_VARIANTS))
else
ifeq ($(VECTORIZATION),static)
	$(info Configured with static dispatch vectorization)
endif
endif

$(BIN): $(OBJS_VECTORIZE) $(OBJS) $(BLDDIR)/gamma.o
	$(CC) $(OBJS) $(OBJS_VECTORIZE) $(BLDDIR)/gamma.o $(LDFLAGS) -o $@

$(OBJS): $(BLDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) $(BASE_VECTORIZATION_FLAGS) -c $< -o $@

$(OBJS_VECTORIZE_AVX2): $(BLDDIR)/%_avx2.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -mavx2 -mfma -DSD_DISPATCH_DYNAMIC -DSD_SRC_VARIANT -c $< -o $@

$(OBJS_VECTORIZE_SSE2): $(BLDDIR)/%_sse2.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -msse2 -DSD_DISPATCH_DYNAMIC -DSD_SRC_VARIANT -c $< -o $@

$(OBJS_VECTORIZE_NEON): $(BLDDIR)/%_neon.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -march=armv7 -DSD_DISPATCH_DYNAMIC -DSD_SRC_VARIANT -c $< -o $@

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
