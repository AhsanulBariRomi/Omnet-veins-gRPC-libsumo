cmake_minimum_required(VERSION 3.10)
project(SumoGrpcServer)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

# 1. Where to find the SUMO Headers
include_directories(/home/mdahsanulbari/sumo/src)

# 2. Where to find the SUMO Library binaries
link_directories(/home/mdahsanulbari/sumo/bin)

add_executable(server main.cc sumo_cosimulation.pb.cc sumo_cosimulation.grpc.pb.cc)

# 3. Link the exact library name (sumocpp)
target_link_libraries(server 
    grpc++ 
    grpc++_reflection
    protobuf 
    sumocpp 
    Threads::Threads
)