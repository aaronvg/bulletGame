/*
-----------------------------------------------------------------------------
Filename:    MinimalOgre.cpp
-----------------------------------------------------------------------------

This source file is part of the
   ___                 __    __ _ _    _ 
  /___\__ _ _ __ ___  / / /\ \ (_) | _(_)
 //  // _` | '__/ _ \ \ \/  \/ / | |/ / |
/ \_// (_| | | |  __/  \  /\  /| |   <| |
\___/ \__, |_|  \___|   \/  \/ |_|_|\_\_|
      |___/                              
      Tutorial Framework
      http://www.ogre3d.org/tikiwiki/
-----------------------------------------------------------------------------
*/
#include "MinimalOgre.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#   include <macUtils.h>
#   include "AppDelegate.h"
#endif

//-------------------------------------------------------------------------------------
MinimalOgre::MinimalOgre(void)
    : mRoot(0),
    mCamera(0),
    mSceneMgr(0),
    mWindow(0),
    mResourcesCfg(Ogre::StringUtil::BLANK),
    mPluginsCfg(Ogre::StringUtil::BLANK),
    mTrayMgr(0),
    mCameraMan(0),
    mDetailsPanel(0),
    scorePanel(0),
    mCursorWasVisible(false),
    mShutDown(false),
    mInputManager(0),
    mMouse(0),
    mKeyboard(0),
    mState(0),
    headNode(0),
    vZero(Ogre::Vector3::ZERO)
{
	mTimer = OGRE_NEW Ogre::Timer();
	mTimer->reset();
}
//-------------------------------------------------------------------------------------
MinimalOgre::~MinimalOgre(void)
{
    if (mTrayMgr) delete mTrayMgr;
    if (mCameraMan) delete mCameraMan;

    //Remove ourself as a Window listener
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
    windowClosed(mWindow);
    delete mRoot;
}

bool MinimalOgre::go(void)
{
#ifdef _DEBUG
    mResourcesCfg = "resources_d.cfg";
    mPluginsCfg = "plugins_d.cfg";
#else
    mResourcesCfg = "resources.cfg";
    mPluginsCfg = "plugins.cfg";
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    Ogre::String workingDir = Ogre::macBundlePath()+"/Contents/Resources/";
    mResourcesCfg = workingDir + mResourcesCfg;
    mPluginsCfg = workingDir + mPluginsCfg;
#endif

    // construct Ogre::Root
    mRoot = new Ogre::Root(mPluginsCfg);

//-------------------------------------------------------------------------------------
    // setup resources
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load(mResourcesCfg);

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // OS X does not set the working directory relative to the app,
            // In order to make things portable on OS X we need to provide
            // the loading with it's own bundle path location
            if (!Ogre::StringUtil::startsWith(archName, "/", false)) // only adjust relative dirs
                archName = Ogre::String(Ogre::macBundlePath() + "/Contents/Resources/" + archName);
#endif
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                archName, typeName, secName);
        }
    }

//-------------------------------------------------------------------------------------
    // configure
    // Show the configuration dialog and initialise the system
    // You can skip this and use root.restoreConfig() to load configuration
    // settings if you were sure there are valid ones saved in ogre.cfg
    if(mRoot->restoreConfig() || mRoot->showConfigDialog())
    {
        // If returned true, user clicked OK so initialise
        // Here we choose to let the system create a default rendering window by passing 'true'
        mWindow = mRoot->initialise(true, "MinimalOgre Render Window");
    }
    else
    {
        return false;
    }
//-------------------------------------------------------------------------------------
    // choose scenemanager
    // Get the SceneManager, in this case a generic one
    mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);
    mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
    mSceneMgr->setShadowFarDistance(2500.0);
//-------------------------------------------------------------------------------------
    // create camera
    // Create the camera
    mCamera = mSceneMgr->createCamera("PlayerCam");
    mCamera->setFOVy(Ogre::Radian(1.50));

    // Position it at 500 in Z direction
    mCamera->setPosition(Ogre::Vector3(0,0,2000));
    // Look back along -Z
    mCamera->lookAt(vZero);
    mCamera->setNearClipDistance(5);

    mCameraMan = new CameraMan(mCamera);   // create a default camera controller
//-------------------------------------------------------------------------------------
    // create viewports
    // Create one viewport, entire window
    Ogre::Viewport* vp = mWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(.01,.03,.01));

    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(
        Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));
//-------------------------------------------------------------------------------------
    // Set default mipmap level (NB some APIs ignore this)
    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
//-------------------------------------------------------------------------------------
    // Create any resource listeners (for loading screens)
    //createResourceListener();
//-------------------------------------------------------------------------------------
    // load resources
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
//-------------------------------------------------------------------------------------
    // Create the scene

    // Create the bounding geometry, used only in collision testing.
    //ballBound = Ogre::Sphere(vZero, 200);
    boxBound = Ogre::PlaneBoundedVolume(Ogre::Plane::NEGATIVE_SIDE);
    boxBound.planes.push_back(wallBack = Ogre::Plane(Ogre::Vector3::NEGATIVE_UNIT_Z, 0));
    boxBound.planes.push_back(wallFront = Ogre::Plane(Ogre::Vector3::UNIT_Z,0));
    boxBound.planes.push_back(wallDown = Ogre::Plane(Ogre::Vector3::UNIT_Y,0));
    boxBound.planes.push_back(wallUp = Ogre::Plane(Ogre::Vector3::NEGATIVE_UNIT_Y,0));
    boxBound.planes.push_back(wallLeft = Ogre::Plane(Ogre::Vector3::UNIT_X, 0));
    boxBound.planes.push_back(wallRight = Ogre::Plane(Ogre::Vector3::NEGATIVE_UNIT_X,0));


    // Use the planes from above to generate new meshes for walls.
    Ogre::MeshManager::getSingleton().createPlane("ground",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, wallDown,
        WALL_SIZE, WALL_SIZE, 20, 20, true, 1, 1, 1, Ogre::Vector3::UNIT_Z);
    Ogre::Entity* entGround = mSceneMgr->createEntity("GroundEntity", "ground");
    entGround->setMaterialName("Custom/texture_blend");
    entGround->setCastShadows(false);
    Ogre::SceneNode* nodeGround = mSceneMgr->getRootSceneNode()->createChildSceneNode(); 
    nodeGround->setPosition(0 , -PLANE_DIST, 0); //1600 / 5 is our tilewidth
    nodeGround->attachObject(entGround);


    Ogre::MeshManager::getSingleton().createPlane("ceiling",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, wallUp,
        WALL_SIZE, WALL_SIZE, 20, 20, true, 1, 2, 2, Ogre::Vector3::UNIT_Z);
    Ogre::Entity* entCeiling = mSceneMgr->createEntity("CeilingEntity", "ceiling");
    entCeiling->setMaterialName("Examples/CloudySky");
    entCeiling->setCastShadows(false);
    Ogre::SceneNode* nodeCeiling = mSceneMgr->getRootSceneNode()->createChildSceneNode(); 
    nodeCeiling->setPosition(0 , PLANE_DIST, 0); //1600 / 5 is our tilewidth
    nodeCeiling->attachObject(entCeiling);

    // mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entCeiling);
    // entCeiling->setMaterialName("Examples/CloudySky");
    // entCeiling->setCastShadows(false);

    Ogre::MeshManager::getSingleton().createPlane("back",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, wallBack,
        WALL_SIZE, WALL_SIZE, 20, 20, true, 1, 5, 5, Ogre::Vector3::UNIT_Y);
    Ogre::Entity* entBack = mSceneMgr->createEntity("BackEntity", "back");
    entBack->setMaterialName("Examples/Rockwall");
    entBack->setCastShadows(false);
    Ogre::SceneNode* nodeBack = mSceneMgr->getRootSceneNode()->createChildSceneNode("backNode"); 
    nodeBack->setPosition(0 , 0, PLANE_DIST); //1600 / 5 is our tilewidth
    nodeBack->attachObject(entBack);


    // mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entBack);
    // entBack->setMaterialName("Examples/Rockwall");
    // entBack->setCastShadows(false);

    Ogre::MeshManager::getSingleton().createPlane("front",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, wallFront,
        WALL_SIZE, WALL_SIZE, 20, 20, true, 1, 5, 5, Ogre::Vector3::UNIT_Y);
    Ogre::Entity* entFront = mSceneMgr->createEntity("FrontEntity", "front");
    entFront->setMaterialName("Examples/Rockwall");
    entFront->setCastShadows(false);
    Ogre::SceneNode* nodeFront = mSceneMgr->getRootSceneNode()->createChildSceneNode("frontNode"); 
    nodeFront->setPosition(0 , 0, -PLANE_DIST); //1600 / 5 is our tilewidth
    nodeFront->attachObject(entFront);



    // mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entFront);
    // entFront->setMaterialName("Examples/Rockwall");
    // entFront->setCastShadows(false);

    Ogre::MeshManager::getSingleton().createPlane("left",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, wallLeft,
        WALL_SIZE, WALL_SIZE, 20, 20, true, 1, 5, 5, Ogre::Vector3::UNIT_Y);
    Ogre::Entity* entLeft = mSceneMgr->createEntity("LeftEntity", "left");
    entLeft->setMaterialName("Examples/Rockwall");
    entLeft->setCastShadows(false);
    Ogre::SceneNode* nodeLeft = mSceneMgr->getRootSceneNode()->createChildSceneNode("leftNode"); 
    nodeLeft->setPosition(-PLANE_DIST , 0, 0); //1600 / 5 is our tilewidth
    nodeLeft->attachObject(entLeft);


    // mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entLeft);
    // entLeft->setMaterialName("Examples/Rockwall");
    // entLeft->setCastShadows(false);

    Ogre::MeshManager::getSingleton().createPlane("right",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, wallRight,
        WALL_SIZE, WALL_SIZE, 20, 20, true, 1, 5, 5, Ogre::Vector3::UNIT_Y);
    Ogre::Entity* entRight = mSceneMgr->createEntity("RightEntity", "right");
    entRight->setMaterialName("Examples/Rockwall");
    entRight->setCastShadows(false);
    Ogre::SceneNode* nodeRight = mSceneMgr->getRootSceneNode()->createChildSceneNode("rightNode"); 
    nodeRight->setPosition(PLANE_DIST, 0, 0); //1600 / 5 is our tilewidth
    nodeRight->attachObject(entRight);


    // mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entRight);
    // entRight->setMaterialName("Examples/Rockwall");
    // entRight->setCastShadows(false);

    moving = false;


    // Set up simulation/bullet collision objects
    sim = new Simulator(mSceneMgr);


    sim->addPlane(0, 1, 0, -PLANE_DIST);
    sim->addPlane(0, -1, 0, -PLANE_DIST);
    sim->addPlane(1, 0, 0, -PLANE_DIST);
    sim->addPlane(-1, 0, 0, -PLANE_DIST);
    sim->addPlane(0, 0, 1, -PLANE_DIST);
    sim->addPlane(0, 0, -1, -PLANE_DIST);


    // Create the visible mesh ball.
    
    Ogre::Entity* ballMesh = mSceneMgr->createEntity("Ball", "sphere.mesh");
    ballMesh->setMaterialName("Examples/SphereMappedRustySteel");
    ballMesh->setCastShadows(true);

    globalBall = NULL;


    // Attach the node.
    headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    headNode->attachObject(ballMesh);

    Ball* ball = new Ball(headNode, -80, 0, -350, 100);
    sim->addMainBall(ball);
    ball->removeGravity();
    // Create tile meshes and collision objects for each tile.
    levelSetup(1);

    theCube = mSceneMgr->getRootSceneNode()->createChildSceneNode("cubeNode");
    cubeObject = new Cube(12, mSceneMgr, sim);
    theCube->setPosition(Ogre::Vector3(-40, 10, 80));

    // TODO, constructor determines where we start building cube.
   // cubeObject = new Cube(theCube, 12, mSceneMgr, sim);

    // setUpBalls(5);

    // Handles the simonSays initial animation:
    gameStart = true;
    currTile = tileEntities.size() - 1; // which tile has to be lit up first

    newgame = false;
    score = 0;

    // Set ambient light
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.15, 0.15, 0.15));

    // Create a light
    Ogre::Light* lSun = mSceneMgr->createLight("SunLight");
    lSun->setType(Ogre::Light::LT_POINT);
    lSun->setDiffuseColour(0.95, 0.95, 1.00);
    lSun->setPosition(2000,-1000,0);
    lSun->setAttenuation(3250, 1.0, 0.0000000001, 0.000001);
    lSun->setCastShadows(false);
//-------------------------------------------------------------------------------------
    //create FrameListener
    Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
    OIS::ParamList pl;
    size_t windowHnd = 0;
    std::ostringstream windowHndStr;

    mWindow->getCustomAttribute("WINDOW", &windowHnd);
    windowHndStr << windowHnd;
    pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

    mInputManager = OIS::InputManager::createInputSystem( pl );

    mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject( OIS::OISKeyboard, true ));
    mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject( OIS::OISMouse, true ));

    mMouse->setEventCallback(this);
    mKeyboard->setEventCallback(this);

    //Set initial mouse clipping size
    windowResized(mWindow);

    //Register as a Window listener
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

    mTrayMgr = new OgreBites::SdkTrayManager("InterfaceName", mWindow, mMouse, this);
    mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
    mTrayMgr->showLogo(OgreBites::TL_BOTTOMRIGHT);
    mTrayMgr->hideCursor();

    // create a params panel for displaying sample details
    Ogre::StringVector items;
    items.push_back("cam.pX");
    items.push_back("cam.pY");
    items.push_back("cam.pZ");
    items.push_back("");
    items.push_back("cam.oW");
    items.push_back("cam.oX");
    items.push_back("cam.oY");
    items.push_back("cam.oZ");
    items.push_back("");
    items.push_back("Filtering");
    items.push_back("Poly Mode");

    mDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_NONE, "DetailsPanel", 200, items);
    mDetailsPanel->setParamValue(9, "Bilinear");
    mDetailsPanel->setParamValue(10, "Solid");
    mDetailsPanel->hide();

    Ogre::StringVector scorelist;
    scorelist.push_back("Score");

    scorePanel = mTrayMgr->createParamsPanel(OgreBites::TL_TOPLEFT, "ScorePanel", 200, scorelist);

    paused = false;
    slowdownval = 0.0;


    mRoot->addFrameListener(this);
//-------------------------------------------------------------------------------------
    mRoot->startRendering();

    return true;
}




// Set up the level
//  - attaches textured tiles placed at random locations
//  to the main SceneNode depending on what level it is.
void MinimalOgre::levelSetup(int num) {

    int x = 0;
    int y = 0;
    int z = 0;
    // Since each mesh starts at the center of the plane, we need to offset it
    // to the top right corner of the plane and start counting from there.
    int offset = WALL_SIZE/2 - TILE_WIDTH/2;

    srand(time(0));

    for(int i = 0; i < num; i++) {
        Ogre::Plane wallTile = Ogre::Plane(Ogre::Vector3::UNIT_X, -PLANE_DIST +1);
        std::stringstream ss;

        std::stringstream ssDebug;
        ss << i;

        // TODO, check random num was not already seen, or maybe we do want
        // to have tiles repeat in the sequence we need
        int tileNum = std::rand() % 99; // get random tile between 0 and 99;
        ssDebug << tileNum;
        std::cout << "Random number1: " + ssDebug.str() << std::endl;
        ssDebug.str(std::string());

        int wallTileNum = tileNum % NUM_TILES_WALL; //possible number of tiles per wall.
        ssDebug << wallTileNum;
        std::cout << "Random number: " + ssDebug.str() << std::endl;
        int row = wallTileNum / NUM_TILES_ROW; //5 is the number of tiles per row.
        int col = wallTileNum % NUM_TILES_ROW;

        ssDebug.str(std::string());
        ssDebug << row;
        ssDebug << " ";
        ssDebug << col;

        std::cout << "Row/col " + ssDebug.str() << std::endl;

        Ogre::SceneNode* node1; //= mSceneMgr->getRootSceneNode()->createChildSceneNode(); 
        int xsize = 240;
        int ysize = 240;
        int zsize = 240;


        // left
        if(tileNum < 25) {
            // set up x y z units of 0, 1 or -1, which will be used when we setPosition
            // set up our WallTileLeft or whatever to point to the right direction.
            wallTile = Ogre::Plane(Ogre::Vector3::UNIT_X, 1);
            x = 0;
            y = -1 * (row * TILE_WIDTH) + offset;
            z = -1 * (col * TILE_WIDTH) + offset;
            xsize = 10;
            node1 = mSceneMgr->getSceneNode("leftNode")->createChildSceneNode();
        }

        // front
        else if (tileNum < 50) {
            x = 1 * (col * TILE_WIDTH) - offset;
            y = -1 * (row * TILE_WIDTH) + offset;
            z = 0;
            zsize = 10;
            wallTile = Ogre::Plane(Ogre::Vector3::UNIT_Z, 1);
            node1 = mSceneMgr->getSceneNode("frontNode")->createChildSceneNode();

            Ogre::SceneNode* parent = mSceneMgr->getSceneNode("frontNode");

        }

        // right
        else if (tileNum < 75) {
            x = 0;
            y = -1 * (row * TILE_WIDTH) + offset;
            z = 1 * (col * TILE_WIDTH) - offset;
            xsize = 10;
            wallTile = Ogre::Plane(Ogre::Vector3::NEGATIVE_UNIT_X, 1);
            node1 = mSceneMgr->getSceneNode("rightNode")->createChildSceneNode();
        }

        // back
        else if (tileNum < 100) {
            x = 1 * (col * TILE_WIDTH) - offset;
            y = -1 * (row * TILE_WIDTH) + offset;
            z = 0;
            zsize = 10;
            wallTile = Ogre::Plane(Ogre::Vector3::NEGATIVE_UNIT_Z, 1);
            node1 = mSceneMgr->getSceneNode("backNode")->createChildSceneNode();
        }
       




        // Build the entity name based on which tile number it is.
        std::string str = "tile";
        str.append(ss.str());
        std::string entityStr = "tileEntity";
        entityStr.append(ss.str());
        std::cout << "tileEntityName: " + entityStr << std::endl;

        Ogre::MeshManager::getSingleton().createPlane(str,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, wallTile,
        TILE_WIDTH, TILE_WIDTH, 20, 20, true, 1, 5, 5, Ogre::Vector3::UNIT_Y);
        Ogre::Entity* tile = mSceneMgr->createEntity(entityStr, str);
        
        node1->translate(x ,y, z); //1600 / 5 is our tilewidth
        node1->attachObject(tile);
        tile->setMaterialName("Examples/Chrome");
        tile->setCastShadows(false);
        sim->addTile(node1, xsize, ysize, zsize);
        tileEntities.push_back(tile);
    }
}

void MinimalOgre::setUpBalls(int cubeSize) {
    // 10, 20
    cubeSize = 12;
    float ballSize = 50; //diameter
    // default size of sphere mesh is 200.
    float meshSize =  ballSize / 100; //200 is size of the mesh.
   
                
        // ballMeshpc->setCastShadows(true);

    for(int x = 0; x < cubeSize; x++) {
        for (int y = 0; y < cubeSize; y++) {
            for(int z = 0; z < cubeSize; z++) {
               
                Ogre::Entity* ballMeshpc = mSceneMgr->createEntity("cube.mesh");
                Ogre::SceneNode* nodepc = mSceneMgr->getRootSceneNode()->createChildSceneNode();
                nodepc->attachObject(ballMeshpc);
                nodepc->setScale(Ogre::Vector3(meshSize, meshSize, meshSize));

                Ball* ball = new Ball(nodepc, x * ballSize, y * ballSize, z * ballSize, ballSize/2);
                sim->addBall(ball);
                //  double force = 4000.0;
                //  Ogre::Vector3 direction = mCamera->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Z;
                //  ball->applyForce(force * direction.x, force * direction.y, force * direction.z);
            }
        }
    }
}


bool MinimalOgre::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if(mWindow->isClosed())
        return false;

    if(mShutDown)
        return false;

    if(paused) {
        slowdownval += 1/1800.f;
        Ogre::Entity* tile = tileEntities[0];   // mSceneMgr->getEntity("tileEntity0");     
        tile->setMaterialName("Examples/Chrome");

    }
    else {
      //  Ogre::Entity* tile = tileEntities[0];   // mSceneMgr->getEntity("tileEntity0");     
      //  tile->setMaterialName("Examples/BumpyMetal");
    }
    if(slowdownval <= 1/60.f)
    {
        bool hit;
        if(tileEntities.size() > 0) {
            hit = sim->simulateStep(slowdownval);
        }
        if(hit)
        {
            if(tileEntities.size() > 0) {
                tileEntities.back()->setMaterialName("Examples/BumpyMetal");
                tileEntities.pop_back();
            }
            score++;
        }
    }

    if(moving)
    {
        // move the cube.
       // std::cout << "position " << theCube->getPosition() << std::endl;
        Ogre::Vector3 pos = theCube->getPosition();
        cubeObject->moveRight();
    }

    // if we press "n" on the keyboard, it toggles the gradual cubeReset.
    if(newgame) {
        cubeObject->resetCube();
    }

    // If this is the first time we start the game, lets play the simon says animation
    if(gameStart) {
        timer.reset();
        gameStart = false;
    }


    //simonSaysAnim();


    // Get collision in each plane (or just front plane for now)
        // check if collision contact points are within our tile xy (if it's front plane)
    

    /********************************************************************
     * Animation
     */

    //Ogre::Vector3 point = headNode->getPosition();
    //Ogre::Real adjust = 0.0;
    //bool found = false;

    // Given a bounding box, we can easily test each plane in the PlaneList.
    /*
    for (int i = 0; i < 6 && !found; i++) {
      Ogre::Real dist = boxBound.planes[i].getDistance(ballBound.getCenter());
      if (dist < 100.01) {
        mDirection = mDirection.reflect(boxBound.planes[i].normal);
        adjust = 100.5 - dist;
        found = true;
      }
    } */

    // Add distance traveled plus collision adjustment, and update position.
    //point = point + (((evt.timeSinceLastFrame * mSpeed) + adjust) * mDirection);
    //ballBound.setCenter(point);
    //headNode->setPosition(point);

    /*******************************************************************/

    //Need to capture/update each device
    mKeyboard->capture();
    mMouse->capture();

    mTrayMgr->frameRenderingQueued(evt);
    
    if (!mTrayMgr->isDialogVisible())
    {
        mCameraMan->frameRenderingQueued(evt);   // if dialog isn't up, then update the camera
        if (mDetailsPanel->isVisible())   // if details panel is visible, then update its contents
        {
            mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
            mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
            mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
            mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().w));
            mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().x));
            mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().y));
            mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().z));
        }
        scorePanel->setParamValue(0, Ogre::StringConverter::toString(score));
    }
    return true;
}



// Lights up the tiles in the tileEntities, from the back of the deque to the front.
void MinimalOgre::simonSaysAnim() {
    long currTime = timer.getMilliseconds();
    //   std::cout << currTime << std::endl;
    int numTiles = tileEntities.size();
    //std::cout << numTiles << std::endl;
    int startTime = 2000; // starts 2 secs into the game.
    int timePerTile = 1500; // each tile lights up for this duration (2 secs)
    int waitTime = 100; // waits 500ms between each tile being lit up.

    int animStart = (waitTime + timePerTile) * (tileEntities.size()-1 - currTile) + startTime;
    int animEnd = animStart + timePerTile;
    if(currTile >= 0) {
        if(currTime > animStart && currTime <= animEnd)
        {
            // If we are at the initial tile, revert the last tile to original texture
            // since we are currently just looping all the time through them and changing their textures.
            if(currTile == tileEntities.size() - 1) {
                tileEntities[0]->setMaterialName("Examples/Chrome");
            }
            // Revert previous tile to original texture
            if(currTile + 1 < tileEntities.size()) {
                tileEntities[currTile + 1]->setMaterialName("Examples/Chrome");
            }
            tileEntities[currTile]->setMaterialName("Examples/BumpyMetal");
            // moves on to the next tile.
            currTile--;

        }
    }
    else {
        if(tileEntities.size() > 0) {
            currTile = tileEntities.size() - 1;
            timer.reset();
        }
    }

}



//-------------------------------------------------------------------------------------
bool MinimalOgre::keyPressed( const OIS::KeyEvent &arg )
{
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

    if (arg.key == OIS::KC_F)   // toggle visibility of advanced frame stats
    {
        mTrayMgr->toggleAdvancedFrameStats();
    }
    else if (arg.key == OIS::KC_G)   // toggle visibility of even rarer debugging details
    {
        if (mDetailsPanel->getTrayLocation() == OgreBites::TL_NONE)
        {
            mTrayMgr->moveWidgetToTray(mDetailsPanel, OgreBites::TL_TOPRIGHT, 0);
            mDetailsPanel->show();
        }
        else
        {
            mTrayMgr->removeWidgetFromTray(mDetailsPanel);
            mDetailsPanel->hide();
        }
    }
    else if (arg.key == OIS::KC_T)   // cycle polygon rendering mode
    {
        Ogre::String newVal;
        Ogre::TextureFilterOptions tfo;
        unsigned int aniso;

        switch (mDetailsPanel->getParamValue(9).asUTF8()[0])
        {
        case 'B':
            newVal = "Trilinear";
            tfo = Ogre::TFO_TRILINEAR;
            aniso = 1;
            break;
        case 'T':
            newVal = "Anisotropic";
            tfo = Ogre::TFO_ANISOTROPIC;
            aniso = 8;
            break;
        case 'A':
            newVal = "None";
            tfo = Ogre::TFO_NONE;
            aniso = 1;
            break;
        default:
            newVal = "Bilinear";
            tfo = Ogre::TFO_BILINEAR;
            aniso = 1;
            break;
        }

        Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
        Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(aniso);
        mDetailsPanel->setParamValue(9, newVal);
    }
    else if (arg.key == OIS::KC_R)   // cycle polygon rendering mode
    {
        Ogre::String newVal;
        Ogre::PolygonMode pm;

        switch (mCamera->getPolygonMode())
        {
        case Ogre::PM_SOLID:
            newVal = "Wireframe";
            pm = Ogre::PM_WIREFRAME;
            break;
        case Ogre::PM_WIREFRAME:
            newVal = "Points";
            pm = Ogre::PM_POINTS;
            break;
        default:
            newVal = "Solid";
            pm = Ogre::PM_SOLID;
            break;
        }

        mCamera->setPolygonMode(pm);
        mDetailsPanel->setParamValue(10, newVal);
    }
    else if(arg.key == OIS::KC_F5)   // refresh all textures
    {
        Ogre::TextureManager::getSingleton().reloadAll();
    }
    else if (arg.key == OIS::KC_SYSRQ)   // take a screenshot
    {
        mWindow->writeContentsToTimestampedFile("screenshot", ".jpg");
    }
    else if (arg.key == OIS::KC_ESCAPE)
    {
        mShutDown = true;
    }
    else if (arg.key == OIS::KC_SPACE)
    {
    }
    else if (arg.key == OIS::KC_P)
    {
        paused = !paused;
        slowdownval = 0.0;
    }
    else if(arg.key == OIS::KC_M) {
        moving = !moving;
        
    }
    else if(arg.key == OIS::KC_N) {
        newgame = !newgame;
    }

    mCameraMan->injectKeyDown(arg);
    return true;
}

bool MinimalOgre::keyReleased( const OIS::KeyEvent& arg )
{
    mCameraMan->injectKeyUp(arg);
    return true;
}

bool MinimalOgre::mouseMoved( const OIS::MouseEvent& arg )
{
    if (mTrayMgr->injectMouseMove(arg)) return true;
    mCameraMan->injectMouseMove(arg);
    return true;
}

bool MinimalOgre::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    // if there is an old ball, remove it
    // create a new ball
    if(globalBall != NULL) {
        sim->removeBall(globalBall);
        delete globalBall->node;
    }
 


    // set oldball pointer to the new ball.


    Ogre::Entity* ballMeshpc = mSceneMgr->createEntity("sphere.mesh");
    //ballMeshpc->setMaterialName("Examples/SphereMappedRustySteel");
    ballMeshpc->setCastShadows(true);

    Ogre::SceneNode* nodepc = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    nodepc->attachObject(ballMeshpc);
    int x = mCamera->getPosition().x;
    int y = mCamera->getPosition().y;
    int z = mCamera->getPosition().z;
     //nodepc->setScale(Ogre::Vector3(.5, .5, .5));

      float ballSize = 220; //diameter
        // default size of sphere mesh is 200.
     float meshSize =  ballSize / 200; //200 is size of the sphere mesh. 100 is size of square.
    nodepc->setScale(Ogre::Vector3(meshSize, meshSize, meshSize));
    globalBall = new Ball(nodepc, x, y, z, ballSize/2);
    globalBall->increaseMass(15);
    sim->addBall(globalBall);
    double force = 150000.0;
    Ogre::Vector3 direction = mCamera->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Z;
    globalBall->applyForce(force * direction.x, force * direction.y, force * direction.z);

    if (mTrayMgr->injectMouseDown(arg, id)) return true;
        mCameraMan->injectMouseDown(arg, id);
    return true;
}

bool MinimalOgre::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    if (mTrayMgr->injectMouseUp(arg, id)) return true;
    mCameraMan->injectMouseUp(arg, id);
    return true;
}

//Adjust mouse clipping area
void MinimalOgre::windowResized(Ogre::RenderWindow* rw)
{
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, depth, left, top);

    const OIS::MouseState &ms = mMouse->getMouseState();
    ms.width = width;
    ms.height = height;
}

//Unattach OIS before window shutdown (very important under Linux)
void MinimalOgre::windowClosed(Ogre::RenderWindow* rw)
{
    //Only close for window that created OIS (the main window in these demos)
    if( rw == mWindow )
    {
        if( mInputManager )
        {
            mInputManager->destroyInputObject( mMouse );
            mInputManager->destroyInputObject( mKeyboard );

            OIS::InputManager::destroyInputSystem(mInputManager);
            mInputManager = 0;
        }
    }
}



#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
    int main(int argc, char *argv[])
#endif
    {
#if defined(OGRE_IS_IOS)
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        int retVal = UIApplicationMain(argc, argv, @"UIApplication", @"AppDelegate");
        [pool release];
        return retVal;
#elif (OGRE_PLATFORM == OGRE_PLATFORM_APPLE) && __LP64__
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

        mAppDelegate = [[AppDelegate alloc] init];
        [[NSApplication sharedApplication] setDelegate:mAppDelegate];
        int retVal = NSApplicationMain(argc, (const char **) argv);

        [pool release];

        return retVal;
#else
        // Create application object
        MinimalOgre app;

        try {
            app.go();
        } catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
            std::cerr << "An exception has occured: " <<
                e.getFullDescription().c_str() << std::endl;
#endif
        }
#endif
        return 0;
    }

#ifdef __cplusplus
}
#endif
