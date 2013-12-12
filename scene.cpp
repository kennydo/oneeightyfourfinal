#include "scene.h"
#include <cstdio>
#include <iostream>
#include <cmath>

Scene::Scene(ParsedObj* o, Skeleton* s, Kinematics* k){
    // initialize variables
    renderMode = GL_RENDER;
    mouseButtonPressed = 0;
    translateX = 0;
    translateY = 0;
    rotateAboutX = 0;
    rotateAboutY = 0;
    scaleFactor = 1.0;
    selectedJointId = -1;

    mousePreviousX = 0;
    mousePreviousY = 0;
    windowPreviousX = 0;
    windowPreviousY = 0;
    delta = Vector3f(0, 0, 0);

    // just to be explicit about what obj is
    obj = o;
    skeleton = s;
    kinematics = k;

    // When the scene is initialized the GL params aren't
    // set yet and this will cause a segfault
    converter = NULL;
}

void Scene::refreshCamera(int mouseX, int mouseY){
    // the x and y are where the click happened.
    // when the renderMode isn't GL_SELECT, those values aren't used for anything
    // so just pass in dummy 0,0
    int viewport[4];
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    GLfloat light_pos[4] = {0.0, 0.0, -1.0, 0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);


   //glRotatef(30, 1, 0, 0);
   //glRotatef(30, 0, 1, 0);


    // we'll probably want to use AABB and figure out scaling
    glScalef(0.05, 0.05, 0.05);
    glRotatef(rotateAboutX, 1, 0, 0);
    glRotatef(rotateAboutY, 0, 1, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(renderMode == GL_SELECT){
        glGetIntegerv(GL_VIEWPORT, viewport);
        gluPickMatrix((double) mouseX, (double) mouseY,
                      PICK_TOLERANCE, PICK_TOLERANCE,
                      viewport);
    }

    // we'll probably want to not be using ortho projection
    glOrtho(-0.6, 0.6,
            -0.6, 0.6,
            -1.0, 1.0);
    glScalef(scaleFactor, scaleFactor, scaleFactor);
    glTranslatef(translateX * 0.05, translateY * 0.05, 0);
}

void Scene::draw(){
    glInitNames();
    if(skeleton != NULL){
        drawSkeleton();
    }
    drawTestSkeleton();
    if(obj != NULL){
        //drawObj();
    }
    drawGrid();

}

void Scene::drawGrid() {
    
    glPushAttrib(GL_ENABLE_BIT); 
    
    glLineStipple(1, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(-100, 0, 0);
    glVertex3f(100, 0, 0);
    glEnd();
    
    glBegin(GL_LINES);
    glVertex3f(0, -100, 0);
    glVertex3f(0, 100, 0);
    glEnd();
    
    glBegin(GL_LINES);
    glVertex3f(0, 0, -100);
    glVertex3f(0, 0, 100);
    glEnd();
    
    glPopAttrib();
}

void Scene::drawSkeleton(){
    Link *joint, *parent;

    for(int i=0; i < int(skeleton->joints.size()); i++){
        joint = skeleton->joints[i];
        parent = skeleton->parents[i];

        if(parent != NULL){
            glColor3f(0.0, 1.0, 1.0);
            glBegin(GL_LINES);
            glVertex3f(parent->pos().x(), parent->pos().y(), parent->pos().z());
            glVertex3f(joint->pos().x(), joint->pos().y(), joint->pos().z());
            glEnd();
        }

        // now draw the ball joints!
        glPushName(0);
        glLoadName(i);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(joint->pos().x(), joint->pos().y(), joint->pos().z());
        if(i == selectedJointId){
            glColor3f(1.0, 1.0, 0.0);
        } else {
            glColor3f(0.0, 0.0, 1.0);
        }
        glutWireSphere(0.5, 10, 10);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        if(i == selectedJointId){
            glColor4f(1.0, 1.0, 0.0, 0.1);
        } else {
            glColor4f(0.0, 0.0, 1.0, 0.1);
        }
        glutSolidSphere(0.5, 10, 10);
        glPopMatrix();

        glPopName();
    }
}

void Scene::drawTestSkeleton() {
    Vector3f p1, p2;
    Link link;
    
    for (unsigned int i = 0; i < kinematics->path_.size(); i++) {
        link = (kinematics->path_)[i];
        
        //if root link, then first point is origin
        if (link.getInnerLink() == -1)  {
            p1 = kinematics->origin_;
        } else { // otherwise first point is inner link's position
            p1 = (kinematics->path_[(link.getInnerLink())]).pos();
        }
        
        //second point is just current link's position
        p2 = link.pos();
    
        //draw the link
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINES);
        glVertex3f(p1.x(), p1.y(), p1.z());
        glVertex3f(p2.x(), p2.y(), p2.z());
        glEnd();
    
    }
    
}

void Scene::rotateTestSkeleton(Quaternionf q) {
    kinematics->solveFK(kinematics->path_[0], q);
}

void Scene::drawObj(){
    if(obj == NULL){ return; }

    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBegin(GL_TRIANGLES);
    ObjFace* face;
    Eigen::Vector3f *normal, *vertex;
    for(unsigned int i=0; i < obj->faces.size(); i++){
        face = obj->faces[i];
        normal = face->normal;

        for(int j=0; j<3; j++){
            vertex = face->vertices[j];
            glNormal3f(normal->x(), normal->y(), normal->z());
            glVertex3f(vertex->x(), vertex->y(), vertex->z());
        }
    }
    glEnd();
}

// Moves the skeleton up and down, obviously this is poorly named..
// we'll work on that.
void Scene::moveTestSkeleton(float x, float y, float z) {
    // IK can only solve for the end effector, so we want to find the last
    // element and move it.
    delta.x() += x;
    delta.y() += y;
    delta.z() += z;
    
    cout << "TARGET POSITION\n" << delta << endl;
    kinematics->solveIK(&(kinematics->path_[2]), delta);
}

void Scene::moveJoint(Vector3f direction){
    /* we normalize the direction passed in,
     * so don't depend on its magnitude staying the same
     */
    if(selectedJointId < 0){
        return;
    }
    direction.normalize();

    Vector3f currentPosition = kinematics->path_[selectedJointId].pos();
    Vector3f newPosition = currentPosition + direction/5;

    kinematics->solveIK(&(kinematics->path_[selectedJointId]), newPosition);
    updateSkeletonJointPositions();
}

void Scene::onLeftClick(int mouseX, int mouseY) {
    /*
     * the x and y are where the click happened, where
     * x=0 is left side of the screen and
     * y=0 is the bottom of the screen
     */
    if(converter != NULL)
        delete converter;
    converter = new MouseToWorldConverter();

    double x, y, z;
    converter->convert(mouseX, mouseY, x, y, z);
    mouseClickStartX = x;
    mouseClickStartY = y;
    mousePreviousX = mouseClickStartX;
    mousePreviousY = mouseClickStartY;
    printf("Scene::onLeftClick called with x=%f, y=%f\n", x, y);
    mouseButtonPressed = GLUT_LEFT_BUTTON;
    GLint numHits;

    glSelectBuffer(PICK_BUFFER_SIZE, pickBuffer);
    renderMode = GL_SELECT;
    glRenderMode(renderMode);
    refreshCamera(mouseX, mouseY);
    draw();

    renderMode = GL_RENDER;
    numHits = glRenderMode(renderMode);
    // if numHits == -1, there was a overflow in the selection buffer
    if (numHits == 0){
        selectedJointId = -1;
        return;
    }

    printf("Scene got %d hits\n", numHits);
    // we successfully hit a named object!
    GLuint numItems, item;
    float zMin, zMax;
    // we only care about the first hit for now
    numItems = pickBuffer[0];
    zMin = pickBuffer[1] / (pow(2.0, 32.0) - 1.0);
    zMax = pickBuffer[2] / (pow(2.0, 32.0) - 1.0);

    hitZ = (zMax + zMin) / 2.0;
    printf("numItems: %d\nzMin: %f\nzMax: %f\n",
           numItems, zMin, zMax);
    printf("Setting hitZ: %f\n", hitZ);
    for(unsigned int j=0; j<numItems; j++){
        item = pickBuffer[3 + j];
        printf("item: %d\n", item);
        if(skeleton->isEndEffector(item)){
            selectedJointId = item;
        } else {
            printf("Clicked joint %d, but it's not end effector\n", item);
        }
    }
    printf("\n");
}

void Scene::onLeftRelease(int mouseX, int mouseY) {
    double x, y, z;
    converter->convert(mouseX, mouseY, x, y, z);
    //printf("Scene::onLeftRelease called with x=%f, y=%f\n", x, y);
    mouseButtonPressed = 0;
}

void Scene::onRightClick(int mouseX, int mouseY){
    if(converter != NULL)
        delete converter;
    converter = new MouseToWorldConverter();

    mouseButtonPressed = GLUT_RIGHT_BUTTON;

    windowPreviousX = mouseX;
    windowPreviousY = mouseY;
}

void Scene::onRightRelease(int mouseX, int mouseY){
    mouseButtonPressed = 0;
}

void Scene::onMouseMotion(int mouseX, int mouseY) {
    // this function is called every time the mouse moves while a button is pressed


    double x, y, z;
    converter->convert(mouseX, mouseY, x, y, z);
    Eigen::Vector3f position = Eigen::Vector3f(x,y, hitZ);

    //double dX = x - mouseClickStartX;
    //double dY = y - mouseClickStartY;

    double eX = x - mousePreviousX;
    double eY = y - mousePreviousY;

    //printf("Scene::onMouseMotion called with x=%f, y=%f    dX=%f, dY=%f\n", x, y, dX, dY);
    if(mouseButtonPressed == GLUT_LEFT_BUTTON){
        if(selectedJointId < 0){
            // translation
            translateX += eX;
            translateY += eY;
        } else {
            printf("Trying to move to (%f, %f, %f)\n",
                   position.x(), position.y(), position.z());
            kinematics->solveIK(&(kinematics->path_[selectedJointId]), position);
            updateSkeletonJointPositions();
        }
    } else if (mouseButtonPressed == GLUT_RIGHT_BUTTON) {
        // right button is rotation
        int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
        int windowWidth = glutGet(GLUT_WINDOW_WIDTH);

        rotateAboutY += mouseX - windowPreviousX;
        rotateAboutX -= mouseY - windowPreviousY;
    }
    mousePreviousX = x;
    mousePreviousY = y;

    windowPreviousX = mouseX;
    windowPreviousY = mouseY;
}

void Scene::updateSkeletonJointPositions(){
    for(int i=0; i < int(kinematics->path_.size()); i++){
        skeleton->joints[i]->updateLink(kinematics->path_[i]);
    }
}

void Scene::onZoomIn(){
    scaleFactor += 0.05;
}

void Scene::onZoomOut(){
    scaleFactor -= 0.05;
}

MouseToWorldConverter::MouseToWorldConverter(){
    glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);
}

void MouseToWorldConverter::convert(int mouseX, int mouseY, double& objX, double& objY, double& objZ) {
    gluUnProject(mouseX, mouseY, 0,
                 modelViewMatrix, projectionMatrix, viewport,
                 &objX, &objY, &objZ);
}

