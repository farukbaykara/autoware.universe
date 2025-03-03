// Copyright 2023 Tier IV, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ament_index_cpp/get_package_share_directory.hpp"
#include "behavior_velocity_planner/node.hpp"
#include "planning_interface_test_manager/planning_interface_test_manager.hpp"
#include "planning_interface_test_manager/planning_interface_test_manager_utils.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

using behavior_velocity_planner::BehaviorVelocityPlannerNode;
using planning_test_utils::PlanningInterfaceTestManager;

std::shared_ptr<PlanningInterfaceTestManager> generateTestManager()
{
  auto test_manager = std::make_shared<PlanningInterfaceTestManager>();

  // set subscriber with topic name: behavior_velocity_planner → test_node_
  test_manager->setPathSubscriber("behavior_velocity_planner_node/output/path");

  // set behavior_velocity_planner node's input topic name(this topic is changed to test node)
  test_manager->setPathWithLaneIdTopicName(
    "behavior_velocity_planner_node/input/path_with_lane_id");

  test_manager->setInitialPoseTopicName("behavior_velocity_planner_node/input/vehicle_odometry");
  test_manager->setOdometryTopicName("behavior_velocity_planner_node/input/vehicle_odometry");

  return test_manager;
}

std::shared_ptr<BehaviorVelocityPlannerNode> generateNode()
{
  auto node_options = rclcpp::NodeOptions{};

  const auto planning_test_utils_dir =
    ament_index_cpp::get_package_share_directory("planning_test_utils");
  const auto behavior_velocity_planner_dir =
    ament_index_cpp::get_package_share_directory("behavior_velocity_planner");
  const auto motion_velocity_smoother_dir =
    ament_index_cpp::get_package_share_directory("motion_velocity_smoother");

  test_utils::updateNodeOptions(
    node_options,
    {planning_test_utils_dir + "/config/test_common.param.yaml",
     planning_test_utils_dir + "/config/test_nearest_search.param.yaml",
     planning_test_utils_dir + "/config/test_vehicle_info.param.yaml",
     motion_velocity_smoother_dir + "/config/default_motion_velocity_smoother.param.yaml",
     motion_velocity_smoother_dir + "/config/Analytical.param.yaml",
     behavior_velocity_planner_dir + "/config/behavior_velocity_planner.param.yaml",
     behavior_velocity_planner_dir + "/config/blind_spot.param.yaml",
     behavior_velocity_planner_dir + "/config/crosswalk.param.yaml",
     behavior_velocity_planner_dir + "/config/detection_area.param.yaml",
     behavior_velocity_planner_dir + "/config/intersection.param.yaml",
     behavior_velocity_planner_dir + "/config/no_stopping_area.param.yaml",
     behavior_velocity_planner_dir + "/config/occlusion_spot.param.yaml",
     behavior_velocity_planner_dir + "/config/run_out.param.yaml",
     behavior_velocity_planner_dir + "/config/speed_bump.param.yaml",
     behavior_velocity_planner_dir + "/config/stop_line.param.yaml",
     behavior_velocity_planner_dir + "/config/traffic_light.param.yaml",
     behavior_velocity_planner_dir + "/config/virtual_traffic_light.param.yaml",
     behavior_velocity_planner_dir + "/config/out_of_lane.param.yaml"});

  node_options.append_parameter_override("launch_stop_line", true);
  node_options.append_parameter_override("launch_crosswalk", true);
  node_options.append_parameter_override("launch_traffic_light", true);
  node_options.append_parameter_override("launch_intersection", true);
  node_options.append_parameter_override("launch_blind_spot", true);
  node_options.append_parameter_override("launch_detection_area", true);
  node_options.append_parameter_override(
    "launch_virtual_traffic_light", false);  // TODO(Kyoichi Sugahara) set to true
  node_options.append_parameter_override(
    "launch_occlusion_spot", false);  // TODO(Kyoichi Sugahara) set to true
  node_options.append_parameter_override("launch_no_stopping_area", true);
  node_options.append_parameter_override(
    "launch_run_out", false);  // TODO(Kyoichi Sugahara) set to true
  node_options.append_parameter_override(
    "launch_speed_bump", false);  // TODO(Kyoichi Sugahara) set to true
  node_options.append_parameter_override("launch_out_of_lane", true);

  return std::make_shared<BehaviorVelocityPlannerNode>(node_options);
}

void publishMandatoryTopics(
  std::shared_ptr<PlanningInterfaceTestManager> test_manager,
  std::shared_ptr<BehaviorVelocityPlannerNode> test_target_node)
{
  // publish necessary topics from test_manager
  test_manager->publishTF(test_target_node, "/tf");
  test_manager->publishAcceleration(test_target_node, "behavior_velocity_planner_node/input/accel");
  test_manager->publishPredictedObjects(
    test_target_node, "behavior_velocity_planner_node/input/dynamic_objects");
  test_manager->publishPointCloud(
    test_target_node, "behavior_velocity_planner_node/input/no_ground_pointcloud");
  test_manager->publishOdometry(
    test_target_node, "behavior_velocity_planner_node/input/vehicle_odometry");
  test_manager->publishAcceleration(test_target_node, "behavior_velocity_planner_node/input/accel");
  test_manager->publishMap(test_target_node, "behavior_velocity_planner_node/input/vector_map");
  test_manager->publishTrafficSignals(
    test_target_node, "behavior_velocity_planner_node/input/traffic_signals");
  test_manager->publishMaxVelocity(
    test_target_node, "behavior_velocity_planner_node/input/external_velocity_limit_mps");
  test_manager->publishVirtualTrafficLightState(
    test_target_node, "behavior_velocity_planner_node/input/virtual_traffic_light_states");
  test_manager->publishOccupancyGrid(
    test_target_node, "behavior_velocity_planner_node/input/occupancy_grid");
}

TEST(PlanningModuleInterfaceTest, NodeTestWithExceptionPathWithLaneID)
{
  rclcpp::init(0, nullptr);
  auto test_manager = generateTestManager();
  auto test_target_node = generateNode();

  publishMandatoryTopics(test_manager, test_target_node);

  // test with nominal path_with_lane_id
  ASSERT_NO_THROW_WITH_ERROR_MSG(test_manager->testWithNominalPathWithLaneId(test_target_node));
  EXPECT_GE(test_manager->getReceivedTopicNum(), 1);

  // test with empty path_with_lane_id
  ASSERT_NO_THROW_WITH_ERROR_MSG(test_manager->testWithAbnormalPathWithLaneId(test_target_node));
  rclcpp::shutdown();
}

TEST(PlanningModuleInterfaceTest, NodeTestWithOffTrackEgoPose)
{
  rclcpp::init(0, nullptr);

  auto test_manager = generateTestManager();
  auto test_target_node = generateNode();
  publishMandatoryTopics(test_manager, test_target_node);

  // test for normal trajectory
  ASSERT_NO_THROW_WITH_ERROR_MSG(test_manager->testWithNominalPathWithLaneId(test_target_node));

  // make sure behavior_path_planner is running
  EXPECT_GE(test_manager->getReceivedTopicNum(), 1);

  ASSERT_NO_THROW_WITH_ERROR_MSG(test_manager->testOffTrackFromPathWithLaneId(test_target_node));

  rclcpp::shutdown();
}
