cmake_minimum_required(VERSION 2.8)
project(DicomToStl)

set(DCMTK_INSTALL_DIR "" CACHE PATH "Installation path of DCMTK") 

set(DCMTK_LIBRARY_DIRS ${DCMTK_INSTALL_DIR}/lib) 
set(DCMTK_INCLUDE_DIRS ${DCMTK_INSTALL_DIR}/include)

include_directories(${DCMTK_INCLUDE_DIRS})
link_directories(${DCMTK_LIBRARY_DIRS})

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1 /MP2")
    set(CMAKE_CXX_FLAGS_MINSIZEREL     "/MT /O1 /Ob1 /D NDEBUG /MP")
    set(CMAKE_CXX_FLAGS_RELEASE        "/MT /O2 /Ob2 /D NDEBUG /MP")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1 /D NDEBUG /MP")
else()
    SET(CMAKE_CXX_FLAGS "-Wall -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g -DDEBUG ")
endif()

file(GLOB SRC_FILES src/*
    "*.h"
    "*.cpp"
)

add_executable(dicomtostl ${SRC_FILES})
target_link_libraries(dicomtostl dcmdata dcmimgle ofstd oflog ws2_32 netapi32)
