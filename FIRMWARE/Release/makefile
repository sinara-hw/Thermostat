################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./config.obj" \
"./enet_fs.obj" \
"./idle_task.obj" \
"./internal_adc.obj" \
"./leds.obj" \
"./lwip_task.obj" \
"./mqtt_client.obj" \
"./pid_task.obj" \
"./pwm.obj" \
"./send_command.obj" \
"./serial.obj" \
"./serial_task.obj" \
"./spi.obj" \
"./startup_ccs.obj" \
"./telnet.obj" \
"./thermostat.obj" \
"./uart_th.obj" \
"./watchdog.obj" \
"./drivers/pinout.obj" \
"./third_party/FreeRTOS/Source/croutine.obj" \
"./third_party/FreeRTOS/Source/list.obj" \
"./third_party/FreeRTOS/Source/queue.obj" \
"./third_party/FreeRTOS/Source/tasks.obj" \
"./third_party/FreeRTOS/Source/timers.obj" \
"./third_party/FreeRTOS/Source/portable/CCS/ARM_CM4F/port.obj" \
"./third_party/FreeRTOS/Source/portable/CCS/ARM_CM4F/portasm.obj" \
"./third_party/FreeRTOS/Source/portable/MemMang/heap_2.obj" \
"./utils/eeprom_pb.obj" \
"./utils/locator.obj" \
"./utils/lwiplib.obj" \
"./utils/ringbuf.obj" \
"./utils/uartstdio.obj" \
"./utils/ustdlib.obj" \
$(GEN_CMDS__FLAG) \
-llibc.a \
-l"C:/ti/TivaWare_C_Series-2.1.3.156/driverlib/ccs/Debug/driverlib.lib" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include drivers/subdir_vars.mk
-include third_party/FreeRTOS/Source/subdir_vars.mk
-include third_party/FreeRTOS/Source/portable/CCS/ARM_CM4F/subdir_vars.mk
-include third_party/FreeRTOS/Source/portable/MemMang/subdir_vars.mk
-include utils/subdir_vars.mk
-include subdir_rules.mk
-include drivers/subdir_rules.mk
-include third_party/FreeRTOS/Source/subdir_rules.mk
-include third_party/FreeRTOS/Source/portable/CCS/ARM_CM4F/subdir_rules.mk
-include third_party/FreeRTOS/Source/portable/MemMang/subdir_rules.mk
-include utils/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
thermostat_max.out \

EXE_OUTPUTS__QUOTED += \
"thermostat_max.out" \

BIN_OUTPUTS += \
thermostat_max.bin \

BIN_OUTPUTS__QUOTED += \
"thermostat_max.bin" \


# All Target
all: thermostat_max.out

# Tool invocations
thermostat_max.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: ARM Linker'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA1 --display_error_number --diag_wrap=off --diag_warning=225 --gen_func_subsections=on --ual -z -m"enet_s2e_ccs.map" --heap_size=0 --stack_size=4096 -i"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/lib" -i"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/include" --reread_libs --display_error_number --diag_wrap=off --warn_sections --xml_link_info="thermostat_max_linkInfo.xml" --rom_model -o "thermostat_max.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

thermostat_max.bin: $(EXE_OUTPUTS)
	@echo 'Building files: $(strip $(EXE_OUTPUTS__QUOTED))'
	@echo 'Invoking: ARM Hex Utility'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armhex"  -o "thermostat_max.bin" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $(strip $(EXE_OUTPUTS__QUOTED))'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(BIN_OUTPUTS__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "config.obj" "enet_fs.obj" "idle_task.obj" "internal_adc.obj" "leds.obj" "lwip_task.obj" "mqtt_client.obj" "pid_task.obj" "pwm.obj" "send_command.obj" "serial.obj" "serial_task.obj" "spi.obj" "startup_ccs.obj" "telnet.obj" "thermostat.obj" "uart_th.obj" "watchdog.obj" "drivers\pinout.obj" "third_party\FreeRTOS\Source\croutine.obj" "third_party\FreeRTOS\Source\list.obj" "third_party\FreeRTOS\Source\queue.obj" "third_party\FreeRTOS\Source\tasks.obj" "third_party\FreeRTOS\Source\timers.obj" "third_party\FreeRTOS\Source\portable\CCS\ARM_CM4F\port.obj" "third_party\FreeRTOS\Source\portable\CCS\ARM_CM4F\portasm.obj" "third_party\FreeRTOS\Source\portable\MemMang\heap_2.obj" "utils\eeprom_pb.obj" "utils\locator.obj" "utils\lwiplib.obj" "utils\ringbuf.obj" "utils\uartstdio.obj" "utils\ustdlib.obj" 
	-$(RM) "config.d" "enet_fs.d" "idle_task.d" "internal_adc.d" "leds.d" "lwip_task.d" "mqtt_client.d" "pid_task.d" "pwm.d" "send_command.d" "serial.d" "serial_task.d" "spi.d" "startup_ccs.d" "telnet.d" "thermostat.d" "uart_th.d" "watchdog.d" "drivers\pinout.d" "third_party\FreeRTOS\Source\croutine.d" "third_party\FreeRTOS\Source\list.d" "third_party\FreeRTOS\Source\queue.d" "third_party\FreeRTOS\Source\tasks.d" "third_party\FreeRTOS\Source\timers.d" "third_party\FreeRTOS\Source\portable\CCS\ARM_CM4F\port.d" "third_party\FreeRTOS\Source\portable\MemMang\heap_2.d" "utils\eeprom_pb.d" "utils\locator.d" "utils\lwiplib.d" "utils\ringbuf.d" "utils\uartstdio.d" "utils\ustdlib.d" 
	-$(RM) "third_party\FreeRTOS\Source\portable\CCS\ARM_CM4F\portasm.d" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-"C:/ti/ccsv8/utils/tiobj2bin/tiobj2bin" "thermostat_max.out" "thermostat_max.bin" "C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armofd" "C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armhex" "C:/ti/ccsv8/utils/tiobj2bin/mkhex4bin"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

