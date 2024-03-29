SET(execName ZED_Optimized_Grab)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 OLD)
	cmake_policy(SET CMP0015 OLD)
endif(COMMAND cmake_policy)

SET(EXECUTABLE_OUTPUT_PATH ".")


IF(WIN32) # Windows
    SET(ZED_INCLUDE_DIRS $ENV{ZED_INCLUDE_DIRS})
        if (CMAKE_CL_64) # 64 bits
                SET(ZED_LIBRARIES $ENV{ZED_LIBRARIES_64})
        else(CMAKE_CL_64) # 32 bits
                message("32bits compilation is no more available with CUDA7.5 and more")
        endif(CMAKE_CL_64)
    SET(ZED_LIBRARY_DIR $ENV{ZED_LIBRARY_DIR})
    SET(OPENCV_DIR $ENV{OPENCV_DIR})
    SET(Eigen_INCLUDE_DIRS $ENV{EIGEN_DIR})

   find_package(OpenCV 3.1)
ELSE() # Linux
    find_package(ZED 1.0 REQUIRED)
#    find_package(ZED 0.9 REQUIRED)
    SET(Eigen_INCLUDE_DIRS "/usr/include/eigen3") ##depends if installed by packet or by own build install
    SET(Eigen_INCLUDE_DIRS_2 "/usr/local/include/eigen3") ##depends if installed by packet or by own build install
    include_directories(${Eigen_INCLUDE_DIRS_2}) 

##For Jetson, OpenCV4Tegra is based on OpenCV2.4
     exec_program(uname ARGS -p OUTPUT_VARIABLE CMAKE_SYSTEM_NAME2)
     if ( CMAKE_SYSTEM_NAME2 MATCHES "aarch64" OR CMAKE_SYSTEM_NAME2 MATCHES "armv7l" )
        find_package(OpenCV 2.4)
     else()
      find_package(OpenCV 3.1)
     endif()

ENDIF(WIN32)

find_package(CUDA REQUIRED)
 
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${ZED_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${Eigen_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

link_directories(${ZED_LIBRARY_DIR})
link_directories(${OpenCV_LIBRARY_DIRS})
link_directories(${CUDA_LIBRARY_DIRS})

SET(SRC_FOLDER src)
FILE(GLOB_RECURSE SRC_FILES "${SRC_FOLDER}/*.cpp")

ADD_EXECUTABLE(${execName} ${SRC_FILES})
set_property(TARGET ${execName} PROPERTY OUTPUT_NAME "ZED Optimized Grab")

add_definitions(-std=c++0x) # -m64)

TARGET_LINK_LIBRARIES(${execName}
                        ${ZED_LIBRARIES}
                        ${OpenCV_LIBRARIES}
                        ${CUDA_LIBRARIES} ${CUDA_npps_LIBRARY} ${CUDA_nppi_LIBRARY}
                    )

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3" ) # Release Perf mode
