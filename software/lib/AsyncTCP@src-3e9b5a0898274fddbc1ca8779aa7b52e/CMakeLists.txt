set(COMPONENT_SRCDIRS
    "src"
)

set(COMPONENT_ADD_INCLUDEDIRS
    "src"
)

register_component()

target_compile_options(${COMPONENT_TARGET} PRIVATE -fno-rtti)
