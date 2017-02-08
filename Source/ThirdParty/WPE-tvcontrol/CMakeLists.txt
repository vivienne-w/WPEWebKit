set(WPE_TVCONTROL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE-tvcontrol")

option(USE_WPE_TVCONTROL_BACKEND_LINUX_DVB "Whether to enable support for the linux dvb tv backend" OFF)
option(USE_WPE_TVCONTROL_BACKEND_BCM "Whether to enable support for the bcm tv backend" OFF)

set(WPE_TVCONTROL_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source/ThirdParty/WPE/include"
)

if (ENABLE_TV_CONTROL)
    add_definitions(-DENABLE_TV_CONTROL)
endif()

if (USE_WPE_TVCONTROL_BACKEND_LINUX_DVB)
    find_package(LinuxDVB REQUIRED)
    list(APPEND WPE_TVCONTROL_INCLUDE_DIRECTORIES
        "${WPE_TVCONTROL_SOURCE_DIR}/src/linux-dvb"
        ${LINUX_DVB_INCLUDE_DIRS}
    )
    if (USE_WPE_BACKEND_BCM_RPI)
        list(APPEND WPE_TVCONTROL_INCLUDE_DIRECTORIES
             "${WPE_TVCONTROL_SOURCE_DIR}/src/bcm-rpi"
        )
    endif()
    list(APPEND WPE_TVCONTROL_LIBRARIES
        ${LINUX_DVB_LIBRARIES}
    )
    list(APPEND WPE_TVCONTROL_SOURCES
        src/linux-dvb/SourceBackend.cpp
        src/linux-dvb/TunerBackend.cpp
        src/linux-dvb/ChannelBackend.cpp
    )
    add_definitions(-DTVCONTROL_BACKEND_LINUX_DVB=1)
endif()

if (USE_WPE_BACKEND_BCM_NEXUS)
    add_definitions(-DTVCONTROL_BACKEND_BCM=1)
    include(src/bcm-nexus/CMakeLists.txt)
endif ()

add_library(WPE-tvcontrol SHARED ${WPE_TVCONTROL_SOURCES})
target_include_directories(WPE-tvcontrol PRIVATE ${WPE_TVCONTROL_INCLUDE_DIRECTORIES})
target_link_libraries(WPE-tvcontrol ${WPE_TVCONTROL_LIBRARIES})
install(TARGETS WPE-tvcontrol DESTINATION "${LIB_INSTALL_DIR}")
