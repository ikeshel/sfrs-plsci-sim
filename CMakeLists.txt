cmake_minimum_required(VERSION 3.16)
project(sfrs-plsci-sim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})
find_package(ROOT REQUIRED COMPONENTS Core RIO Hist Tree)
add_executable(sfrs-plsci-sim
  src/main.cc
  src/DetectorConstruction.cc
  src/PrimaryGeneratorAction.cc
  src/ActionInitialization.cc
  src/EventAction.cc
  src/SteppingAction.cc
  src/StackingAction.cc
  src/RootOutput.cc
)
target_include_directories(sfrs-plsci-sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sfrs-plsci-sim PRIVATE ${Geant4_LIBRARIES} ROOT::Core ROOT::RIO ROOT::Hist ROOT::Tree)
