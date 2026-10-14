if(NOT TARGET utility)
    ensure_submodule_initialized(utility)
    add_subdirectory(utility)
endif()
