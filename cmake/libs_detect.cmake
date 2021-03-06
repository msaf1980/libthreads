if(WIN32)
  set(LIB_PREFIX "")
  set(LIB_STATIC ".lib")
  set(LIB_SHARED ".dll")
elseif(APPLE)
  set(LIB_PREFIX "lib")
  set(LIB_STATIC ".a")
  set(LIB_SHARED ".dylib")
else()
  set(LIB_PREFIX "lib")
  set(LIB_STATIC ".a")
  set(LIB_SHARED ".so")
endif()
