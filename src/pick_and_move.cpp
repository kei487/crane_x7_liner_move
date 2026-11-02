// SPDX-FileCopyrightText: 2026 Keitaro Nakamura
// SPDX-License-Identifier: Apache 2.0
#include <cmath>
#include <thread>
#include <vector>

#include "angles/angles.h"
#include "moveit/move_group_interface/move_group_interface.h"
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/quaternion.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

using MoveGroupInterface = moveit::planning_interface::MoveGroupInterface;

static const rclcpp::Logger LOGGER = rclcpp::get_logger("pick_and_move");
static const char * const GRIPPER_JOINT = "crane_x7_gripper_finger_a_joint";

geometry_msgs::msg::Pose makeGraspPose(double x, double y, double z)
{
  geometry_msgs::msg::Pose pose;
  tf2::Quaternion q;
  q.setRPY(angles::from_degrees(90), angles::from_degrees(0), angles::from_degrees(90));
  pose.orientation = tf2::toMsg(q);
  pose.position.x = x;
  pose.position.y = y;
  pose.position.z = z;
  return pose;
}

// Gazebo では use_sim_time:=true が無いと joint_states の時刻が古く見え、
// 現在姿勢取得に失敗してクラッシュすることがある。
bool waitForRobotState(MoveGroupInterface & arm, int attempts = 10, double wait_sec = 2.0)
{
  arm.startStateMonitor(wait_sec);
  for (int i = 0; i < attempts; ++i) {
    if (arm.getCurrentState(wait_sec)) {
      RCLCPP_INFO(LOGGER, "Current robot state is available.");
      return true;
    }
    RCLCPP_WARN(
      LOGGER,
      "Waiting for robot state (%d/%d). "
      "If using Gazebo, relaunch with use_sim_time:=true",
      i + 1, attempts);
  }
  RCLCPP_ERROR(
    LOGGER,
    "Failed to fetch current robot state. "
    "For Gazebo: ros2 launch crane_x7_liner_move pick_and_move.launch.py use_sim_time:=true");
  return false;
}

bool executeCartesian(
  MoveGroupInterface & arm,
  const std::vector<geometry_msgs::msg::Pose> & waypoints)
{
  arm.setStartStateToCurrentState();

  moveit_msgs::msg::RobotTrajectory trajectory;
  const double eef_step = 0.005;
  const double jump_threshold = 0.0;
  const double fraction = arm.computeCartesianPath(
    waypoints, eef_step, jump_threshold, trajectory);
  RCLCPP_INFO(LOGGER, "Cartesian path fraction: %.2f (%zu waypoints)", fraction, waypoints.size());
  if (fraction < 0.95) {
    return false;
  }

  const moveit::core::MoveItErrorCode result = arm.execute(trajectory);
  if (result != moveit::core::MoveItErrorCode::SUCCESS) {
    RCLCPP_ERROR(LOGGER, "Straight path execution failed. code=%d", result.val);
    return false;
  }
  return true;
}

// 現在の手先姿勢から goal までを直線補間する。
// 直線が途中で切れる場合は、いったん高い経由点を挟んで再試行する。
bool moveTipStraight(MoveGroupInterface & arm, const geometry_msgs::msg::Pose & goal)
{
  if (!arm.getCurrentState(1.0)) {
    RCLCPP_ERROR(LOGGER, "Cannot plan straight path without current state.");
    return false;
  }

  if (executeCartesian(arm, {goal})) {
    return true;
  }

  // 失敗パターン: テーブル・自己干渉・IK不連続などで直線が途中までしか解けない
  const geometry_msgs::msg::Pose current = arm.getCurrentPose().pose;
  geometry_msgs::msg::Pose via = current;
  via.position.z = std::max({current.position.z, goal.position.z, 0.2}) + 0.08;
  via.orientation = goal.orientation;

  RCLCPP_WARN(
    LOGGER,
    "Direct straight path failed. Retry via (%.3f, %.3f, %.3f).",
    via.position.x, via.position.y, via.position.z);

  if (executeCartesian(arm, {via, goal})) {
    return true;
  }

  RCLCPP_ERROR(
    LOGGER,
    "Straight path is incomplete even with a via pose. "
    "Target may be unreachable or colliding.");
  return false;
}

// 物体を掴むと目標角度まで閉じられないことがある。失敗しても把持動作は続行する。
bool moveGripper(
  MoveGroupInterface & gripper, double angle, bool required = true)
{
  // getCurrentJointValues() は状態未取得時に空になり segfault の原因になるため使わない
  if (!gripper.setJointValueTarget(GRIPPER_JOINT, angle)) {
    RCLCPP_ERROR(LOGGER, "Invalid gripper joint target.");
    return false;
  }

  const moveit::core::MoveItErrorCode result = gripper.move();
  if (result != moveit::core::MoveItErrorCode::SUCCESS) {
    if (required) {
      RCLCPP_ERROR(LOGGER, "Gripper move failed. code=%d", result.val);
      return false;
    }
    RCLCPP_WARN(
      LOGGER,
      "Gripper did not reach target (code=%d). Continue assuming object contact.",
      result.val);
  }
  return true;
}

bool moveNamed(MoveGroupInterface & arm, const std::string & name)
{
  arm.setNamedTarget(name);
  const moveit::core::MoveItErrorCode result = arm.move();
  if (result != moveit::core::MoveItErrorCode::SUCCESS) {
    RCLCPP_ERROR(LOGGER, "Named move '%s' failed. code=%d", name.c_str(), result.val);
    return false;
  }
  return true;
}

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions node_options;
  node_options.automatically_declare_parameters_from_overrides(true);
  auto move_group_arm_node = rclcpp::Node::make_shared("move_group_arm_node", node_options);
  auto move_group_gripper_node = rclcpp::Node::make_shared("move_group_gripper_node", node_options);
  // For current state monitor
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(move_group_arm_node);
  executor.add_node(move_group_gripper_node);
  std::thread([&executor]() {executor.spin();}).detach();

  // 直線軌道は実機コントローラが追従しやすい速度にする
  MoveGroupInterface move_group_arm(move_group_arm_node, "arm");
  move_group_arm.setMaxVelocityScalingFactor(0.3);  // Set 0.0 ~ 1.0
  move_group_arm.setMaxAccelerationScalingFactor(0.3);  // Set 0.0 ~ 1.0

  MoveGroupInterface move_group_gripper(move_group_gripper_node, "gripper");
  move_group_gripper.setMaxVelocityScalingFactor(1.0);  // Set 0.0 ~ 1.0
  move_group_gripper.setMaxAccelerationScalingFactor(1.0);  // Set 0.0 ~ 1.0
  // 把持時は指が目標角度に届かないことが多いので許容を広げる
  move_group_gripper.setGoalJointTolerance(angles::from_degrees(10.0));

  if (!waitForRobotState(move_group_arm)) {
    rclcpp::shutdown();
    return 1;
  }

  auto finish = [&]() {
    move_group_arm.clearPathConstraints();
    moveNamed(move_group_arm, "home");
    rclcpp::shutdown();
  };

  // SRDFに定義されている"home"の姿勢にする
  if (!moveNamed(move_group_arm, "home")) {
    finish();
    return 1;
  }

  // グリッパーを開く
  if (!moveGripper(move_group_gripper, angles::from_degrees(60), true)) {
    finish();
    return 1;
  }

  // 関節制約は手先の直線補間と両立しにくいので使わない。
  // 直線性は computeCartesianPath 側で保証する。
  // ※実機で成功している目標座標はそのまま維持する
  const geometry_msgs::msg::Pose approach_pose = makeGraspPose(0.2, -0.15, 0.05);
  const geometry_msgs::msg::Pose grasp_pose = makeGraspPose(0.3, -0.15, 0.05);
  const geometry_msgs::msg::Pose lift_pose = makeGraspPose(0.3, 0.0, 0.3);
  const geometry_msgs::msg::Pose place_pose = makeGraspPose(0.3, 0.15, 0.1);

  if (!moveTipStraight(move_group_arm, approach_pose) ||
    !moveTipStraight(move_group_arm, grasp_pose))
  {
    finish();
    return 1;
  }

  // 物体に当たって指が目標角度まで閉じない場合がある（実機でも発生）
  moveGripper(move_group_gripper, angles::from_degrees(15), false);

  if (!moveTipStraight(move_group_arm, lift_pose) ||
    !moveTipStraight(move_group_arm, place_pose))
  {
    finish();
    return 1;
  }

  moveGripper(move_group_gripper, angles::from_degrees(60), false);

  finish();
  return 0;
}
