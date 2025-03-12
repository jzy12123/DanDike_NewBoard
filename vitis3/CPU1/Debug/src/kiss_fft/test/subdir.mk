################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/kiss_fft/test/benchfftw.c \
../src/kiss_fft/test/benchkiss.c \
../src/kiss_fft/test/doit.c \
../src/kiss_fft/test/pstats.c \
../src/kiss_fft/test/test_real.c \
../src/kiss_fft/test/test_simd.c \
../src/kiss_fft/test/twotonetest.c 

OBJS += \
./src/kiss_fft/test/benchfftw.o \
./src/kiss_fft/test/benchkiss.o \
./src/kiss_fft/test/doit.o \
./src/kiss_fft/test/pstats.o \
./src/kiss_fft/test/test_real.o \
./src/kiss_fft/test/test_simd.o \
./src/kiss_fft/test/twotonetest.o 

C_DEPS += \
./src/kiss_fft/test/benchfftw.d \
./src/kiss_fft/test/benchkiss.d \
./src/kiss_fft/test/doit.d \
./src/kiss_fft/test/pstats.d \
./src/kiss_fft/test/test_real.d \
./src/kiss_fft/test/test_simd.d \
./src/kiss_fft/test/twotonetest.d 


# Each subdirectory must supply rules for building sources it contributes
src/kiss_fft/test/%.o: ../src/kiss_fft/test/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -IC:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/system_wrapper/export/system_wrapper/sw/system_wrapper/freertos_ps7_cortexa9_1/bspinclude/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


