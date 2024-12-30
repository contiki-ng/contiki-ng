*** Settings ***
Suite Setup                   Setup
Suite Teardown                Teardown
Test Setup                    Reset Emulation
Test Teardown                 Test Teardown
Resource                      ${RENODEKEYWORDS}

*** Variables ***
${UART}                       sysbus.uart0
${URI}                        @https://dl.antmicro.com/projects/renode

*** Keywords ***
Create Machine
    [Arguments]     ${elf}      ${name}     ${id}

    Execute Command             mach create ${name}
    Execute Command             using sysbus
    Execute Command             machine LoadPlatformDescription @platforms/cpus/cc2538.repl
    Execute Command             connector Connect radio wireless

    Execute Command             machine PyDevFromFile @scripts/pydev/rolling-bit.py 0x400D2004 0x4 True "sysctrl"

    Execute Command             sysbus WriteDoubleWord 0x00280028 ${id}
    Execute Command             sysbus WriteDoubleWord 0x0028002C 0x00
    Execute Command             sysbus WriteDoubleWord 0x00280030 0xAB
    Execute Command             sysbus WriteDoubleWord 0x00280034 0x89
    Execute Command             sysbus WriteDoubleWord 0x00280038 0x00
    Execute Command             sysbus WriteDoubleWord 0x0028003C 0x4B
    Execute Command             sysbus WriteDoubleWord 0x00280040 0x12
    Execute Command             sysbus WriteDoubleWord 0x00280044 0x00

    Execute Command             sysbus LoadBinary @https://dl.antmicro.com/projects/renode/cc2538_rom_dump.bin-s_524288-0c196cdc21b5397f82e0ff42b206d1cc4b6d7522 0x0
    Execute Command             sysbus LoadELF ${elf}
    Execute Command             cpu VectorTableOffset `sysbus GetSymbolAddress "vectors"`


*** Test Cases ***
Should Talk Over Wireless Network
    Set Test Variable           ${REPEATS}      3

    Execute Command             emulation CreateIEEE802_15_4Medium "wireless"
    Execute Command             wireless SetRangeWirelessFunction 11

    Create Machine              @${CURDIR}/build/cc2538dk/udp-server.cc2538dk       "server"      1
    Execute Command             wireless SetPosition radio 0 0 0
    ${server-tester}=           Create Terminal Tester      ${UART}     machine=server
    Execute Command             mach clear

    Create Machine              @${CURDIR}/build/cc2538dk/udp-client.cc2538dk       "client-1"    2
    Execute Command             wireless SetPosition radio 10 0 0
    ${client1-tester}=          Create Terminal Tester      ${UART}     machine=client-1
    Execute Command             mach clear

    Create Machine              @${CURDIR}/build/cc2538dk/udp-client.cc2538dk       "client-2"    3
    Execute Command             wireless SetPosition radio 0 10 0
    ${client2-tester}=          Create Terminal Tester      ${UART}     machine=client-2
    Execute Command             mach clear


    Start Emulation

    FOR  ${i}  IN RANGE  0  ${REPEATS}
        Wait For Line On Uart       Received request 'hello (\\d+)' from fd00::200:0:0:2       testerId=${server-tester}        treatAsRegex=true      timeout=100
        Wait For Line On Uart       Sending response.  	                                       testerId=${server-tester}        treatAsRegex=true      timeout=100
        Wait For Line On Uart       Received request 'hello (\\d+)' from fd00::200:0:0:3       testerId=${server-tester}        treatAsRegex=true      timeout=100
        Wait For Line On Uart       Sending response.                                          testerId=${server-tester}        treatAsRegex=true      timeout=100
    END

    FOR  ${i}  IN RANGE  0  ${REPEATS}
        Wait For Line On Uart       Sending request ${i} to fd00::200:0:0:1                    testerId=${client1-tester}
        Wait For Line On Uart       Received response 'hello ${i}' from fd00::200:0:0:1        testerId=${client1-tester}
    END

    FOR  ${i}  IN RANGE  0  ${REPEATS}
        Wait For Line On Uart       Sending request ${i} to fd00::200:0:0:1                    testerId=${client2-tester}
        Wait For Line On Uart       Received response 'hello ${i}' from fd00::200:0:0:1        testerId=${client2-tester}
    END
