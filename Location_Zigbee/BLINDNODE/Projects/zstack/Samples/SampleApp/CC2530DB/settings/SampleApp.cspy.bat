@REM This batch file has been generated by the IAR Embedded Workbench
@REM C-SPY Debugger, as an aid to preparing a command line for running
@REM the cspybat command line utility using the appropriate settings.
@REM
@REM You can launch cspybat by typing the name of this batch file followed
@REM by the name of the debug file (usually an ELF/DWARF or UBROF file).
@REM Note that this file is generated every time a new debug session
@REM is initialized, so you may want to move or rename the file before
@REM making changes.
@REM 


"E:\Zigbee\common\bin\cspybat" "E:\Zigbee\8051\bin\8051proc.dll" "E:\Zigbee\8051\bin\8051emu_cc.dll"  %1 --plugin "E:\Zigbee\8051\bin\8051bat.dll" --backend -B "--proc_core" "plain" "--proc_code_model" "banked" "--proc_nr_virtual_regs" "16" "--proc_pdata_bank_reg_addr" "0xA0" "--proc_dptr_nr_of" "1" "--proc_DPL1" "0x84" "--proc_DPH1" "0x85" "--proc_codebank_reg" "0x9F" "--proc_codebank_start" "0x8000" "--proc_codebank_end" "0xFFFF" "--proc_codebank_mask" "0x07" "--proc_data_model" "large" "-p" "E:\Zigbee\8051\config\devices\_generic\io8052.ddf" "--proc_exclude_exit_breakpoint" "--proc_driver" "chipcon" "--retain_memory" "--verify_download" "use_crc16" "--stack_overflow" "--number_of_banks" "4" 


