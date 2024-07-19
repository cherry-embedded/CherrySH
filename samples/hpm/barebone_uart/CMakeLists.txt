# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYRB 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(cherryshell)

sdk_compile_options("-O2")

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
