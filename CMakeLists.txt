# CMake build script for microkvs
# Intended to be integrated into a larger project, not built standalone.

add_library(microkvs STATIC
	driver/STM32QSPIStorageBank.cpp
	driver/STM32StorageBank.cpp
	driver/TestStorageBank.cpp

	kvs/KVS.cpp
	)

# TODO: only for stm32 targets?
target_include_directories(microkvs
	PUBLIC ${CMAKE_SOURCE_DIR}
	"$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
