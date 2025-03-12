################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/ADDA.c \
../src/Amplifier_Switch.c \
../src/Communications_Protocol.c \
../src/FFT2C.c \
../src/FFT2C_emxAPI.c \
../src/FFT2C_emxutil.c \
../src/FFT2C_initialize.c \
../src/FFT2C_terminate.c \
../src/FFTImplementationCallback.c \
../src/FFT_Main.c \
../src/MostLike_FindFrequence.c \
../src/Msg_Que.c \
../src/My_KissFft.c \
../src/cJSON.c \
../src/eml_setop.c \
../src/fft.c \
../src/findpeaks.c \
../src/freertos_hello_world.c \
../src/rtGetInf.c \
../src/rtGetNaN.c \
../src/rt_nonfinite.c 

OBJS += \
./src/ADDA.o \
./src/Amplifier_Switch.o \
./src/Communications_Protocol.o \
./src/FFT2C.o \
./src/FFT2C_emxAPI.o \
./src/FFT2C_emxutil.o \
./src/FFT2C_initialize.o \
./src/FFT2C_terminate.o \
./src/FFTImplementationCallback.o \
./src/FFT_Main.o \
./src/MostLike_FindFrequence.o \
./src/Msg_Que.o \
./src/My_KissFft.o \
./src/cJSON.o \
./src/eml_setop.o \
./src/fft.o \
./src/findpeaks.o \
./src/freertos_hello_world.o \
./src/rtGetInf.o \
./src/rtGetNaN.o \
./src/rt_nonfinite.o 

C_DEPS += \
./src/ADDA.d \
./src/Amplifier_Switch.d \
./src/Communications_Protocol.d \
./src/FFT2C.d \
./src/FFT2C_emxAPI.d \
./src/FFT2C_emxutil.d \
./src/FFT2C_initialize.d \
./src/FFT2C_terminate.d \
./src/FFTImplementationCallback.d \
./src/FFT_Main.d \
./src/MostLike_FindFrequence.d \
./src/Msg_Que.d \
./src/My_KissFft.d \
./src/cJSON.d \
./src/eml_setop.d \
./src/fft.d \
./src/findpeaks.d \
./src/freertos_hello_world.d \
./src/rtGetInf.d \
./src/rtGetNaN.d \
./src/rt_nonfinite.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -IC:/Users/saber/Desktop/share/DanDike_NewBoard/vitis2/system_wrapper/export/system_wrapper/sw/system_wrapper/rtos_ps7_crotex9_1/bspinclude/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


