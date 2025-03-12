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
../src/Msg_Que.c \
../src/My_KissFft.c \
../src/cJSON.c \
../src/freertos_hello_world.c 

OBJS += \
./src/ADDA.o \
./src/Amplifier_Switch.o \
./src/Communications_Protocol.o \
./src/Msg_Que.o \
./src/My_KissFft.o \
./src/cJSON.o \
./src/freertos_hello_world.o 

C_DEPS += \
./src/ADDA.d \
./src/Amplifier_Switch.d \
./src/Communications_Protocol.d \
./src/Msg_Que.d \
./src/My_KissFft.d \
./src/cJSON.d \
./src/freertos_hello_world.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -IC:/Users/saber/Desktop/share/DanDike_NewBoard/vitis3/system_wrapper/export/system_wrapper/sw/system_wrapper/freertos_ps7_cortexa9_1/bspinclude/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


