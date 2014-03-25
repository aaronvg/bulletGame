#include "MinimalOgre.h"
#include <btBulletDynamicsCommon.h>
class Cube
{
public:
    double x, y, z;
    Ogre::SceneNode* node;
    std::deque<Ball *> ballList;
    std::deque<Ogre::Vector3> positionList;
    Simulator* simulator;
  public:
    
    Cube(int cubeSize, Ogre::SceneManager* mSceneMgr, Simulator* sim)
    {
        //node = newnode;
             // 10, 20
        cubeSize = 12;
        float ballSize = 20; //diameter
        // default size of sphere mesh is 200.
        float meshSize =  ballSize / 100; //200 is size of the sphere mesh. 100 is size of square.

        for(int x = 0; x < cubeSize; x++) {
            for (int y = 0; y < cubeSize; y++) {
                for(int z = 0; z < cubeSize; z++) {
                   
                    Ogre::Entity* ballMeshpc = mSceneMgr->createEntity("cube.mesh");
                    Ogre::SceneNode* nodepc = mSceneMgr->getRootSceneNode()->createChildSceneNode();

                    //node->createChildSceneNode(); //node is the node that holds the cube.
                                                                            // we attach all balls to this node.
                    nodepc->attachObject(ballMeshpc);
                    nodepc->setScale(Ogre::Vector3(meshSize, meshSize, meshSize));
                    Ball* ball = new Ball(nodepc, x * ballSize, y * ballSize, z * ballSize, ballSize/2);
                    sim->addBall(ball);
                    ballList.push_front(ball);
                    positionList.push_front(Ogre::Vector3(x * ballSize, y * ballSize, z * ballSize)); //saves original position that they were in.
                }
            }
        }
        simulator = sim;
    }

    void moveRight() {
        btRigidBody* body;
        Ball * ball = ballList[0];
        Ogre::Vector3 pos = ball->node->getPosition();
        //node->setPosition(++pos.x, pos.y, pos.z); //change position of the scenenode
        std::cout << "positionNode " << ball->node->getPosition() <<std::endl;

        for(int i = 0; i < ballList.size(); i++) {
            ball = ballList[i];
            body = ball->getRigidBody();
            Ogre::Vector3 ballPos = ball->node->getPosition();
            if(ballPos.x > 1000) {
                ball->node->setPosition(ballPos.x - 1800, ballPos.y, ballPos.z);
                positionList[i].x -= 1800; //update cube position.
            }
            else {
                ball->node->setPosition(ballPos.x + 5, ballPos.y, ballPos.z);
                positionList[i].x += 5;
            }
            
            btTransform newTrans = body->getWorldTransform();

            newTrans.setOrigin( btVector3((ball->node->getPosition().x), ball->node->getPosition().y,
                ball->node->getPosition().z));
            //newTrans.setRotation( newRotation );
            body->setWorldTransform( newTrans );

            //add it again
            // simulator->dynamicsWorld->addRigidBody(body);
        }
    }

    void resetCube() {
        btRigidBody* body;
        Ball * ball = ballList[0];
        Ogre::Vector3 pos = ball->node->getPosition();
        for(int i = 0; i < ballList.size(); i++) {
            ball = ballList[i];
            body = ball->getRigidBody();
            Ogre::Vector3 ballPos = ball->node->getPosition();
            // ball->node->setPosition(ballPos.x - 1800, ballPos.y, ballPos.z);

            float lerp = .01f;

            // ballPos is the position of the ball (mesh).
            // positionList[i] is the target position we want to reach.
            ballPos.x += (positionList[i].x - ballPos.x) * lerp;
            ballPos.y += (positionList[i].y - ballPos.y) * lerp;
            ballPos.z += (positionList[i].z - ballPos.z) * lerp;

            // set position of node back to new calculated value.
            ball->node->setPosition(ballPos);

            // update the rigidbody based on this new position.
            btTransform newTrans = body->getWorldTransform();
            newTrans.setOrigin( btVector3((ball->node->getPosition().x), ball->node->getPosition().y,
                   ball->node->getPosition().z));
            body->setWorldTransform( newTrans );

        }
    }
};
