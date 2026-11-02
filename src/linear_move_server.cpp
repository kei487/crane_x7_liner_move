// SPDX-FileCopyrightText: 2026 Keitaro Nakamura
// SPDX-License-Identifier: Apache 2.0
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "angles/angles.h"
#include "crane_x7_liner_move/srv/move_tip_straight.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "moveit/move_group_interface/move_group_interface.h"
#include "rclcpp/rclcpp.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

using MoveGroupInterface = moveit::planning_interface::MoveGroupInterface;
using MoveTipStraight = crane_x7_liner_move::srv::MoveTipStraight;

class LinearMoveServer : public rclcpp::Node
{
public:
  explicit LinearMoveServer(const rclcpp::Node::SharedPtr & move_group_arm_node)
  : Node("linear_move_server"),
    arm_(move_group_arm_node, "arm")
  {
    arm_.setMaxVelocityScalingFactor(0.3);
    arm_.setMaxAccelerationScalingFactor(0.3);

    service_ = create_service<MoveTipStraight>(
      "move_tip_straight",
      std::bind(
        &LinearMoveServer::handleRequest, this,
        std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(get_logger(), "Service ready: /move_tip_straight");
  }

  bool waitForRobotState(int attempts = 10, double wait_sec = 2.0)
  {
    arm_.startStateMonitor(wait_sec);
    for (int i = 0; i < attempts; ++i) {
      if (arm_.getCurrentState(wait_sec)) {
        RCLCPP_INFO(get_logger(), "Current robot state is available.");
        return true;
      }
      RCLCPP_WARN(
        get_logger(),
        "Waiting for robot state (%d/%d). "
        "If using Gazebo, relaunch with use_sim_time:=true",
        i + 1, attempts);
    }
    RCLCPP_ERROR(
      get_logger(),
      "Failed to fetch current robot state. "
      "For Gazebo: use_sim_time:=true");
    return false;
  }

private:
  geometry_msgs::msg::Pose makePose(
    double x, double y, double z,
    double roll_deg, double pitch_deg, double yaw_deg) const
  {
    geometry_msgs::msg::Pose pose;
    tf2::Quaternion q;
    q.setRPY(
      angles::from_degrees(roll_deg),
      angles::from_degrees(pitch_deg),
      angles::from_degrees(yaw_deg));
    pose.orientation = tf2::toMsg(q);
    pose.position.x = x;
    pose.position.y = y;
    pose.position.z = z;
    return pose;
  }

  bool executeCartesian(
    const std::vector<geometry_msgs::msg::Pose> & waypoints,
    double & fraction_out)
  {
    arm_.setStartStateToCurrentState();

    moveit_msgs::msg::RobotTrajectory trajectory;
    const double eef_step = 0.005;
    const double jump_threshold = 0.0;
    fraction_out = arm_.computeCartesianPath(
      waypoints, eef_step, jump_threshold, trajectory);
    RCLCPP_INFO(
      get_logger(), "Cartesian path fraction: %.2f (%zu waypoints)",
      fraction_out, waypoints.size());
    if (fraction_out < 0.95) {
      return false;
    }

    const auto result = arm_.execute(trajectory);
    if (result != moveit::core::MoveItErrorCode::SUCCESS) {
      RCLCPP_ERROR(get_logger(), "Execution failed. code=%d", result.val);
      return false;
    }
    return true;
  }

  bool moveTipStraight(const geometry_msgs::msg::Pose & goal, double & fraction_out)
  {
    if (!arm_.getCurrentState(1.0)) {
      fraction_out = 0.0;
      return false;
    }

    if (executeCartesian({goal}, fraction_out)) {
      return true;
    }

    const geometry_msgs::msg::Pose current = arm_.getCurrentPose().pose;
    geometry_msgs::msg::Pose via = current;
    via.position.z = std::max({current.position.z, goal.position.z, 0.2}) + 0.08;
    via.orientation = goal.orientation;

    RCLCPP_WARN(
      get_logger(),
      "Direct path failed. Retry via (%.3f, %.3f, %.3f).",
      via.position.x, via.position.y, via.position.z);

    return executeCartesian({via, goal}, fraction_out);
  }

  void handleRequest(
    const std::shared_ptr<MoveTipStraight::Request> request,
    std::shared_ptr<MoveTipStraight::Response> response)
  {
    std::lock_guard<std::mutex> lock(motion_mutex_);

    const double roll = request->use_custom_orientation ? request->roll_deg : 90.0;
    const double pitch = request->use_custom_orientation ? request->pitch_deg : 0.0;
    const double yaw = request->use_custom_orientation ? request->yaw_deg : 90.0;
    const auto goal = makePose(
      request->x, request->y, request->z, roll, pitch, yaw);

    RCLCPP_INFO(
      get_logger(),
      "Request: xyz=(%.3f, %.3f, %.3f) rpy_deg=(%.1f, %.1f, %.1f)",
      request->x, request->y, request->z, roll, pitch, yaw);

    double fraction = 0.0;
    const bool ok = moveTipStraight(goal, fraction);
    response->success = ok;
    response->fraction = fraction;
    response->message = ok ?
      "Moved tip along a Cartesian path." :
      "Failed to compute or execute a straight path to the target.";

    RCLCPP_INFO(
      get_logger(), "Result: success=%s fraction=%.2f",
      ok ? "true" : "false", fraction);
  }

  MoveGroupInterface arm_;
  rclcpp::Service<MoveTipStraight>::SharedPtr service_;
  std::mutex motion_mutex_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions node_options;
  node_options.automatically_declare_parameters_from_overrides(true);
  auto move_group_arm_node = rclcpp::Node::make_shared(
    "linear_move_arm_node", node_options);

  rclcpp::executors::SingleThreadedExecutor arm_executor;
  arm_executor.add_node(move_group_arm_node);
  std::thread([&arm_executor]() {arm_executor.spin();}).detach();

  auto server = std::make_shared<LinearMoveServer>(move_group_arm_node);
  if (!server->waitForRobotState()) {
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::spin(server);
  rclcpp::shutdown();
  return 0;
}
