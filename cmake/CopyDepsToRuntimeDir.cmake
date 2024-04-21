function(copy_deps_to_runtime_dir target)
	add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying OpenAL's .dll"
	  COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:OpenAL>, $<TARGET_FILE_DIR:${target}>"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:OpenAL>" "$<TARGET_FILE_DIR:${target}>"
	)
endfunction()