if (PORT STREQUAL "GTK")
  WEBKIT_OPTION_DEPEND(USE_GSTREAMER_GL ENABLE_OPENGL)
endif ()
WEBKIT_OPTION_DEPEND(USE_GSTREAMER_GL ENABLE_VIDEO)
WEBKIT_OPTION_DEPEND(USE_GSTREAMER_MPEGTS ENABLE_VIDEO)
WEBKIT_OPTION_DEPEND(USE_WESTEROS_SINK ENABLE_VIDEO)
WEBKIT_OPTION_DEPEND(USE_WPE_VIDEO_PLANE_DISPLAY_DMABUF USE_GSTREAMER_GL)
