/****************************************************************
 *
 * Copyright (c) 2010
 *
 * Fraunhofer Institute for Manufacturing Engineering	
 * and Automation (IPA)
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Project name: care-o-bot
 * ROS stack name: cob3_driver
 * ROS package name: cob3_platform
 *								
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *			
 * Author: Florian Weisshardt, email:florian.weisshardt@ipa.fhg.de
 * Supervised by: Christian Connette, email:christian.connette@ipa.fhg.de
 *
 * Date of creation: Jan 2010
 * ToDo:
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Fraunhofer Institute for Manufacturing 
 *       Engineering and Automation (IPA) nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License LGPL as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License LGPL for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License LGPL along with this program. 
 * If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************/

//##################
//#### includes ####

// standard includes
//--

// ROS includes
#include <ros/ros.h>

// ROS message includes
#include <geometry_msgs/Twist.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>

// ROS service includes
#include <cob3_srvs/Init.h>
#include <cob3_srvs/Stop.h>
#include <cob3_srvs/Shutdown.h>

// external includes
#include <cob3_platform/PlatformHardware.h>

//####################
//#### node class ####
class NodeClass
{
    //
    public:
	    // create a handle for this node, initialize node
	    ros::NodeHandle n;
                
        // topics to publish
        ros::Publisher topicPub_Pose2D;
        ros::Publisher topicPub_Odometry;
//        tf::TransformBroadcaster odom_broadcaster;

	    // topics to subscribe, callback is called for new messages arriving
        ros::Subscriber topicSub_CmdVel;
        
        // service servers
        ros::ServiceServer srvServer_Init;
        ros::ServiceServer srvServer_Home;
        ros::ServiceServer srvServer_Stop;
        ros::ServiceServer srvServer_Shutdown;
            
        // service clients
        //--
        
        // global variables
        PlatformHardware* pltf;
        bool isInitialized;
        double cmdVelX, cmdVelY, cmdVelTh;
        double x, y, th;
        double dxMM, dyMM, dth, dvth;
        double vxMMS, vyMMS, vth, vvth;
        ros::Time current_time;

        // Constructor
        NodeClass()
        {
            // initialize global variables
            isInitialized = false;
            current_time = ros::Time::now();
            cmdVelX = cmdVelY = cmdVelTh = 0;
            x = y = th = 0;
            dxMM = dyMM = dth = dvth = 0;
            vxMMS = vyMMS = vth = vvth = 0;

        	// implementation of topics to publish
            topicPub_Odometry = n.advertise<nav_msgs::Odometry>("odometry", 50);

            // implementation of topics to subscribe
            topicSub_CmdVel = n.subscribe("cmd_vel", 1, &NodeClass::topicCallback_CmdVel, this);
            
            // implementation of service servers
            srvServer_Init = n.advertiseService("Init", &NodeClass::srvCallback_Init, this);
            srvServer_Stop = n.advertiseService("Stop", &NodeClass::srvCallback_Stop, this);
            srvServer_Shutdown = n.advertiseService("Shutdown", &NodeClass::srvCallback_Shutdown, this);
        }
        
        // Destructor
        ~NodeClass() 
        {
            pltf->setVelPltf(0, 0, 0, 0);
            pltf->shutdownPltf();
            isInitialized = false;
            delete pltf;
        }

        // topic callback functions 
        // function will be called when a new message arrives on a topic
        void topicCallback_CmdVel(const geometry_msgs::Twist::ConstPtr& msg)
        {
            ROS_DEBUG("received new velocity command [cmdVelX=%3.5f,cmdVelY=%3.5f,cmdVelTh=%3.5f]", 
                     msg->linear.x, msg->linear.y, msg->angular.z);

            cmdVelX = msg->linear.x;
            cmdVelY = msg->linear.y;
            cmdVelTh = msg->angular.z;
        }

        // service callback functions
        // function will be called when a service is querried
        bool srvCallback_Init(cob3_srvs::Init::Request &req,
                              cob3_srvs::Init::Response &res )
        {
            if(isInitialized == false)
            {
                ROS_INFO("...initializing platform...");
                pltf = new PlatformHardware();
                pltf->initPltf();
                isInitialized = true;
                res.success = 0; // 0 = true, else = false
            }
            else
            {
                ROS_ERROR("...platform already initialized...");
                res.success = 1;
                res.errorMessage.data = "platform already initialized";
            }            
            return true;
        }
        
        bool srvCallback_Stop(cob3_srvs::Stop::Request &req,
                              cob3_srvs::Stop::Response &res )
        {
            if(isInitialized == true)
            {
                ROS_INFO("...stopping platform...");
                cmdVelX = 0;
                cmdVelY = 0;
                cmdVelTh = 0;
                res.success = 0; // 0 = true, else = false
            }
            else
            {
                ROS_ERROR("...platform not initialized...");
                res.success = 1;
                res.errorMessage.data = "platform not initialized";
            }
            return true;
        }

        bool srvCallback_Shutdown(cob3_srvs::Shutdown::Request &req,
                                  cob3_srvs::Shutdown::Response &res )
        {
            if(isInitialized == true)
            {
                ROS_INFO("...shutting down platform...");
                pltf->shutdownPltf();
                isInitialized = false;
                res.success = 0; // 0 = true, else = false
            }            
            else
            {
                ROS_ERROR("...platform not initialized...");
                res.success = 1;
                res.errorMessage.data = "platform not initialized";
            }
            return true;
        }
        
        // other function declarations
        void updateCmdVel()
        {
            // send vel if platform is initialized
            if(isInitialized == true)
            {
                ROS_DEBUG("update cmdVel");
                pltf->setVelPltf(cmdVelX*1000, cmdVelY*1000, cmdVelTh, 0);  // convert from m/s to mm/s
            }
        }

        void updateOdometry()
        {
            if(isInitialized == true)
            {
                // get odometry from platform
                pltf->getDeltaPosePltf(dxMM, dyMM, dth, dvth,
        					           vxMMS, vyMMS, vth, vvth);

                // add delta values to old values
                x += dxMM / 1000.0; // convert from mm to m
                y += dyMM / 1000.0; // convert from mm to m
                th += dth * 2; // Hack!! TODO error in odometry calculation, change in library 

                //next, we'll publish the odometry message over ROS
                nav_msgs::Odometry odom;
                odom.header.stamp = current_time;
                odom.header.frame_id = "odom";
                odom.child_frame_id = "base_link";

                //set the position
                odom.pose.pose.position.x = x;
                odom.pose.pose.position.y = y;
                odom.pose.pose.position.z = 0.0;
                odom.pose.pose.orientation = tf::createQuaternionMsgFromYaw(th);

                //set the velocity
                odom.twist.twist.linear.x = vxMMS / 1000.0; // convert from mm/s to m/s
                odom.twist.twist.linear.y = vyMMS / 1000.0; // convert from mm/s to m/s
                odom.twist.twist.angular.z = vth;

                //publish the message
                topicPub_Odometry.publish(odom);

                ROS_DEBUG("published Pose2D: pos[x=%3.2f,y=%3.2f,th=%3.2f]",x, y, th);
                ROS_DEBUG("published Pose2D: vel[vy=%3.2f,vx=%3.2f,vth=%3.2f]",vxMMS, vyMMS, vth);
            }
        }
};

//#######################
//#### main programm ####
int main(int argc, char** argv)
{
    // initialize ROS, spezify name of node
    ros::init(argc, argv, "cob3_driver_platform");
    
    NodeClass nodeClass;

    // main loop
 	ros::Rate loop_rate(10); // Hz 
    while(nodeClass.n.ok())
    {
        nodeClass.current_time = ros::Time::now();

        // publish Odometry
        nodeClass.updateOdometry();

        // update velocity of platform
        nodeClass.updateCmdVel();

        ros::spinOnce();
        loop_rate.sleep();
    }
    
//    ros::spin();

    return 0;
}
