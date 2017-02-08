find_package(LinuxDVB REQUIRED)

list(APPEND WPE_TVCONTROL_SOURCES
    src/bcm-rpi/tvcontrol-backend.cpp
)
