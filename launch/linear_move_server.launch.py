# SPDX-FileCopyrightText: 2026 Keitaro Nakamura
# SPDX-License-Identifier: Apache 2.0
import os

from ament_index_python.packages import get_package_share_directory
from crane_x7_description.robot_description_loader import RobotDescriptionLoader
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.actions import SetParameter
import yaml


def load_file(package_name, file_path):
    package_path = get_package_share_directory(package_name)
    absolute_file_path = os.path.join(package_path, file_path)
    try:
        with open(absolute_file_path, 'r') as file:
            return file.read()
    except EnvironmentError:
        return None


def load_yaml(package_name, file_path):
    package_path = get_package_share_directory(package_name)
    absolute_file_path = os.path.join(package_path, file_path)
    try:
        with open(absolute_file_path, 'r') as file:
            return yaml.safe_load(file)
    except EnvironmentError:
        return None


def generate_launch_description():
    description_loader = RobotDescriptionLoader()

    robot_description_semantic = {
        'robot_description_semantic': load_file(
            'crane_x7_moveit_config', 'config/crane_x7.srdf')
    }

    kinematics_yaml = load_yaml('crane_x7_moveit_config', 'config/kinematics.yaml')
    if kinematics_yaml and 'arm' in kinematics_yaml:
        kinematics_yaml['arm']['kinematics_solver_timeout'] = 0.1

    declare_use_sim_time = DeclareLaunchArgument(
        'use_sim_time', default_value='false',
        description='Set true when using the gazebo simulator.')

    server_node = Node(
        package='crane_x7_liner_move',
        executable='linear_move_server',
        output='screen',
        parameters=[
            {'use_sim_time': LaunchConfiguration('use_sim_time')},
            {'robot_description': description_loader.load()},
            robot_description_semantic,
            kinematics_yaml,
        ])

    return LaunchDescription([
        declare_use_sim_time,
        SetParameter(name='use_sim_time', value=LaunchConfiguration('use_sim_time')),
        server_node,
    ])
