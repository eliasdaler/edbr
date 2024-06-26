function(set_exe_target_common_props target)
  if(WIN32)
    set_target_properties(${target}
      PROPERTIES
        LINK_FLAGS_DEBUG   "/SUBSYSTEM:CONSOLE"
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
    copy_deps_to_runtime_dir(${target})
  endif()

  set_target_properties(${target} PROPERTIES
    INSTALL_RPATH $ORIGIN/lib
  )
endfunction()
