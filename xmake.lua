set_languages("c++23")
add_requires("openssl", "zlib")
target("pacbot2")
set_kind("binary")
add_links("ixwebsocket")
add_includedirs("include")
add_files("src/**.cpp")
add_syslinks("ssl", "crypto") -- OpenSSL dependencies
add_syslinks("z") -- Link zlib for gzip support

add_linkdirs("/usr/local/lib", "/usr/lib")

    -- Ensure OpenSSL and zlib are found
