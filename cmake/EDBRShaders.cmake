# For 2D games, use:
#
#  get_edbr_common_2d_shaders(EDBR_2D_SHADERS)
#  target_shaders(platformer ${EDBR_2D_SHADERS})
#
# For 3D games:
#
#  get_edbr_common_3d_shaders(EDBR_3D_SHADERS)
#  target_shaders(platformer ${EDBR_3D_SHADERS})
#
# If the game has custom shaders, just append them to the list
# before passing it info target_shaders

# makes return(PROPAGATE ...) work
cmake_policy(SET CMP0140 NEW)

function(get_edbr_common_2d_shaders outVar)
  set(EDBR_2D_SHADERS
    fullscreen_triangle.vert
    sprite.vert
    sprite.frag
    crt_lottes.frag

    imgui.vert
    imgui.frag
  )

  get_target_property(EDBR_SOURCE_DIR edbr SOURCE_DIR)
  set(EDBR_SHADERS_DIR "${EDBR_SOURCE_DIR}/src/shaders/")
  list(TRANSFORM EDBR_2D_SHADERS PREPEND "${EDBR_SHADERS_DIR}")

  set(${outVar} ${EDBR_2D_SHADERS})
  return(PROPAGATE ${outVar})
endfunction()

# im3d has quite a few shaders...
function(get_im3d_shaders outVar)
  set(IM3D_SHADERS
    im3d_points.vert
    im3d_points.frag
    im3d_lines.vert
    im3d_lines.geom
    im3d_lines.frag
    im3d_triangles.vert
    im3d_triangles.frag
  )

  get_target_property(EDBR_SOURCE_DIR edbr SOURCE_DIR)
  set(IM3D_SHADERS_DIR "${EDBR_SOURCE_DIR}/src/shaders/im3d/")
  list(TRANSFORM IM3D_SHADERS PREPEND "${IM3D_SHADERS_DIR}")

  set(${outVar} ${IM3D_SHADERS})
  return(PROPAGATE ${outVar})
endfunction()

function(get_edbr_common_3d_shaders outVar)
  set(EDBR_3D_SHADERS
    fullscreen_triangle.vert
    skybox.frag
    skinning.comp
    mesh.vert
    mesh_depth_only.vert
    mesh_depth.frag
    mesh.frag
    postfx.frag
    depth_resolve.frag
    sprite.vert
    sprite.frag
    shadow_map_point.vert
    shadow_map_point.frag

    imgui.vert
    imgui.frag
  )

  get_target_property(EDBR_SOURCE_DIR edbr SOURCE_DIR)
  set(EDBR_SHADERS_DIR "${EDBR_SOURCE_DIR}/src/shaders/")
  list(TRANSFORM EDBR_3D_SHADERS PREPEND "${EDBR_SHADERS_DIR}")

  get_im3d_shaders(IM3D_SHADERS)
  list(APPEND EDBR_3D_SHADERS ${IM3D_SHADERS})

  set(${outVar} ${EDBR_3D_SHADERS})
  return(PROPAGATE ${outVar})
endfunction()
