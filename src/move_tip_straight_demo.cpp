// SPDX-FileCopyrightText: 2026 Keitaro Nakamura
// SPDX-License-Identifier: Apache 2.0
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "crane_x7_liner_move/srv/move_tip_straight.hpp"
#include "rclcpp/rclcpp.hpp"

using MoveTipStraight = crane_x7_liner_move::srv::MoveTipStraight;
using namespace std::chrono_literals;

struct Target
{
  double x;
  double y;
  double z;
  std::string label;
};

bool callMoveTipStraight(
  const rclcpp::Node::SharedPtr & node,
  const rclcpp::Client<MoveTipStraight>::SharedPtr & client,
  const Target & target)
{
  auto request = std::make_shared<MoveTipStraight::Request>();
  request->x = target.x;
  request->y = target.y;
  request->z = target.z;
  request->use_custom_orientation = false;

  RCLCPP_INFO(
    node->get_logger(), "Call [%s]: xyz=(%.3f, %.3f, %.3f)",
    target.label.c_str(), target.x, target.y, target.z);

  auto future = client->async_send_request(request);
  if (rclcpp::spin_until_future_complete(node, future, 60s) !=
    rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_ERROR(node->get_logger(), "Service call timed out: %s", target.label.c_str());
    return false;
  }

  const auto response = future.get();
  if (response->success) {
    RCLCPP_INFO(
      node->get_logger(), "Success [%s] fraction=%.2f (%s)",
      target.label.c_str(), response->fraction, response->message.c_str());
    return true;
  }

  RCLCPP_ERROR(
    node->get_logger(), "Failed [%s] fraction=%.2f (%s)",
    target.label.c_str(), response->fraction, response->message.c_str());
  return false;
}

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("move_tip_straight_demo");
  auto client = node->create_client<MoveTipStraight>("move_tip_straight");

  RCLCPP_INFO(node->get_logger(), "Waiting for /move_tip_straight ...");
  while (rclcpp::ok() && !client->wait_for_service(1s)) {
    RCLCPP_INFO(node->get_logger(), "still waiting...");
  }
  if (!rclcpp::ok()) {
    rclcpp::shutdown();
    return 1;
  }

  // 引数 x y z があれば単発呼び出し、なければデモ軌跡
  std::vector<Target> targets;
  if (argc >= 4) {
    targets.push_back(
      {std::stod(argv[1]), std::stod(argv[2]), std::stod(argv[3]), "cli_target"});
  } else {
    targets = {
      {0.25, -0.15, 0.10, "approach"},
      {0.30, -0.15, 0.10, "grasp"},
      {0.30, 0.00, 0.25, "lift"},
      {0.30, 0.15, 0.10, "place"},
    };
    RCLCPP_INFO(
      node->get_logger(),
      "No xyz args. Running built-in demo path. "
      "Usage: ros2 run crane_x7_liner_move move_tip_straight_demo -- x y z");
  }

  bool all_ok = true;
  for (const auto & target : targets) {
    if (!callMoveTipStraight(node, client, target)) {
      all_ok = false;
      break;
    }
  }

  rclcpp::shutdown();
  return all_ok ? 0 : 1;
}
