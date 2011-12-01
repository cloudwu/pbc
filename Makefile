ifeq ($(OS),Windows_NT)
  SO := dll
  EXE := .exe
else
  SO := so
  EXE :=
endif

CC = gcc
CFLAGS = -O2

BUILD = build

LIBSRCS = context.c varint.c array.c pattern.c register.c proto.c map.c alloc.c rmessage.c wmessage.c bootstrap.c stringpool.c
LIBNAME = $(BUILD)/pbc.$(SO)

TESTSRCS = addressbook.c pattern.c
PROTOSRCS = addressbook.proto descriptor.proto

BUILD_O = $(BUILD)/o

all : lib test

lib : $(LIBNAME)

clean :
	rm -rf $(BUILD)

$(BUILD) : $(BUILD_O)

$(BUILD_O) :
	mkdir -p $@

LIB_O :=

define BUILD_temp
  TAR :=  $(BUILD_O)/$(notdir $(basename $(1)))
  LIB_O := $(LIB_O) $$(TAR).o
  $$(TAR).o : | $(BUILD_O)
  -include $$(TAR).d
  $$(TAR).o : src/$(1)
	$(CC) $(CFLAGS) -c -Isrc -I. -o $$@ -MMD $$<
endef

$(foreach s,$(LIBSRCS),$(eval $(call BUILD_temp,$(s))))

$(LIBNAME) : $(LIB_O)
	$(CC) --shared -o $(LIBNAME) $^

TEST :=

define TEST_temp
  TAR :=  $(BUILD)/$(notdir $(basename $(1)))
  TEST := $(TEST) $$(TAR)$$(EXE)
  $$(TAR)$$(EXE) : | $(BUILD)
  $$(TAR)$$(EXE) : | $(LIBNAME)
  $$(TAR)$$(EXE) : test/$(1)
	$(CC) $(CFLAGS) -I. -L$(BUILD) -lpbc -o $$@ $$<
endef

$(foreach s,$(TESTSRCS),$(eval $(call TEST_temp,$(s))))

test : $(TEST) proto

PROTO :=

define PROTO_temp
  TAR :=  $(BUILD)/$(notdir $(basename $(1)))
  PROTO := $(PROTO) $$(TAR).pb
  $$(TAR).pb : | $(BUILD)
  $$(TAR).pb : test/$(1)
	protoc -o$$@ $$<
endef

$(foreach s,$(PROTOSRCS),$(eval $(call PROTO_temp,$(s))))

proto : $(PROTO)

.PHONY : all lib test proto clean

