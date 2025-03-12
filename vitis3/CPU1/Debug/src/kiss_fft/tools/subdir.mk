################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/kiss_fft/tools/fftutil.c \
../src/kiss_fft/tools/kiss_fastfir.c \
../src/kiss_fft/tools/psdpng.c 

OBJS += \
./src/kiss_fft/tools/fftutil.o \
./src/kiss_fft/tools/kiss_fastfir.o \
./src/kiss_fft/tools/psdpng.o 

C_DEPS += \
./src/kiss_fft/tools/fftutil.d \
./src/kiss_fft/tools/kiss_fastfir.d \
./src/kiss_fft/tools/psdpng.d 


# Each subdirectory must supply rules for building sources it contributes
src/kiss_fft/tools/%.o: ../src/kiss_fft/tools/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -IC:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/system_wrapper/export/system_wrapper/sw/system_wrapper/freertos_ps7_cortexa9_1/bspinclude/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


