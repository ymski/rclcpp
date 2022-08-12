// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#include <functional>
#include "rclcpp/node_interfaces/node_service_introspection.hpp"
#include "rcl/service_introspection.h"
#include "rcl/client.h"
#include "rcl/service.h"

using rclcpp::node_interfaces::NodeServiceIntrospection;


NodeServiceIntrospection::NodeServiceIntrospection(
  rclcpp::node_interfaces::NodeBaseInterface::SharedPtr & node_base,
  const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr & node_parameters)
: node_base_(node_base)
{
  // declare service introspection parameters
  if (!node_parameters->has_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_SERVICE_PARAMETER)) {
    node_parameters->declare_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_SERVICE_PARAMETER,
        rclcpp::ParameterValue(true));
  }
  if (!node_parameters->has_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_SERVICE_EVENT_CONTENT_PARAMETER)) {
    node_parameters->declare_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_SERVICE_EVENT_CONTENT_PARAMETER,
        rclcpp::ParameterValue(true));
  }
  if (!node_parameters->has_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_CLIENT_PARAMETER)) {
    node_parameters->declare_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_CLIENT_PARAMETER,
        rclcpp::ParameterValue(true));
  }
  if (!node_parameters->has_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_CLIENT_EVENT_CONTENT_PARAMETER)) {
    node_parameters->declare_parameter(RCL_SERVICE_INTROSPECTION_PUBLISH_CLIENT_EVENT_CONTENT_PARAMETER,
        rclcpp::ParameterValue(true));
  }

  std::function<void(const std::vector<rclcpp::Parameter> &)>
    configure_service_introspection_callback = 
    [this](const std::vector<rclcpp::Parameter> & parameters) {
      rcl_ret_t ret;
      for (const auto & param: parameters) {
        if (param.get_name() == RCL_SERVICE_INTROSPECTION_PUBLISH_SERVICE_PARAMETER) {
          for (auto srv = services_.begin(); srv != services_.end(); ++srv) {
            if (srv->expired()) {
              srv = services_.erase(srv);
            } else {
              ret = rcl_service_introspection_configure_server_service_events(
                  srv->lock()->get_service_handle().get(),
                  this->node_base_->get_rcl_node_handle(),
                  param.get_value<bool>());
              if (RCL_RET_OK != ret) {
                throw std::runtime_error("Could not configure service introspection events");
              }
            }
          }
        } else if (param.get_name() == RCL_SERVICE_INTROSPECTION_PUBLISH_CLIENT_PARAMETER) {
          for (auto clt = clients_.begin(); clt != clients_.end(); ++clt) {
            if (clt->expired()){
              clt = clients_.erase(clt);
            } else {
              ret = rcl_service_introspection_configure_client_service_events(
                  clt->lock()->get_client_handle().get(),
                  this->node_base_->get_rcl_node_handle(),
                  param.get_value<bool>());
              if (RCL_RET_OK != ret) {
                throw std::runtime_error("Could not configure service introspection events");
              }
            }
          }
        } else if (param.get_name() == RCL_SERVICE_INTROSPECTION_PUBLISH_SERVICE_EVENT_CONTENT_PARAMETER) {
          for (auto srv = services_.begin(); srv != services_.end(); ++srv) {
            if (srv->expired()){
              srv = services_.erase(srv);
            } else {
              rcl_service_introspection_configure_server_service_event_message_payload(
                  srv->lock()->get_service_handle().get(), param.get_value<bool>());
            }
          }
        } else if (param.get_name() == RCL_SERVICE_INTROSPECTION_PUBLISH_CLIENT_EVENT_CONTENT_PARAMETER) {
          for (auto clt = clients_.begin(); clt != clients_.end(); ++clt) {
            if (clt->expired()){
              clt = clients_.erase(clt);
            } else {
              rcl_service_introspection_configure_client_service_event_message_payload(
                  clt->lock()->get_client_handle().get(), param.get_value<bool>());
            }
          }
        }
      }
    };
  // register callbacks
  post_set_parameters_callback_handle_ = node_parameters->add_post_set_parameters_callback(configure_service_introspection_callback);
}

void
NodeServiceIntrospection::register_client(rclcpp::ClientBase::SharedPtr client)
{
  std::weak_ptr<rclcpp::ClientBase> weak_client = client;
  clients_.push_back(weak_client);
}

void
NodeServiceIntrospection::register_service(rclcpp::ServiceBase::SharedPtr service)
{
  std::weak_ptr<rclcpp::ServiceBase> weak_service = service;
  services_.push_back(weak_service);
}
