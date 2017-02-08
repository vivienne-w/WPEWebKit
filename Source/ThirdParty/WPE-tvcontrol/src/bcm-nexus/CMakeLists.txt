list(APPEND WPE_TVCONTROL_LIBRARIES
    nxpl
)

if (EGL_LIBRARIES_nxclient)
    #message("-- Found NXCLIENT: TRUE [egl.pc]")
    #add_definitions(-DBACKEND_BCM_NEXUS_NXCLIENT=1)
    list(APPEND WPE_TVCONTROL_LIBRARIES nxclient)
endif()

list(APPEND WPE_TVCONTROL_SOURCES
    src/bcm-nexus/tvcontrol-backend.cpp
    src/bcm-nexus/BcmTVManager.cpp
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PC_EGL_CFLAGS_OTHER}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PC_EGL_CFLAGS_OTHER}")
