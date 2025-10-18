rwildcard = $(foreach d,$1,$(wildcard $d/$2) $(call rwildcard,$(wildcard $d/*),$2))

PREFIX ?= /usr/local
CC ?= gcc
CFLAGS += -Iinclude -Wall -Wextra -Wpedantic -std=c23
OPTFLAGS += -O2
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

SRCS_VECTORIZE_OBJS = $(SRCS_VECTORIZE:%.c=$(BLDDIR)/%.o)

OBJ_VARIANTS = $(basename $1)_avx2.o $(basename $1)_sse2.o
OBJS_VECTORIZE = $(foreach s,$(SRCS_VECTORIZE_OBJS),$(call OBJ_VARIANTS,$s))
OBJS_NO_VECTORIZE = $(filter-out $(SRCS_VECTORIZE_OBJS),$(OBJS))

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(OBJS_VECTORIZE) $(LDFLAGS) -o $@

$(OBJS_NO_VECTORIZE): $(BLDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(SRCS_VECTORIZE_OBJS): $(BLDDIR)/%.o: %.c $(call OBJ_VARIANTS,$(BLDDIR)/%.o)
	@mkdir -p $(dir $@)
	$(CC) -c $< -mavx2 -mfma -DSD_VECTORIZE=SD_VECTORIZE_AVX2 $(CFLAGS) $(OPTFLAGS) -o $(basename $@)_avx2.o
	$(CC) -c $< -msse2 -DSD_VECTORIZE=SD_VECTORIZE_SSE2 $(CFLAGS) $(OPTFLAGS) -o $(basename $@)_sse2.o
	$(CC) -c $< $(CFLAGS) $(OPTFLAGS) $(DEPFLAGS) -o $@

$(OBJS_VECTORIZE):


.PHONY: clean
clean:
	find $(BLDDIR) -type f \( -name *.o -o -name *.d \) -exec rm -f {} +
	rm -f $(BIN)

-include $(DEPS)
