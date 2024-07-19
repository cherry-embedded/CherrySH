# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYRB 1)
set(CONFIG_FREERTOS 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(cherryshell)

sdk_compile_options("-O2")
sdk_compile_definitions(-D__freertos_irq_stack_top=_stack)
sdk_compile_definitions(-DCONFIG_FREERTOS=1)
sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)
sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=1)

sdk_inc(inc)
sdk_inc(../../../)

sdk_app_src(
    src/main.c
    src/shell.c
    ../../../cherryrl/chry_readline.c
    ../../../chry_shell.c
    ../../../builtin/help.c
    ../../../builtin/shsize.c
    ../../../builtin/login.c
)

generate_ses_project()
