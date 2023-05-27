cmake_minimum_required(VERSION 3.15.0)
set(HASH_HEADERS
    hash-library/hash/crc32.h
    hash-library/hash/hash.h
    hash-library/hash/hmac.h
    hash-library/hash/keccak.h
    hash-library/hash/md5.h
    hash-library/hash/sha1.h
    hash-library/hash/sha3.h
    hash-library/hash/sha256.h
)

set(HASH_SOURCES
    hash-library/crc32.cpp
    hash-library/digest.cpp
    hash-library/keccak.cpp
    hash-library/md5.cpp
    hash-library/sha1.cpp
    hash-library/sha3.cpp
    hash-library/sha256.cpp
)

add_library(hash STATIC
    ${HASH_HEADERS}
    ${HASH_SOURCES}
)

target_include_directories(hash PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/hash-library
)
