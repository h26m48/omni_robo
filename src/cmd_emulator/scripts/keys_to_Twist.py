#!/usr/bin/env python
import rospy
from std_msgs.msg import String
from std_msgs.msg import UInt16
from geometry_msgs.msg import Twist

key_mapping = { 'o':0, 'w': 1, 's': 2, 'a': 3, 'd':4, 'q':5, 'e':6, 'r':7, 'f':8 }
twist = Twist()

def keys_cb(msg, twist_pub ):

	if len(msg.data) == 0 or not key_mapping.has_key(msg.data):
		return # unknown key.
	vels = key_mapping[msg.data]

	t = UInt16()
	t = vels
	
	#twist.linear.x = 0
	#twist.linear.y = 0
	#twist.angular.z = 0
	
	if t == 1:
		twist.linear.x = 0.1
	elif t == 2:
		twist.linear.x = -0.1
	elif t == 3:
		twist.linear.y = 0.1
	elif t == 4:
		twist.linear.y = -0.1
	elif t == 5:
		twist.angular.z = 0.1
	elif t == 6:
		twist.angular.z = -0.1
	elif t == 0:
		twist.linear.x = 0
		twist.linear.y = 0
		twist.angular.z = 0
		twist.angular.y = 0
	elif t == 7:
		twist.angular.y = twist.angular.y + 1
	elif t == 8:
		twist.angular.y = twist.angular.y - 1
		

	publisher.publish(twist)

	
if __name__ == '__main__':
 
  rospy.init_node('keys_to_Twist')

  publisher = rospy.Publisher('cmd_vel', Twist, queue_size=1)
  rospy.Subscriber('keys', String, keys_cb, publisher)
  rospy.spin()
