#include "MinimalOgre.h"
#include <btBulletDynamicsCommon.h>
class Cube
{
public:
    double x, y, z;
    Ogre::SceneNode* node;
    std::deque<Ball *> ballList;
    Simulator* simulator;
  public:
    
    Cube(Ogre::SceneNode* newnode, int cubeSize, Ogre::SceneManager* mSceneMgr, Simulator* sim)
    {
        node = newnode;
             // 10, 20
        cubeSize = 7;
        float ballSize = 50; //diameter
        // default size of sphere mesh is 200.
        float meshSize =  ballSize / 100; //200 is size of the sphere mesh. 100 is size of square.

        for(int x = 0; x < cubeSize; x++) {
            for (int y = 0; y < cubeSize; y++) {
                for(int z = 0; z < cubeSize; z++) {
                   
                    Ogre::Entity* ballMeshpc = mSceneMgr->createEntity("cube.mesh");
                    Ogre::SceneNode* nodepc = node->createChildSceneNode(); //node is the node that holds the cube.
                                                                            // we attach all balls to this node.
                    nodepc->attachObject(ballMeshpc);
                    nodepc->setScale(Ogre::Vector3(meshSize, meshSize, meshSize));
                    Ball* ball = new Ball(nodepc, x * ballSize, y * ballSize, z * ballSize, ballSize/2);
                    sim->addBall(ball);
                    ballList.push_front(ball);
                }
            }
        }
        simulator = sim;
    }

    void moveRight() {
      
     
        btRigidBody* body;
        Ball * ball;
        Ogre::Vector3 pos = node->getPosition();
        node->setPosition(++pos.x, pos.y, pos.z); //change position of the scenenode
        if(pos.x > 200)
            node->setPosition(0, pos.y, pos.z);
        for(int i = 0; i < ballList.size(); i++) {
            // get rigidbody
            ball = ballList[i];
            body = ball->getRigidBody();
            Ogre::Vector3 ballPos = ball->node->getPosition();
           // node->setPosition(++ballPos.x, ballPos.y, ballPos.z);

            simulator->dynamicsWorld->removeRigidBody(body);

            btTransform newTrans = body->getWorldTransform();
            newTrans.setOrigin( btVector3((ball->node->_getDerivedPosition().x), ball->node->_getDerivedPosition().y,
              ball->node->_getDerivedPosition().z));
         //   newTrans.setOrigin( btVector3((ball->node->getPosition().x), ball->node->getPosition().y,
         //       ball->node->getPosition().z));
            //newTrans.setRotation( newRotation );

            btTransform btt;
            btt.setOrigin(btt.getOrigin()--);

            std::cout << "positionNode " << ball->node->getPosition() <<std::endl;
            body->setWorldTransform( newTrans );


            //add it again
            simulator->dynamicsWorld->addRigidBody(body);
        }

/*

         btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);
    transform.setOrigin(btVector3(X, Y, Z));  // or whatever
    rigidBody->getMotionState()->setWorldTransform(transform);
    rigidBody->setCenterOfMassTransform(transform);
    */
    }

};
