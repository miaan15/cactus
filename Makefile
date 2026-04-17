OUT_DIR = build
FLAGS = -collection:cactus=cactus
MODE ?= debug

ifeq ($(MODE), release)
	FLAGS += -o:speed -no-bounds-check
	MODE_MSG = "RELEASE"
else ifeq ($(MODE), debug)
	FLAGS += -debug
	MODE_MSG = "DEBUG"
else
	$(error "Unknown MODE: $(MODE). Use 'debug' or 'release'")
endif

demo: $(OUT_DIR) prebuild
	@mkdir -p $(OUT_DIR)/demo
	@echo "Run demo: $(TARGET)..."
	odin run demo/$(TARGET)_demo $(FLAGS) -out:$(OUT_DIR)/demo/$(TARGET)

demo_build: $(OUT_DIR) prebuild
	@mkdir -p $(OUT_DIR)/demo
	@echo "Build demo: $(TARGET)..."
	odin build demo/$(TARGET)_demo $(FLAGS) -out:$(OUT_DIR)/demo/$(TARGET)

test: $(OUT_DIR) prebuild
	@mkdir -p $(OUT_DIR)/test
	@echo "Run test: $(TARGET)..."
	odin test test/$(TARGET)_test $(FLAGS) -out:$(OUT_DIR)/test/$(TARGET)
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

prebuild: 
	$(if $(TARGET),,$(error "TARGET is required! Usage: make build TARGET=your/path/to/target"))
	@echo "MODE: $(MODE_MSG)"
