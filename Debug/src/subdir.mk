################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hlSrio578.c \
../src/hlSrioBase.c \
../src/hlSrioConfig.c \
../src/hlSrioConfigSelf.c \
../src/hlSrioDebug.c \
../src/hlSrioOther.c \
../src/hlSrioReConfig.c \
../src/hlSrioReInit.c \
../src/hlSrioShow.c 

OBJS += \
./src/hlSrio578.o \
./src/hlSrioBase.o \
./src/hlSrioConfig.o \
./src/hlSrioConfigSelf.o \
./src/hlSrioDebug.o \
./src/hlSrioOther.o \
./src/hlSrioReConfig.o \
./src/hlSrioReInit.o \
./src/hlSrioShow.o 

C_DEPS += \
./src/hlSrio578.d \
./src/hlSrioBase.d \
./src/hlSrioConfig.d \
./src/hlSrioConfigSelf.d \
./src/hlSrioDebug.d \
./src/hlSrioOther.d \
./src/hlSrioReConfig.d \
./src/hlSrioReInit.d \
./src/hlSrioShow.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 Linux gcc compiler'
	arm-linux-gnueabihf-gcc -Wall -O0 -g3 -I../../standalone_bsp_0/ps7_cortexa9_0/include -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


