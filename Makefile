TARGET  := aspic
SRCDIR  := src
SRC     := $(shell find $(SRCDIR) -name "*.c" -type f)
OBJDIR  := obj
OBJ     := $(SRC:%.c=$(OBJDIR)/%.o)
DEP     := $(SRC:%.c=$(OBJDIR)/%.d)

CC      := clang
CFLAGS  := -MMD -MP -I$(SRCDIR) -std=c11 -pedantic -g
WFLAGS  := -Wall -Wextra -Wwrite-strings
LDFLAGS := -lreadline

C_GREEN  := \033[1;32m
C_YELLOW := \033[1;33m
C_NONE   := \033[0m

$(TARGET): $(OBJ)
	@echo "$(C_GREEN)linking$(C_NONE) $@"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@echo "$(C_GREEN)compiling\033[0m $<"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

-include $(DEP)

clean:
	@echo "$(C_YELLOW)removing$(C_NONE) $(OBJDIR)/"
	-@rm -r $(OBJDIR)

mrproper: clean
	@echo "$(C_YELLOW)removing$(C_NONE) $(TARGET)"
	-@rm $(TARGET)

format:
	clang-format src/*c -i

all: mrproper $(TARGET)
