#include <ros/ros.h>
#include <iostream>
#include <geometry_msgs/Pose2D.h>
#include <tf/transform_broadcaster.h>
#include <math.h>
///////////////////////////////
using namespace std;

float pose_x = 0;
float pose_y = 0;
float pose_z = 0;

float R = 0.76;//0.76;

float angle = 0;
///////////////////////////////
void PoseCallBack (const geometry_msgs::Pose2D& pose){
	pose_x = pose.x;
	pose_y = pose.y;
	pose_z = pose.theta;
}
///////////////////////////////
int main (int argc, char **argv){
	
	ros::init(argc, argv, "body_odom");
	ros::NodeHandle nh;
	
	ros::Subscriber pose = nh.subscribe("pose", 100, PoseCallBack);
	
	tf::TransformBroadcaster body_broadcaster;
	ros::Time current_time;
	
	ros::Rate loop_rate(1000);
	
	while(ros::ok()){
	
		current_time = ros::Time::now();
				
		geometry_msgs::TransformStamped t;
		t.header.stamp = current_time;
		t.header.frame_id = "base_link";
		t.child_frame_id = "body_link";
		
		t.transform.translation.x = (float) R + pose_x;
    t.transform.translation.y = (float)pose_y;
    t.transform.translation.z = 0;
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(angle);
    t.transform.rotation = odom_quat;
    
    body_broadcaster.sendTransform(t);
    
    ros::spinOnce();
    loop_rate.sleep();
	
	}
	
	return 0;
}
