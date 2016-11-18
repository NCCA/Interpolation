#include <QMouseEvent>
#include <QGuiApplication>
#include <QFont>
#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Transformation.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>



NGLScene::NGLScene()
{
  setTitle("Using the ngl::Util interpolation templates");
  m_time=0;
  m_animate=true;

}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL( int _w, int _h )
{
  m_cam.setShape( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::instance();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  // we are creating a shader called Phong
  shader->createShaderProgram("Phong");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("PhongVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("PhongFragment",ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource("PhongVertex","shaders/PhongVertex.glsl");
  shader->loadShaderSource("PhongFragment","shaders/PhongFragment.glsl");
  // compile the shaders
  shader->compileShader("PhongVertex");
  shader->compileShader("PhongFragment");
  // add them to the program
  shader->attachShaderToProgram("Phong","PhongVertex");
  shader->attachShaderToProgram("Phong","PhongFragment");
  // now bind the shader attributes for most NGL primitives we use the following
  // layout attribute 0 is the vertex data (x,y,z)
  shader->bindAttribute("Phong",0,"inVert");
  // attribute 1 is the UV data u,v (if present)
  shader->bindAttribute("Phong",1,"inUV");
  // attribute 2 are the normals x,y,z
  shader->bindAttribute("Phong",2,"inNormal");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("Phong");
  // and make it active ready to load values
  (*shader)["Phong"]->use();
  // the shader will use the currently active material and light0 so set them
  ngl::Material m(ngl::STDMAT::GOLD);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,0,20);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,1,0);
  // now load to our new camera
  m_cam.set(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam.setShape(45,(float)width()/height(),0.05,350);
  shader->setShaderParam3f("viewerPos",m_cam.getEye().m_x,m_cam.getEye().m_y,m_cam.getEye().m_z);
  // now create our light this is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  ngl::Mat4 iv=m_cam.getViewMatrix();
  iv.transpose();
  ngl::Light light(ngl::Vec3(-2,5,2),ngl::Colour(1,1,1,1),ngl::Colour(1,1,1,1),ngl::LightModes::POINTLIGHT);
  light.setTransform(iv);
  // load these values to the shader as well
  light.loadToShader("light");
  m_text.reset( new ngl::Text(QFont("Arial",16)));
  m_text->setColour(1,1,1);
  m_text->setScreenSize(width(),height());
  startTimer(10);

}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M=m_transform.getMatrix();
  MV=  M*m_cam.getViewMatrix();
  MVP=  MV*m_cam.getProjectionMatrix();
  normalMatrix=MV;
  normalMatrix.inverse();
  shader->setShaderParamFromMat4("MV",MV);
  shader->setShaderParamFromMat4("MVP",MVP);
  shader->setShaderParamFromMat3("normalMatrix",normalMatrix);
  shader->setShaderParamFromMat4("M",M);
}

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_win.width,m_win.height);
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["Phong"]->use();


  static ngl::Vec3 start(-8,-5,0);
  static ngl::Vec3 end(8,5,0);

   // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  // draw
  ngl::Vec3 lpos=ngl::lerp(start,end,m_time);
  m_transform.setPosition(lpos);
  ngl::Material m(ngl::STDMAT::GOLD);
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");

  loadMatricesToShader();
  prim->draw("teapot");


  ngl::Vec3 tpos=ngl::trigInterp(start,end,m_time);
  tpos.m_y=tpos.m_y+2.0;
  m_transform.setPosition(tpos);
  m.change(ngl::STDMAT::PEWTER);
      // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");

  loadMatricesToShader();
  prim->draw("teapot");

  ngl::Vec3  cpos=ngl::cubic(start,end,m_time);
  cpos.m_y=cpos.m_y-2.0;
  m_transform.setPosition(cpos);
  m.change(ngl::STDMAT::BRASS);
      // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader("material");

  loadMatricesToShader();
  prim->draw("teapot");

  QString text=QString("T=%1").arg(m_time);
  m_text->renderText(10,18,text );
  text.sprintf("Trigonomic interpolation [%0.4f %0.4f %0.4f]",tpos.m_x,tpos.m_y,tpos.m_z);
  m_text->renderText(10,40,text );
  text.sprintf("Linear interpolation [%0.4f %0.4f %0.4f]",lpos.m_x,lpos.m_y,lpos.m_z);
  m_text->renderText(10,60,text );
  text.sprintf("Cubic interpolation [%0.4f %0.4f %0.4f]",cpos.m_x,cpos.m_y,cpos.m_z);
  m_text->renderText(10,80,text );
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent( QMouseEvent* _event )
{
  // note the method buttons() is the button state when event was called
  // that is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if ( m_win.rotate && _event->buttons() == Qt::LeftButton )
  {
    int diffx = _event->x() - m_win.origX;
    int diffy = _event->y() - m_win.origY;
    m_win.spinXFace += static_cast<int>( 0.5f * diffy );
    m_win.spinYFace += static_cast<int>( 0.5f * diffx );
    m_win.origX = _event->x();
    m_win.origY = _event->y();
    update();
  }
  // right mouse translate code
  else if ( m_win.translate && _event->buttons() == Qt::RightButton )
  {
    int diffX      = static_cast<int>( _event->x() - m_win.origXPos );
    int diffY      = static_cast<int>( _event->y() - m_win.origYPos );
    m_win.origXPos = _event->x();
    m_win.origYPos = _event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();
  }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent( QMouseEvent* _event )
{
  // that method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if ( _event->button() == Qt::LeftButton )
  {
    m_win.origX  = _event->x();
    m_win.origY  = _event->y();
    m_win.rotate = true;
  }
  // right mouse translate mode
  else if ( _event->button() == Qt::RightButton )
  {
    m_win.origXPos  = _event->x();
    m_win.origYPos  = _event->y();
    m_win.translate = true;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent( QMouseEvent* _event )
{
  // that event is called when the mouse button is released
  // we then set Rotate to false
  if ( _event->button() == Qt::LeftButton )
  {
    m_win.rotate = false;
  }
  // right mouse translate mode
  if ( _event->button() == Qt::RightButton )
  {
    m_win.translate = false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent( QWheelEvent* _event )
{

  // check the diff of the wheel position (0 means no change)
  if ( _event->delta() > 0 )
  {
    m_modelPos.m_z += ZOOM;
  }
  else if ( _event->delta() < 0 )
  {
    m_modelPos.m_z -= ZOOM;
  }
  update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  case Qt::Key_Space : m_animate^=true; break;
  case Qt::Key_Left :
      m_time-=0.01f;
      if(m_time<0.0f)
        m_time=0.0f;
  break;
  case Qt::Key_Right :
      m_time+=0.01f;
      if(m_time>1.0f)
        m_time=1.0f;
  break;
  default : break;

  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    update();
}

void NGLScene::timerEvent( QTimerEvent *_event)
{
  if(m_animate==true)
  {
    m_time+=0.01;
    if (m_time >=1.0)
    {
      m_time=0.0;
    }
    update();
  }
}

