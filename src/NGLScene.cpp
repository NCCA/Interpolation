#include <QMouseEvent>
#include <QGuiApplication>
#include <QFont>
#include <QTextStream>
#include "NGLScene.h"
#include <ngl/Transformation.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>

NGLScene::NGLScene()
{
  setTitle("Using the ngl::Util interpolation templates");
  m_time = 0;
  m_animate = true;
}

NGLScene::~NGLScene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";
}

void NGLScene::resizeGL(int _w, int _h)
{
  m_project = ngl::perspective(45.0f, static_cast<float>(_w) / _h, 0.05f, 350.0f);
  m_win.width = static_cast<int>(_w * devicePixelRatio());
  m_win.height = static_cast<int>(_h * devicePixelRatio());
}

void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::initialize();

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f); // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  // we are creating a shader called Phong
  ngl::ShaderLib::createShaderProgram("Phong");
  // now we are going to create empty shaders for Frag and Vert
  ngl::ShaderLib::attachShader("PhongVertex", ngl::ShaderType::VERTEX);
  ngl::ShaderLib::attachShader("PhongFragment", ngl::ShaderType::FRAGMENT);
  // attach the source
  ngl::ShaderLib::loadShaderSource("PhongVertex", "shaders/PhongVertex.glsl");
  ngl::ShaderLib::loadShaderSource("PhongFragment", "shaders/PhongFragment.glsl");
  // compile the shaders
  ngl::ShaderLib::compileShader("PhongVertex");
  ngl::ShaderLib::compileShader("PhongFragment");
  // add them to the program
  ngl::ShaderLib::attachShaderToProgram("Phong", "PhongVertex");
  ngl::ShaderLib::attachShaderToProgram("Phong", "PhongFragment");
  // now we have associated this data we can link the shader
  ngl::ShaderLib::linkProgramObject("Phong");
  ngl::ShaderLib::use("Phong");
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0, 0, 20);
  ngl::Vec3 to(0, 0, 0);
  ngl::Vec3 up(0, 1, 0);
  // now load to our new camera
  m_view = ngl::lookAt(from, to, up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_project = ngl::perspective(45.0f, static_cast<float>(width()) / height(), 0.05f, 350.0f);
  ngl::ShaderLib::setUniform("viewerPos", from);

  ngl::Vec4 lightPos(2.0f, 5.0f, 2.0f, 0.0f);
  ngl::ShaderLib::setUniform("light.position", lightPos);
  ngl::ShaderLib::setUniform("light.ambient", 0.2f, 0.2f, 0.2f, 1.0f);
  ngl::ShaderLib::setUniform("light.diffuse", 1.0f, 1.0f, 1.0f, 1.0f);
  ngl::ShaderLib::setUniform("light.specular", 0.8f, 0.8f, 0.8f, 1.0f);
  // gold like phong material
  ngl::ShaderLib::setUniform("material.ambient", 0.274725f, 0.1995f, 0.0745f, 0.0f);
  ngl::ShaderLib::setUniform("material.diffuse", 0.75164f, 0.60648f, 0.22648f, 0.0f);
  ngl::ShaderLib::setUniform("material.specular", 0.628281f, 0.555802f, 0.3666065f, 0.0f);
  ngl::ShaderLib::setUniform("material.shininess", 51.2f);

  m_text = std::make_unique<ngl::Text>("fonts/Arial.ttf", 16);
  m_text->setColour(1, 1, 1);
  m_text->setScreenSize(width(), height());
  startTimer(20);
}

void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib::use("Phong");
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M = m_transform.getMatrix();
  MV = m_view * M;
  MVP = m_project * MV;
  normalMatrix = MV;
  normalMatrix.inverse().transpose();
  ngl::ShaderLib::setUniform("MV", MV);
  ngl::ShaderLib::setUniform("MVP", MVP);
  ngl::ShaderLib::setUniform("normalMatrix", normalMatrix);
  ngl::ShaderLib::setUniform("M", M);
}

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, m_win.width, m_win.height);
  ngl::ShaderLib::use("Phong");

  static ngl::Vec3 start(-8, -5, 0);
  static ngl::Vec3 end(8, 5, 0);

  // draw
  ngl::Vec3 lpos = ngl::lerp(start, end, m_time);
  m_transform.setPosition(lpos);
  setMaterial(Material::GOLD);

  loadMatricesToShader();
  ngl::VAOPrimitives::draw("teapot");

  ngl::Vec3 tpos = ngl::trigInterp(start, end, m_time);
  tpos.m_y = tpos.m_y + 2.0f;
  m_transform.setPosition(tpos);
  setMaterial(Material::PEWTER);

  loadMatricesToShader();
  ngl::VAOPrimitives::draw("teapot");

  ngl::Vec3 cpos = ngl::cubic(start, end, m_time);
  cpos.m_y = cpos.m_y - 2.0f;
  m_transform.setPosition(cpos);
  setMaterial(Material::BRASS);

  loadMatricesToShader();
  ngl::VAOPrimitives::draw("teapot");

  QString text = QString("T=%1").arg(m_time);
  m_text->renderText(10, 700, fmt::format("T={:0.2f}", m_time));
  m_text->renderText(10, 680, fmt::format("Trigonomic interpolation [{:0.4f} {:0.4f} {:0.4f}]", tpos.m_x, tpos.m_y, tpos.m_z));
  // Note we can use QTextString to build string as well
  QTextStream(&text) << "Linear interpolation [" << lpos.m_x << ',' << lpos.m_y << ',' << lpos.m_z << ']';

  m_text->renderText(10, 660, fmt::format("Linear interpolation [{:0.4f} {:0.4f} {:0.4f}]", lpos.m_x, lpos.m_y, lpos.m_z));

  m_text->renderText(10, 640, fmt::format("Cubic interpolation [{:0.4f} {:0.4f} {:0.4f}]", cpos.m_x, cpos.m_y, cpos.m_z));
}

//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape:
    QGuiApplication::exit(EXIT_SUCCESS);
    break;
  // turn on wirframe rendering
  case Qt::Key_W:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  // turn off wire frame
  case Qt::Key_S:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  // show full screen
  case Qt::Key_F:
    showFullScreen();
    break;
  // show windowed
  case Qt::Key_N:
    showNormal();
    break;
  case Qt::Key_Space:
    m_animate ^= true;
    break;
  case Qt::Key_Left:
    m_time -= 0.01f;
    if (m_time < 0.0f)
      m_time = 0.0f;
    break;
  case Qt::Key_Right:
    m_time += 0.01f;
    if (m_time > 1.0f)
      m_time = 1.0f;
    break;
  default:
    break;
  }
  // finally update the GLWindow and re-draw
  // if (isExposed())
  update();
}

void NGLScene::timerEvent(QTimerEvent *)
{
  if (m_animate == true)
  {
    m_time += 0.01f;
    if (m_time >= 1.0f)
    {
      m_time = 0.0f;
    }
    update();
  }
}

void NGLScene::setMaterial(const Material &_m)
{
  const static float material[][10] = {
      {0.274725f, 0.1995f, 0.0745f, 0.75164f, 0.60648f, 0.22648f, 0.628281f,
       0.555802f, 0.3666065f, 51.2f}, // Gold
      {0.329412f, 0.223529f, 0.027451f, 0.780392f, 0.568627f, 0.113725f, 0.992157f,
       0.941176f, 0.807843f, 27.8974f}, // Brass
      {0.10588f, 0.058824f, 0.113725f, 0.427451f, 0.470588f, 0.541176f,
       0.3333f, 0.3333f, 0.521569f, 9.84615f} // Pewter
  };
  int i = static_cast<int>(_m);
  ngl::ShaderLib::use("Phong");
  ngl::ShaderLib::setUniform("material.ambient", material[i][0], material[i][1], material[i][2], 1.0f);
  ngl::ShaderLib::setUniform("material.diffuse", material[i][3], material[i][4], material[i][5], 1.0f);
  ngl::ShaderLib::setUniform("material.specular", material[i][6], material[i][7], material[i][8], 1.0f);
  ngl::ShaderLib::setUniform("material.shininess", material[i][9]);
}
