#include "ros/ros.h"
#include <stdio.h>
#include <geometry_msgs/Twist.h>
#include <vector>
#include <string>
#include <fstream>

std::vector<float> cmd_x, cmd_w, res_x, res_w,timestamp;
float last_cmd_x = -1.23, last_cmd_w = -1.23, last_res_x = -1.23, last_res_w = -1.23;

void resVelLogger(const geometry_msgs::Twist::ConstPtr &msg)
{
    last_res_x = msg->linear.x;
    last_res_w = msg->angular.z;
}

void cmdVelLogger(const geometry_msgs::Twist::ConstPtr &msg)
{
    last_cmd_x = msg->linear.x;
    last_cmd_w = msg->angular.z;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "velocity_tester_node");
    ros::NodeHandle nh;
    ros::Publisher cmd_vel_pub = nh.advertise<geometry_msgs::Twist>("/cmd_vel", 1, true);
    ros::Subscriber res_vel_sub = nh.subscribe("/velocities", 1, &resVelLogger);
    ros::Subscriber cmd_vel_sub = nh.subscribe("/cmd_vel", 1, &cmdVelLogger);

    float input_x = std::stof(argv[1]);
    float input_w = std::stof(argv[2]);

    ROS_INFO("Starting test with input x of %f and input w of %f", input_x, input_w);

    ros::Rate r(10);
    ros::Time start = ros::Time::now();
    int pnts_logged = 0;

    //Publish velocity
    geometry_msgs::Twist newTwist;
    newTwist.linear.x = input_x;
    newTwist.angular.z = input_w;
    cmd_vel_pub.publish(newTwist);

    //Test will run for 5s or until shutdown
    while (ros::ok() && ros::Time::now() - start < ros::Duration(5))
    {
        ROS_INFO("Points logged: %d", ++pnts_logged);
        ros::spinOnce();

        //Ensure both sets of data have at least first data point
        if (last_cmd_x != -1.23 && last_res_x != -1.23)
        {
            cmd_x.push_back(last_cmd_x);
            cmd_w.push_back(last_cmd_w);
            res_x.push_back(last_res_x);
            res_w.push_back(last_res_w);
        }
        timestamp.push_back((ros::Time::now() - start).toSec());

        r.sleep();
    }

    //Stop motors
    newTwist.linear.x = 0.0;
    newTwist.angular.z = 0.0;
    cmd_vel_pub.publish(newTwist);

    //Write data to file on shutdown
    ROS_INFO("Writing results to file");
    std::string filename = "x_";
    filename += std::string(argv[1]) + "_w_" + std::string(argv[2]) + ".txt";
    std::ofstream results;
    results.open(filename);

    for (int i = 0; i < cmd_x.size(); i++)
    {
        results << timestamp[i] << "\t" << cmd_x[i] << "\t" << res_x[i] << "\t" << cmd_w[i] << "\t" << res_w[i] << "\n";
    }
    results.close();

    return 0;
}