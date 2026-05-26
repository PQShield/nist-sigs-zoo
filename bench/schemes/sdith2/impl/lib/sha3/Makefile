CC=gcc
WARNING_FLAGS=-Wall -Wextra -Wpedantic -Werror -Wno-unused-function
CFLAGS= -O3 -march=native $(WARNING_FLAGS) -std=gnu99 

COMMON_SOURCES=KeccakHash.c KeccakSponge.c KeccakSpongetimes4.c KeccakHashtimes4.c 
COMMON_OBJECTS=KeccakHash.o KeccakSponge.o KeccakSpongetimes4.o KeccakHashtimes4.o 

ifeq ($(PLATFORM), opt64)
	SPECIFIC_SOURCES=opt64/KeccakP-1600-opt64.c opt64/KeccakP-1600-times4-on1.c
	SPECIFIC_OBJECTS=opt64/KeccakP-1600-opt64.o opt64/KeccakP-1600-times4-on1.o
endif
ifeq ($(PLATFORM), avx2)
	SPECIFIC_SOURCES=avx2/KeccakP-1600-AVX2.s avx2/KeccakP-1600-times4-SIMD256.c
	SPECIFIC_OBJECTS=avx2/KeccakP-1600-AVX2.o avx2/KeccakP-1600-times4-SIMD256.o
endif

SOURCES=$(COMMON_SOURCES) $(SPECIFIC_SOURCES)
OBJECTS=$(COMMON_OBJECTS) $(SPECIFIC_OBJECTS)
SHA3LIB=libhash.a

%.o : %.c
	$(CC) -c $(CFLAGS) -I. -I./$(PLATFORM) $< -o $@

all: $(SOURCES) $(SHA3LIB)

$(SHA3LIB): $(OBJECTS)
	ar rcs $@ $^

%.o : %.s
#	as -o $@ $< -Dno_type -Dno_size -Dadd_underscore -Dno_plot
	$(CC) -c -Dno_type -Dno_size -Dadd_underscore -Dno_plt $< -o $@

#.c.o: 
#	    $(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm *.o $(PLATFORM)/*.o 2>/dev/null || true
	rm $(SHA3LIB) 2>/dev/null || true
