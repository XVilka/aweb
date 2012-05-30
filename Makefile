# Some variables
FASM = fasm  
PRG = aweb

################################
# Default target
prog: all

################################
# Objects
all: 
	$(FASM) aweb.asm $(PRG)

