 #
 # Copyright (C) 2016 TATA ELXSI
 # Copyright (C) 2016 Metrological
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 # 1. Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 # 2. Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 # "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 # LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 # PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 # HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 # SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 # LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 # DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 # THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 #/
 #  WPE_TVCONTROL_INCLUDE_DIR - TVControl  include directory
 #  WPE_TVCONTROL_LIBRARY - The library needed to use tvcontrol wpe


#**************************************************************
#Find the WPETVControl  LIBRARIES and INCLUDES
#
#************************************************************
find_package(PkgConfig)
pkg_check_modules(PC_WPETVCONTROL QUIET wpetvcontrol)

if (PC_WPETVCONTROL_FOUND)
    find_library(WPE_TVCONTROL_LIB
    NAMES WPE-tvcontrol
    PATHS  ${PC_WPETVCONTROL_LIBDIR}  ${PC_WPETVCONTROL_LIBRARY_DIRS}
)
endif ()

set(WPE_TVCONTROL_LIBRARY ${WPE_TVCONTROL_LIB})
include(FindPackageHandleStandardArgs)
set(WPE_TVCONTROL_LIBRARY ${WPE_TVCONTROL_LIBRARY}  CACHE PATH "Path to WPE-tvcontrol library")
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WPE_TVCONTROL DEFAULT_MSG  WPE_TVCONTROL_LIBRARY)
mark_as_advanced(WPE_TVCONTROL_LIBRARY)
