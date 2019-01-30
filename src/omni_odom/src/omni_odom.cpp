//位置情報の管理に超絶ムダが多い,-->navのトピックとtfの理解ができればマシになるかも

#include <ros/ros.h>
#include <iostream>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Pose2D.h>
#include <geometry_msgs/PoseStamped.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Int16MultiArray.h>
#include <tf/transform_broadcaster.h>
#include <tf/tf.h>
#include <math.h>
/////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;

float R = 0.76;//0.760;

float tof_x = 0;
float tof_y = 0;
float range = 0;
double K_range = 0.0000000305;

float cmd_x = 0;
float cmd_y = 0;
float ref_x = 0;
float ref_y = 0;
double ref_th = 0;

double ref_quat_z = 0;
double ref_quat_w = 0;

double goal_x = 0;
double goal_y = 0;
double roll = 0;
double pitch = 0;
double yaw = 0;

float ref_omni_x = 0;
float ref_omni_y = 0;
float ref_omni_th = 0;

float odom_x = 0;
float odom_y = 0;
float odom_th = 0;

float bodyPose_x = 0;
float bodyPose_y = 0;

float err_body_x = 0;
float err_body_y = 0;
float err_body_th = 0;

float body_point_x = 0;
float body_point_y = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////
ros::Publisher to_mover;
geometry_msgs::Twist msg_omni;
ros::Publisher to_body;
std_msgs::Int16MultiArray msg_body;
/////////////////////////////////////////////////////////////////////////////////////////////////
void ImuCallBack (const std_msgs::Float32& ang){
	odom_th = ang.data;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void TofCallBack (const geometry_msgs::Pose2D& tof){
	tof_y = tof.x;
	tof_x = tof.y;
	range = tof.theta;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void CmdCallBack (const geometry_msgs::Twist& twist){
	cmd_x = twist.linear.x;
	cmd_y = twist.linear.y;
	//ref_th += twist.angular.z; //* 10.0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void GoalCallBack(const geometry_msgs::PoseStamped& posestamped){
	ref_quat_z = posestamped.pose.orientation.z;
	ref_quat_w = posestamped.pose.orientation.w;
	
	tf::Quaternion ref_q(0,0,ref_quat_z,ref_quat_w);
	tf::Matrix3x3 m(ref_q);
	m.getRPY(roll,pitch,yaw);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void Timer_Callback(const ros::TimerEvent&){
	to_mover.publish(msg_omni);
	to_body.publish(msg_body);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
int main (int argc, char **argv){

	ros::init(argc, argv, "omni_odom");
	ros::NodeHandle nh;
	
	ros::Subscriber imu = nh.subscribe("angle", 100, ImuCallBack);
	ros::Subscriber tof = nh.subscribe("ToF", 100, TofCallBack);
	ros::Subscriber vel = nh.subscribe("cmd_vel",100,CmdCallBack);
	ros::Subscriber rot = nh.subscribe("move_base_simple/goal", 1,GoalCallBack);
	
	to_mover = nh.advertise<geometry_msgs::Twist>("robo_pointer", 100);
	to_body = nh.advertise<std_msgs::Int16MultiArray>("ref_body", 100);
	
	tf::TransformBroadcaster broadcaster;
	ros::Time current_time;

	ros::Rate loop_rate(2000);//2000
	
	ros::Timer timer = nh.createTimer(ros::Duration(0.02), Timer_Callback);
	
	while(ros::ok()){
	
		current_time = ros::Time::now();
		msg_body.data.resize(3);
		
		
		ref_x += cmd_x / 5000;//(cmd_x * cos(odom_th) - cmd_y * sin(odom_th)) / 5000.0; //* 10.0;
		ref_y += cmd_y / 5000;//(cmd_x * sin(odom_th) + cmd_y * cos(odom_th)) / 5000.0; //* 10.0;
		
		//移動機構の目標値
		ref_omni_x = ref_x + R * (1.0 - cos(ref_th)); //+ R * sin(vel_th);//ref_thから変更
		ref_omni_y = ref_y - R * sin(ref_th);//- R * (1.0 - cos(vel_th));
		
		if(ref_th < yaw){
			ref_th += 0.00010;
		}else if(ref_th > yaw){
			ref_th -= 0.00010;
		}
		
		ref_omni_th = ref_th * 1800 / 3.141926;
			
		//センサデータから自己位置を求める
		odom_x -= 0.8 *(tof_x * sin(odom_th) + tof_y * cos(odom_th)) * K_range * range;
		odom_y += 0.8 *(tof_x * cos(odom_th) - tof_y * sin(odom_th)) * K_range * range;
				
		
		//人形の位置を求める
		bodyPose_x = odom_x + R * cos(odom_th);
		bodyPose_y = odom_y + R * sin(odom_th);
		
		//人形座標の偏差
		err_body_x = ref_x + R - bodyPose_x;
		err_body_y = ref_y - bodyPose_y;
		err_body_th = yaw - odom_th;
		
		//人形の偏差から人形の目標値を決定
		body_point_x = err_body_x * cos(odom_th) - err_body_y * sin(odom_th);
		body_point_y = err_body_x * sin(odom_th) + err_body_y * cos(odom_th);
						
		//移動機構のtf//
		geometry_msgs::TransformStamped t;
		t.header.stamp = current_time;
		t.header.frame_id = "odom";
		t.child_frame_id = "base_link";
		
		t.transform.translation.x = (float)odom_x;
    t.transform.translation.y = (float)odom_y;
    t.transform.translation.z = 0;
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(odom_th);
    t.transform.rotation = odom_quat;
    
    broadcaster.sendTransform(t);
    
    //人形に送る目標値
    msg_body.data[0] = (int) 1000 * body_point_x;
    msg_body.data[1] = (int) 1000 * body_point_y;
    msg_body.data[2] = (int) 1000 * err_body_th;
    
    //移動機構に送る座標・目標値
    msg_omni.linear.x = ref_omni_x * 1000.0; 
    msg_omni.linear.y = ref_omni_y * 1000.0;
    msg_omni.linear.z = ref_omni_th;
    msg_omni.angular.x = odom_x * 1000.0;
    msg_omni.angular.y = odom_y * 1000.0;
    
		ros::spinOnce();
		loop_rate.sleep();
		
	}
	
	return 0;
}
