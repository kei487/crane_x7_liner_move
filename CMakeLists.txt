cmake_minimum_required(VERSION 3.5)
project(crane_x7_liner_move)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(angles REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(image_geometry REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/MoveTipStraight.srv"
)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

# ここから
add_executable(color_detection
  src/color_detection.cpp
)
ament_target_dependencies(color_detection
    cv_bridge
    geometry_msgs
    image_geometry
    moveit_ros_planning_interface
    OpenCV
    rclcpp
    tf2_geometry_msgs
)
# ここまででコードに必要なものを記述
add_executable(pick_and_move
  src/pick_and_move.cpp
)
ament_target_dependencies(pick_and_move
    cv_bridge
    geometry_msgs
    image_geometry
    moveit_ros_planning_interface
    OpenCV
    rclcpp
    tf2_geometry_msgs
)
add_executable(pick_and_move_tf
  src/pick_and_move_tf.cpp
)
ament_target_dependencies(pick_and_move_tf
    cv_bridge
    geometry_msgs
    image_geometry
    moveit_ros_planning_interface
    OpenCV
    rclcpp
    tf2_geometry_msgs
)

add_executable(linear_move_server
  src/linear_move_server.cpp
)
ament_target_dependencies(linear_move_server
    angles
    geometry_msgs
    moveit_ros_planning_interface
    rclcpp
    tf2_geometry_msgs
)
target_link_libraries(linear_move_server "${cpp_typesupport_target}")

add_executable(move_tip_straight_demo
  src/move_tip_straight_demo.cpp
)
ament_target_dependencies(move_tip_straight_demo
    rclcpp
)
target_link_libraries(move_tip_straight_demo "${cpp_typesupport_target}")

# src下のソースコードはTARGETSで
# launch下のlaunchファイルはDIRECTORYで
install(TARGETS
  color_detection
  pick_and_move
  pick_and_move_tf
  linear_move_server
  move_tip_straight_demo
  DESTINATION lib/${PROJECT_NAME}/
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_dependencies(rosidl_default_runtime)
ament_package()
