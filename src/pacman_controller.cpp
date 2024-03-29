#include "ros/ros.h"
#include "pacman/Num.h"
#include "pacman/pacmanPos.h"
#include "pacman/ghostsPos.h"
#include "pacman/cookiesPos.h"
#include "pacman/bonusPos.h"
#include "pacman/game.h"
#include "pacman/pos.h"
#include "pacman/mapService.h"



void pacmanPosCallback(const pacman::pacmanPos::ConstPtr& msg)
{
  //ROS_INFO("Num Pacman: %d",msg->nPacman);
  for(int i = 0; i < msg->nPacman; i++)
  {
    //ROS_INFO("Pos Pacman [%d] :  x = [%d]  y = [%d]",i ,msg->pacmanPos[i].x, msg->pacmanPos[i].y);
  }
}
void ghostsPosCallback(const pacman::ghostsPos::ConstPtr& msg)
{
  //ROS_INFO("Num Fantasmas: %d",msg->nGhosts);
  for(int i = 0; i < msg->nGhosts; i++)
  {
    //ROS_INFO("Pos Fantasma [%d] :  x = [%d]  y = [%d]",i ,msg->ghostsPos[i].x, msg->ghostsPos[i].y);
    //ROS_INFO("Modo Fantasma [%d] :  [%d]",i ,msg->mode[i]);
  }
}
void cookiesPosCallback(const pacman::cookiesPos::ConstPtr& msg)
{
  //ROS_INFO("Num Cookies: %d",msg->nCookies);
  for(int i = 0; i < msg->nCookies; i++)
  {
    //ROS_INFO("Pos cookies [%d] :  x = [%d]  y = [%d]",i ,msg->cookiesPos[i].x, msg->cookiesPos[i].y);
  }
}
void bonusPosCallback(const pacman::bonusPos::ConstPtr& msg)
{
  //ROS_INFO("Num Bonus: %d",msg->nBonus);
  for(int i = 0; i < msg->nBonus; i++)
  {
    //ROS_INFO("Pos bonus [%d] :  x = [%d]  y = [%d]",i ,msg->bonusPos[i].x, msg->bonusPos[i].y);
  }
}
void gameStateCallback(const pacman::game::ConstPtr& msg)
{
  //ROS_INFO("Game State: %d",msg->state);
}
int main(int argc, char **argv)
{
    srand (time(NULL));
    ros::init(argc, argv, "pacman_controller");
    ros::NodeHandle n;
    ros::Publisher chatter_pub = n.advertise<pacman::Num>("pacmanActions", 100);
    ros::Subscriber posPacmanSub = n.subscribe("pacmanCoord", 100, pacmanPosCallback);
    ros::Subscriber posGhostsSub = n.subscribe("ghostsCoord", 100, ghostsPosCallback);
    ros::Subscriber posCookiesSub = n.subscribe("cookiesCoord", 100, cookiesPosCallback);
    ros::Subscriber posBonuSub = n.subscribe("bonusCoord", 100, bonusPosCallback);
    ros::Subscriber gameStateSub = n.subscribe("gameState", 100, gameStateCallback);
    ros::ServiceClient mapRequestClient = n.serviceClient<pacman::mapService>("pacman_world");
    pacman::mapService srv;
    ros::Rate loop_rate(1);
    int count = 0;
    while (ros::ok())
    {
      if(mapRequestClient.call(srv))
      {	
	printf("# obs: %d \n", srv.response.nObs);
	printf("minX: %d maxX: %d \n", srv.response.minX, srv.response.maxX);
	printf("minY: %d maxY: %d \n", srv.response.minY, srv.response.maxY);
	/*for(int i = 0; i < srv.response.nObs; i++)
	{
	  printf("Pos obstacles [%d] :  x = [%d]  y = [%d] \n",i ,srv.response.obs[i].x, srv.response.obs[i].y );
	}*/
      }
      else
      {
	printf("Error al llamar al servicio \n");
      }
      
      
      pacman::Num msg;
      msg.num = rand() % 5;
      //ROS_INFO("%d", msg.num);
      chatter_pub.publish(msg);
      ros::spinOnce();
      loop_rate.sleep();
      ++count;
    }
  return 0;
}