# définition des cibles particulières
.PHONY: clean, mrproper

# désactivation des règles implicites
.SUFFIXES:

UNAME_S:=$(shell uname -s)

CC=gcc
CL=clang
CFLAGS= -O3 -Wall -W -Wstrict-prototypes -Werror -Wextra -Wuninitialized
ifeq ($(UNAME_S),Linux)
	IFLAGSDIR= -I/usr/include
	LFLAGSDIR= -L/usr/lib
	COMPIL=$(CC)
endif
ifeq ($(UNAME_S),Darwin)
	IFLAGSDIR= -I/opt/local/include
	LFLAGSDIR= -L/opt/local/lib
	COMPIL=$(CL)
endif
GL_FLAGS= -lGL -lGLU -lglut
MATH_FLAGS= -lm
PNG_FLAGS= -lpng

all: dest_sys visChaos3d visFractal3d

visChaos3d: visChaos3d.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(GL_FLAGS) $(MATH_FLAGS) $(PNG_FLAGS) $< -o $@

visFractal3d: visFractal3d.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(GL_FLAGS) $(MATH_FLAGS) $(PNG_FLAGS) $< -o $@


dest_sys:
	@echo "Destination system:" $(UNAME_S)

clean:
	@rm -f visChaos3d
	@rm -f visFractal3d
