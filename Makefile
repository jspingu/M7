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

OBJS_VECTORIZE = $(OBJS_VECTORIZE_AVX2) $(OBJS_VECTORIZE_SSE2)
DEPS_VECTORIZE = $(DEPS_VECTORIZE_AVX2) $(DEPS_VECTORIZE_SSE2)

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS_VECTORIZE) $(OBJS) $(BLDDIR)/gamma.o
	$(CC) $(OBJS) $(OBJS_VECTORIZE) $(BLDDIR)/gamma.o $(LDFLAGS) -o $@

$(OBJS): $(BLDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(OBJS_VECTORIZE_AVX2): $(BLDDIR)/%_avx2.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -mavx2 -mfma -DSD_VECTORIZE=SD_VECTORIZE_AVX2 $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(OBJS_VECTORIZE_SSE2): $(BLDDIR)/%_sse2.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -msse2 -DSD_VECTORIZE=SD_VECTORIZE_SSE2 $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(BLDDIR)/gamma.o: $(BLDDIR)/gamma.c
	@mkdir -p $(BLDDIR)
	$(CC) -c $< -o $@

$(BLDDIR)/gamma.c: scripts/gengamma.c
	@mkdir -p $(BLDDIR)
	$(CC) $< -lm -o $(BLDDIR)/gengamma
	$(BLDDIR)/gengamma > $@

.PHONY: clean
clean:
	find $(BLDDIR) -type f \( -name *.c -o -name *.o -o -name *.d \) -exec rm -f {} +
	rm -f $(BLDDIR)/gengamma
	rm -f $(BIN)

-include $(DEPS_VECTORIZE) $(DEPS)
