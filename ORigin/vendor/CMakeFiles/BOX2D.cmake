﻿add_library(BOX2D STATIC
    ${VENDOR_DIR}/Box2D/src/collision/b2_broad_phase.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_chain_shape.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_circle_shape.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_collide_circle.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_collide_edge.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_collide_polygon.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_collision.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_distance.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_dynamic_tree.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_edge_shape.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_polygon_shape.cpp
    ${VENDOR_DIR}/Box2D/src/collision/b2_time_of_impact.cpp
    ${VENDOR_DIR}/Box2D/src/common/b2_block_allocator.cpp
    ${VENDOR_DIR}/Box2D/src/common/b2_draw.cpp
    ${VENDOR_DIR}/Box2D/src/common/b2_math.cpp
    ${VENDOR_DIR}/Box2D/src/common/b2_settings.cpp
    ${VENDOR_DIR}/Box2D/src/common/b2_stack_allocator.cpp
    ${VENDOR_DIR}/Box2D/src/common/b2_timer.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_body.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_chain_circle_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_chain_polygon_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_circle_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_contact_manager.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_contact_solver.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_distance_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_edge_circle_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_edge_polygon_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_fixture.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_friction_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_gear_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_island.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_motor_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_mouse_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_polygon_circle_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_polygon_contact.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_prismatic_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_pulley_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_revolute_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_weld_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_wheel_joint.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_world.cpp
    ${VENDOR_DIR}/Box2D/src/dynamics/b2_world_callbacks.cpp
    ${VENDOR_DIR}/Box2D/src/rope/b2_rope.cpp
)

target_include_directories(BOX2D PRIVATE
  ${VENDOR_DIR}/Box2D/include
  ${VENDOR_DIR}/Box2D/src
)