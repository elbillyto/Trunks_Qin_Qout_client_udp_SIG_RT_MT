################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Trunks_Qin_Qout_client_udp_SIG_RT_MT.c \
../src/udpclient.c 

OBJS += \
./src/Trunks_Qin_Qout_client_udp_SIG_RT_MT.o \
./src/udpclient.o 

C_DEPS += \
./src/Trunks_Qin_Qout_client_udp_SIG_RT_MT.d \
./src/udpclient.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


